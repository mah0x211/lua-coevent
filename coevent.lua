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

local reco = require('reco');
local getSentry = require('sentry').default;

-- private functions

local function defaultException( err, ev, evtype, hup, handler )
    print( 'CoEvent Exception: ', err, ev, evtype, hup, handler );
    
    -- close all event-watcher
    handler:close();

    return true;
end


-- MARK: class CoEventHandler
local CoEventHandler = require('halo').class.CoEventHandler;


--- init
-- @param   coevent     instance of CoEvent
-- @param   exception   function
-- @param   handler     function
-- @return  instance
-- @return  err
function CoEventHandler:init( coevt, exception, handler, ctx, ... )
    local own = protected( self );
    local err;
    
    -- use default exception handler
    if exception == nil then
        own.exception = defaultException;
    -- create exception coroutine
    else
        own.exception, err = reco.new( exception, ctx );
        if err then
            return nil, err;
        end
    end
    
    -- create handler coroutine
    own.handler, err = reco.new( handler, ctx, ... );
    -- got error
    if err then
        return nil, err;
    end
    
    own.coevt = coevt;
    -- create weak table for event container
    own.evs = setmetatable({},{
        __mode = 'k'
    });
    
    return self;
end


--- close
function CoEventHandler:close()
    local evs = protected( self ).evs;
    
    -- unwatch all registered events
    for ev in pairs( evs ) do
        ev:unwatch();
    end
end


--- invoke
-- @param   interval
-- @param   oneshot
-- @return  ev
-- @return  err
function CoEventHandler:invoke( ev, evtype, hup )
    local own = protected( self );
    local ok, err = own.handler( ev, evtype, hup, self );
    
    -- got error
    if not ok then
        ok, err = own.exception( err, ev, evtype, hup, self );
        -- close handler
        if not ok then
            defaultException( err, ev, evtype, hup, self );
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


--- createWather
--- createHandler
-- @param   exception
-- @param   fn
-- @param   ctx
-- @param   ...
-- @return  handler
-- @return  err
function CoEvent:createHandler( exception, fn, ctx, ... )
    return CoEventHandler.new( protected( self ), exception, fn, ctx, ... );
end


--- start
-- @return  err
function CoEvent:start()
    local own = protected( self );
    local sentry, err, nevt, ev, evtype, hup, handler;
    
    -- running already
    if own.running then
        return nil;
    end
    
    -- get default sentry
    sentry, err = getSentry();
    if err then
        return err;
    end
    
    -- run
    own.running = true;
    repeat
        -- wait events forever
        nevt, err = sentry:wait();
        
        -- got critical error
        if err then
            return err;
        -- consuming events
        elseif nevt > 0 then
            -- get the occurred event sequentially
            ev, evtype, hup, handler = sentry:getevent();
            
            while ev do
                -- invoke handler
                handler:invoke( ev, evtype, hup );
                -- get next occurred event
                ev, evtype, hup, handler = sentry:getevent();
            end
        end
    until not own.running or #sentry == 0;
    own.running = nil;
    
    return err;
end


--- stop
function CoEvent:stop()
    protected( self ).running = nil;
end


-- exports
return CoEvent.exports;

