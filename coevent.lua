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
local sentry = require('sentry');
local reco = require('reco');
local newco = reco.new;
local pcall = pcall;
local pairs = pairs;
local unpack = unpack or table.unpack;
local setmetatable = setmetatable;
local RunQ = require('coevent.runq');
-- constants
-- coroutine status
local OK = reco.OK;
-- variables
local EvLoop = assert( sentry.default() );
local RQ;
local ActiveCo;


--- watch
-- @param   co
-- @param   asa
-- @param   val
-- @param   ctx
-- @param   oneshot
-- @param   edge
-- @return  ev
-- @return  err
local function watch( co, asa, val, ctx, oneshot, edge )
    if co then
        local ev, err = EvLoop:newevent();

        if not err then
            err = ev[asa]( ev, val, ctx, oneshot, edge );
            if not err then
                co.nevent = co.nevent + 1;
                co.events[ev] = true;
                -- remove from RunQ
                RQ:remove( co );

                return ev;
            end
        end

        return nil, err;
    end

    error( 'cannot watching an event at outside of runloop()', 2 );
end


--- watchSignal
-- @param   signo
-- @param   oneshot
-- @return  ev
-- @return  err
local function watchSignal( signo, oneshot )
    return watch( ActiveCo, 'assignal', signo, ActiveCo, oneshot );
end


--- watchTimer
-- @param   ival
-- @param   oneshot
-- @return  ev
-- @return  err
local function watchTimer( ival, oneshot )
    return watch( ActiveCo, 'astimer', ival, ActiveCo, oneshot );
end


--- watchWritable
-- @param   fd
-- @param   oneshot
-- @param   edge
-- @return  ev
-- @return  err
local function watchWritable( fd, oneshot, edge )
    return watch( ActiveCo, 'aswritable', fd, ActiveCo, oneshot, edge );
end


--- watchReadable
-- @param   fd
-- @param   oneshot
-- @param   edge
-- @return  ev
-- @return  err
local function watchReadable( fd, oneshot, edge )
    return watch( ActiveCo, 'asreadable', fd, ActiveCo, oneshot, edge );
end


--- dispose
-- @param coroutine
local function dispose( co )
    local deferCo = co.deferCo;

    -- unwatch all registered events
    for ev in pairs( co.events ) do
        ev:revert();
    end

    -- set nil to release references
    co.events = nil;
    -- remove coroutine from RunQ
    RQ:remove( co );

    -- run deferCo function
    if co.deferCo then
        local ok, err = pcall( unpack( co.deferCo ) );

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
    local rv;

    ActiveCo = co;
    -- invoke coroutine with context
    if co.status == OK then
        rv = { co( co.ctx, ... ) };
    -- invoke coroutine without context
    else
        rv = { co( ... ) };
    end
    ActiveCo = nil;

    -- success
    if rv[1] then
        -- dispose coroutine if finished
        if co.status == OK then
            dispose( co );
        elseif co.nevent == 0 then
            RQ:add( co );
        end
    -- got error
    else
        -- invoke error function
        local ok, err = pcall( co.errfn, co.ctx, rv[2], rv[3], rv[4] );

        -- invoke default error function
        if not ok then
            defaultErrorFn( err, rv[2], rv[3], rv[4] );
        end

        -- dispose coroutine
        dispose( co );
    end
end


--- spawn
-- @param fn
-- @param ctx
-- @param errfn
-- @return err
local function spawn( fn, ctx, errfn )
    local co, err = newco( fn );

    if not err then
        -- append management fields
        co.ctx = ctx;
        co.errfn = errfn or defaultErrorFn;
        co.nevent = 0;
        co.events = {};
        -- add to runq
        RQ:add( co );
    end

    return err;
end


--- defer
-- @param fn
local function deferCo( fn, ... )
    if not ActiveCo then
        error( 'cannot register the defer function at outside of runloop()' );
    end

    ActiveCo.deferCo = { fn, ... };
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
            break;
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
        local nrunq, nevt, ev, evtype, ishup, co, disabled;

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
                ev, evtype, ishup, co, disabled = EvLoop:getevent();
                while ev do
                    -- remove disabled event object
                    if disabled then
                        co.events[ev] = nil;
                        co.nevent = co.nevent - 1;
                        ev:revert();
                    end
                    invoke( co, ev, evtype, ishup );
                    -- get next event
                    ev, evtype, ishup, co = EvLoop:getevent();
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
    runloop         = runloop,
    spawn           = spawn,
    deferCo         = deferCo,
    watchReadable   = watchReadable,
    watchWritable   = watchWritable,
    watchTimer      = watchTimer,
    watchSignal     = watchSignal,
};

