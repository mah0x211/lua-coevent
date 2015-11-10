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

  example/echo.lua
  lua-coevent

  Created by Masatoshi Teruya on 15/10/06.
  
--]]

local signal = require('signal');
local bind = require('llsocket').inet.bind;
local close = require('llsocket').close;
local listen = require('llsocket').listen;
local accept = require('llsocket').accept;
local recv = require('llsocket').recv;
local send = require('llsocket').send;
local CoEvent = require('coevent');
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
    elseif not len then
        close( req.fd );
    end
end


local function onClientEvent( req, ev, evtype, hup )
    if hup then
        close( req.fd );
    -- recv a message on readable event
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
                writefd( req, msg );
            -- push sendq
            else
                pushSendQ( req, msg );
            end
        end
    -- consume a sendq on writable event
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
            elseif not len then
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
    else
        local fd = assert( accept( server.fd ) );
        local c = {
            fd = fd,
            sendq = {},
            sendqHead = 1,
            sendqTail = 0
        };

        -- create client fd handler
        c.handler = assert( server.co:createHandler( onClientEvent, c ) );
        -- watch readable event
        assert( c.handler:watchReadable( fd ) );
    end
end


local function onSignal( server )
    -- stop event loop
    server.running = nil;
end


local function createServer()
    -- create loop
    local server = {
        running = true
    };
    local sigh, h, fd, err;

    -- create coevent
    server.co = assert( CoEvent.default );
    -- create bind socket
    server.fd = assert( bind( HOST, PORT, SOCK_STREAM, NONBLOCK, true ) );
    err = listen( server.fd );
    if err then
        error( err );
    end

    -- create server fd handler
    h = assert( server.co:createHandler( onAccept, server ) );
    -- watch readable event
    assert( h:watchReadable( server.fd ) );

    -- block SIGINT
    signal.block( signal.SIGINT );
    -- create signal handler
    sigh = assert( server.co:createHandler( onSignal, server ) );
    -- watch SIGINT
    assert( sigh:watchSignal( signal.SIGINT ) );

    -- run
    print( 'start server: ', HOST, PORT );
    repeat
        local handler, ev, evtype, ishup, err = server.co:getevent();

        if err then
            print( err );
            break;
        elseif handler then
            handler:invoke( ev, evtype, ishup );
        else
            break;
        end
    until not server.running;
    print( 'end server' );
end

createServer();

