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
-- modules
local reco = require('reco');
local getSentry = require('sentry').default;
-- constants
local COEVENT_DEFAULT;

-- private functions

local function defaultException( _, handler, err )
    print( 'CoEvent Exception: ', handler, err );
    
    -- close all event-watcher
    handler:close();

    return true;
end


-- MARK: class CoEventHandler
local CoEventHandler = require('halo').class.CoEventHandler;


--- init
-- @param   handler     function
-- @param   ctx         any
-- @param   exception   function
-- @return  instance
-- @return  err
function CoEventHandler:init( handler, ctx, exception )
    local own = protected( self );
    local err;
    
    -- use default exception handler
    if exception == nil then
        own.exception = defaultException;
    -- create exception coroutine
    else
        own.exception, err = reco.new( exception );
        if err then
            return nil, err;
        end
    end
    
    -- create handler coroutine
    own.handler, err = reco.new( handler );
    -- got error
    if err then
        return nil, err;
    end
    
    -- create weak table for event container
    own.evs = setmetatable({},{
        __mode = 'k'
    });
    own.ctx = ctx;

    return self;
end


--- close
function CoEventHandler:close()
    local own = protected( self );

    -- unwatch all registered events
    for ev in pairs( own.evs ) do
        ev:unwatch();
    end
    -- release references
    own.ctx, own.handler, own.exception, own.evs = nil, nil, nil, nil;
end


--- invoke
-- @param   ...
function CoEventHandler:invoke( ... )
    local own = protected( self );
    local ok, err = own.handler( own.ctx, ... );
    
    -- got error
    if not ok then
        ok, err = own.exception( own.ctx, self, err, ... );
        -- close handler
        if not ok then
            defaultException( own.ctx, self, err, ... );
        end
    end
end


--- register
-- @param   handler
-- @param   ctx     protected table of handler
-- @param   event
-- @param   val
-- @param   ...
-- @return  ev
-- @return  err
local function register( handler, ctx, event, val, ... )
    -- get default sentry
    local sentry, err = getSentry();
    
    if not err then
        local ev;

        -- create and register event
        ev, err = sentry[event]( sentry, val, handler, ... );
        -- save event into event container
        if ev then
            ctx.evs[ev] = true;
            return ev;
        end
    end
    
    return nil, err;
end


--- watchTimer
-- @param   ival
-- @param   oneshot
-- @return  ev
-- @return  err
function CoEventHandler:watchTimer( ival, oneshot )
    return register( self, protected( self ), 'timer', ival, oneshot );
end


--- watchSignal
-- @param   signo
-- @param   oneshot
-- @return  ev
-- @return  err
function CoEventHandler:watchSignal( signo, oneshot )
    return register( self, protected( self ), 'signal', signo, oneshot );
end


--- watchReadable
-- @param   fd
-- @param   oneshot
-- @param   edge
-- @return  ev
-- @return  err
function CoEventHandler:watchReadable( fd, oneshot, edge )
    return register( self, protected( self ), 'readable', fd, oneshot, edge );
end


--- watchWritable
-- @param   fd
-- @param   oneshot
-- @param   edge
-- @return  ev
-- @return  err
function CoEventHandler:watchWritable( fd, oneshot, edge )
    return register( self, protected( self ), 'writable', fd, oneshot, edge );
end


-- exports
CoEventHandler = CoEventHandler.exports;


-- MARK: class CoEvent
local CoEvent = require('halo').class.CoEvent;


-- static properties
CoEvent.property {
    EV_READABLE = require('sentry').EV_READABLE,
    EV_WRITABLE = require('sentry').EV_WRITABLE,
    EV_TIMER = require('sentry').EV_TIMER,
    EV_SIGNAL = require('sentry').EV_SIGNAL
};


-- static metamethods

function CoEvent.__index( self, key )
    if key == 'default' then
        self.default = assert( CoEvent.new() );
        return self.default;
    end
end


-- instance methods

function CoEvent:init()
    local own = protected( self );
    local evwait, evconsume, err;

    -- get default sentry
    own.sentry, err = getSentry();
    if err then
        return nil, err;
    end

    -- wait events
    evwait = function()
        local nevt, err;

        repeat
            -- wait events forever
            nevt, err = own.sentry:wait();
            -- got critical error
            if err then
                return nil, nil, nil, nil, err;
            -- consuming events
            elseif nevt > 0 then
                -- switch event getter
                self.getevent = evconsume;
                return evconsume();
            end
        until #own.sentry == 0;
    end;

    -- consume events
    evconsume = function()
        -- get the occurred event sequentially
        ev, evtype, hup, handler = own.sentry:getevent();
        if ev then
            return handler, ev, evtype, hup;
        end

        -- switch event getter
        self.getevent = evwait;
        return evwait();
    end;

    --- getevent
    -- @return  CoEventHandler
    -- @return  ev
    -- @return  evtype
    -- @return  ishup
    -- @return  err
    -- set evwait to default event getter
    self.getevent = evwait;

    return self;
end


--- createHandler
-- @param   handlerCb
-- @param   ctx
-- @param   exceptionCb
-- @return  CoEventHandler
-- @return  err
function CoEvent:createHandler( handlerCb, ctx, exceptionCb )
    return CoEventHandler.new( handlerCb, ctx, exceptionCb );
end



-- exports
return CoEvent.exports;

