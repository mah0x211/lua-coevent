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

  example/echo_delay.lua
  lua-coevent

  Created by Masatoshi Teruya on 15/10/06.
  
--]]
local yield = coroutine.yield;
local CoEvent = require('coevent');
local signal = require('signal');
local bind = require('llsocket').inet.bind;
local close = require('llsocket').close;
local listen = require('llsocket').listen;
local accept = require('llsocket').accept;
local recv = require('llsocket').recv;
local send = require('llsocket').send;
-- constants
local SOCK_STREAM = require('llsocket').SOCK_STREAM;
local EV_READABLE = CoEvent.EV_READABLE;
local EV_WRITABLE = CoEvent.EV_WRITABLE;
local EV_SIGNAL = CoEvent.EV_SIGNAL;
local HOST = '127.0.0.1';
local PORT = '5000';
local NONBLOCK = true;


local function pushSendQ( req, msg )
    req.sendqTail = req.sendqTail + 1;
    req.sendq[req.sendqTail] = msg;
end


local function writefd( req, msg )
    local len, err, again = send( req.fd, msg );

    -- no space of send buffer
    if again then
        pushSendQ( req, msg );
        assert( req.handler:watchWritable( req.fd ) );
    -- got error
    elseif err then
        close( req.fd );
    -- close by peer
    elseif len == 0 then
        close( req.fd );
    end
end


local function onEvent( req, ev, evtype, hup )
    if hup then
        close( req.fd );
    -- recv message
    elseif evtype == EV_READABLE then
        local msg, err, again = recv( req.fd );

        if not again then
            -- got error
            if err then
                close( req.fd );
            -- close by peer
            elseif not msg then
                close( req.fd );
            -- reply
            elseif req.sendqTail == 0 then
                -- register 5 sec delay timer as oneshot event
                assert( req.handler:watchTimer( 5, true ) );
                -- unwatch readable
                ev:unwatch();
                yield();
                -- rewatch readable
                ev:watch();

                writefd( req, msg );
            -- push sendq
            else
                pushSendQ( req, msg );
            end
        end
    -- consume sendq
    elseif evtype == EV_WRITABLE then
        local sendq, head, tail = req.sendq, req.sendqHead, req.sendqTail;
        local len, err, again;

        for i = head, tail do
            len, err, again = send( req.fd, sendq[i] );
            if again then
                break;
            elseif err then
                print( err );
                close( req.fd );
                return;
            -- close by peer
            elseif len == 0 then
                close( req.fd );
                return;
            end

            -- remove message
            sendq[i] = nil;
            head = i;
        end

        -- reset sendq and head/tail position
        if head == tail then
            req.sendqHead, req.sendqTail = 1, 0;
            -- unwatch writable event
            ev:unwatch();
        end
    end
end


local function onAccept( server, ev, evtype, hup )
    if hup then
        close( server.fd );
    -- signal
    elseif evtype == EV_SIGNAL then
        -- stop event loop
        server.co:stop();
    -- readable
    else
        local fd = assert( accept( server.fd ) );
        local c = {
            fd = fd,
            sendq = {},
            sendqHead = 1,
            sendqTail = 0
        };

        -- create client fd handler
        c.handler = assert( server.co:createHandler( nil, onEvent, c ) );
        -- watch readable event
        assert( c.handler:watchReadable( fd ) );
    end
end


local function createServer()
    -- create loop
    local co = CoEvent.new();
    local h, fd, err;
    
    -- create bind socket
    fd = assert( bind( HOST, PORT, SOCK_STREAM, NONBLOCK, true ) );
    err = listen( fd );
    if err then
        error( err );
    end

    -- create server fd handler
    h = assert( co:createHandler( nil, onAccept, {
        fd = fd,
        co = co
    }));
    -- watch readable event
    assert( h:watchReadable( fd ) );

    -- block SIGINT
    signal.block( signal.SIGINT );
    -- watch SIGINT
    assert( h:watchSignal( signal.SIGINT ) );

    -- run
    print( 'start server: ', HOST, PORT );
    print( 'done', co:start() );
    print( 'end server' );
end

createServer();

