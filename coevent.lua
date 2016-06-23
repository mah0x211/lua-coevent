--[[

  Copyright (C) 2015 Masatoshi Teruya

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.

  coevent.lua
  lua-coevent
  Created by Masatoshi Teruya on 15/09/05.

--]]

-- assign to local
local inspect = require('util').inspect;
local bitvec = require('bitvec');
local sentry = require('sentry');
local thread = require('coevent.thread');
local newthread = thread.new;
local pcall = pcall;
local pairs = pairs;
local unpack = unpack or table.unpack;
local setmetatable = setmetatable;
local RunQ = require('coevent.runq');
-- constants
-- coroutine status
local OK = thread.OK;
local YIELD = thread.YIELD;
-- variables
local EvLoop = assert( sentry.default() );
local EvPool = setmetatable( {}, { __mode = 'k' } );
local CoPool = setmetatable( {}, { __mode = 'k' } );
local RQ;
local ActiveCo;


--- revoke
-- @param   id
-- @return  ok
local function revoke( id )
    local co = ActiveCo;

    if co then
        local ev = co.evids[id];

        if ev then
            ev:revert();
            co.events[ev], co.evids[id] = nil, nil;
            co.bitvec:unset( id );
            -- push to event pool
            EvPool[ev] = true;

            return true;
        end

        return false;
    else
        error( 'cannot revoke an event at outside of runloop()', 2 );
    end
end


--- on
-- @param   asa
-- @param   val
-- @param   oneshot
-- @param   edge
-- @return  id
-- @return  err
local function on( asa, val, oneshot, edge )
    local co = ActiveCo;

    if co then
        local ev = next( EvPool );
        local err;

        -- remove from pool
        if ev then
            EvPool[ev] = nil;
        else
            ev, err = EvLoop:newevent();
        end

        if not err then
            err = ev[asa]( ev, val, co, oneshot, edge );
            if not err then
                local id = co.bitvec:ffz();

                co.bitvec:set( id );
                co.nevent = co.nevent + 1;
                co.events[ev], co.evids[id] = id, ev;
                -- remove from RunQ
                RQ:remove( co );

                return id;
            end
        end

        return nil, err;
    end

    error( 'cannot watching an event at outside of runloop()', 2 );
end


--- onSignal
-- @param   signo
-- @param   oneshot
-- @return  ev
-- @return  err
local function onSignal( signo, oneshot )
    return on( 'assignal', signo, oneshot );
end


--- onTimer
-- @param   ival
-- @param   oneshot
-- @return  ev
-- @return  err
local function onTimer( ival, oneshot )
    return on( 'astimer', ival, oneshot );
end


--- onWritable
-- @param   fd
-- @param   oneshot
-- @param   edge
-- @return  ev
-- @return  err
local function onWritable( fd, oneshot, edge )
    return on( 'aswritable', fd, oneshot, edge );
end


--- onReadable
-- @param   fd
-- @param   oneshot
-- @param   edge
-- @return  ev
-- @return  err
local function onReadable( fd, oneshot, edge )
    return on( 'asreadable', fd, oneshot, edge );
end


--- dispose
-- @param coroutine
local function dispose( co )
    local deferCo = co.deferCo;
    local evids = co.evids;

    -- unwatch all registered events
    for ev, id in pairs( co.events ) do
        ev:revert();
        evids[id] = nil;
        EvPool[ev] = true;
    end

    -- set nil to release references
    co.ctx = nil;
    co.errfn = nil;
    co.evids = nil;
    co.events = nil;
    CoPool[co] = true;
    -- remove coroutine from RunQ
    RQ:remove( co );

    -- run deferCo function
    if deferCo then
        local ok, err = pcall( unpack( deferCo ) );

        co.deferCo = nil;
        if not ok then
            error( err );
        end
    end
end


--- defaultErrorFn
-- @param ...
local function defaultErrorFn( ... )
    print( 'defaultErrorFn', ... );
end


--- invoke
-- @param co
-- @param ...
local function invoke( co, ... )
    local rc, err, trace;

    ActiveCo = co;
    rc, err, trace = co( ... );
    ActiveCo = nil;

    -- got error
    if err then
        -- invoke error function
        local ok, errerr = pcall( co.errfn, co.ctx, err, trace );

        -- invoke default error function
        if not ok then
            defaultErrorFn( errerr, err, trace );
        end

        -- dispose coroutine with error
        dispose( co );
    -- dispose coroutine if finished
    elseif rc == OK then
        dispose( co );
    -- add to the run queue
    elseif co.nevent == 0 then
        RQ:add( co );
    end
end


--- spawn
-- @param fn
-- @param ctx
-- @param errfn
local function spawn( fn, ctx, errfn )
    local co = next( CoPool );

    if co then
        CoPool[co] = nil;
        co:init( fn, ctx );
    else
	    co = newthread( fn, ctx );
    end

    -- append management fields
    co.errfn = errfn or defaultErrorFn;
    co.nevent = 0;
    co.bitvec = bitvec.new();
    co.evids = {};
    co.events = {};
    -- add to runq
    RQ:add( co );
end


--- defer
-- @param fn
local function deferCo( fn, ... )
    local co = ActiveCo;

    if not co then
        error( 'cannot register the defer function at outside of runloop()', 2 );
    elseif type( fn ) ~= 'function' then
        error( 'the defer function must be function', 2 );
    end

    co.deferCo = { fn, ... };
end


--- runQueue
-- @return nrunq
local function runQueue()
    local rq = RQ;

    while rq.nqueue > 0 do
        for co in pairs( rq:consume() ) do
            invoke( co );
        end

        if #EvLoop > 0 then
            return rq.nqueue;
        end
    end

    return rq.nqueue;
end


--- runloop
-- @param fn
-- @param ctx
-- @param errfn
-- @return err
local function runloop( fn, ctx, errfn )
    local err;

    -- create RunQ
    RQ = RunQ.new();
    -- create new coroutine
    err = spawn( fn, ctx, errfn );
    if not err then
        local nrunq, nevt, ev, ishup, co, disabled, id, _;

        -- have events
        repeat
            -- invoke runq
            nrunq = runQueue();

            -- wait events forever
            nevt, err = EvLoop:wait( nrunq == 0 and -1 or 0.5 );

            -- got critical error
            if err then
                break;
            -- consuming events
            elseif nevt > 0 then
                ev, _, ishup, co, disabled = EvLoop:getevent();
                while ev do
                    id = co.events[ev];
                    -- remove disabled event object
                    if disabled then
                        co.events[ev], co.evids[id] = nil, nil;
                        co.nevent = co.nevent - 1;
                        -- push to event pool
                        ev:revert();
                        EvPool[ev] = true;
                    end
                    invoke( co, id, ishup );
                    -- get next event
                    ev, _, ishup, co, disabled = EvLoop:getevent();
                end
            end

            -- re-invoke runq
            runQueue();
        until #EvLoop == 0;
    end

    return err;
end


-- exports
return {
    -- evtypes
    EV_READABLE = sentry.EV_READABLE,
    EV_WRITABLE = sentry.EV_WRITABLE,
    EV_TIMER    = sentry.EV_TIMER,
    EV_SIGNAL   = sentry.EV_SIGNAL,
    -- event functions
    runloop     = runloop,
    spawn       = spawn,
    deferCo     = deferCo,
    onReadable  = onReadable,
    onWritable  = onWritable,
    onTimer     = onTimer,
    onSignal    = onSignal,
    revoke      = revoke,
};

