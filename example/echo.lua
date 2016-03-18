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

local coevent = require('coevent');
local InetServer = require('net.stream.inet.server');
local yield = coroutine.yield;
-- constants
local HOST = '127.0.0.1';
local PORT = '5000';


local function handleClient( sock )
    local evRead, msg, again, err, ishup;

    -- print('handleClient', sock:fd());
    -- close socket
    coevent.deferCo(function( sock )
        -- print('defer handleClient', sock:fd() );
        sock:close();
    end, sock );

    evRead, err = coevent.watchReadable( sock:fd() );
    if err then
        error( err );
    end

    while true do
        msg, err, again = sock:recv();
        -- print('recv', msg, err, again)
        -- got internal error
        if err then
            error( err );
        elseif again then
            -- print('yield recv')
            _, _, ishup = yield();
            -- print('resume recv', ishup)
            -- closed by peer
            if ishup then
                break;
            end
        -- closed by peer
        elseif not msg then
            -- print('no msg');
            break;
        else
            len, err, again = sock:send( msg );
            -- print('send', len, err, again)
            if err then
                print(err);
                break;
            end
        end
    end
end


local function createServer()
    -- create loop
    local server = InetServer.new({
        host = HOST;
        port = PORT,
        reuseaddr = true,
        nonblock = true
    });
    local evRead, ishup, sock, again, err;

    err = err or server:listen( BACKLOG );
    if err then
        error( err );
    end

    evRead, err = coevent.watchReadable( server:fd() );
    if err then
        error( err );
    end

    -- start server
    print( 'start server: ', HOST, PORT );

    while true do
        sock, err, again = server:accept();
        -- print('accept', sock, err, again)
        -- got error
        if err then
            print( err );
        -- got client
        elseif sock then
            err = coevent.spawn( handleClient, sock );
            -- print('spawn', err);
        -- wait event
        else
            -- print('yield accept');
            _, _, ishup = yield();
            -- print('resume accept', ishup);
            -- event hangup
            if ishup then
                -- print('close server');
                server:close();
                break;
            end
        end
    end

    print( 'end server' );
end

coevent.runloop( createServer );


