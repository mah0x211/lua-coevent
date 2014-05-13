--[[
  
  Copyright (C) 2014 Masatoshi Teruya

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

  example/exception.lua
  lua-coevent

  Created by Masatoshi Teruya on 14/05/13.
  
--]]

local event = require('coevent');
-- https://github.com/mah0x211/lua-signal
local signal = require('signal');
-- block SIGINT
signal.block( signal.INT );

local function callback( ctx, watcher, hup )
    ctx.count = ctx.count + 1;
    print( 'yield' );
    -- throw an exception error
    coroutine.yield();
    print( 'resume' );
    print( 'callback', ctx.count );
    if ctx.count > 2 then
        watcher:unwatch();
    end
end

-- exception handler
local function exception( ctx, watcher, info )
    print( 'got exception' );
    -- show debug info
    for k,v in pairs( info ) do
        print( k, v );
    end
end

-- create loop: use default limit
local loop = event.loop( nil, exception );
-- create signal SIGINT watcher
local watcher = event.signal( loop, signal.INT );
local oneshot = true;

watcher:watch( oneshot, callback, { count = 0 } );
print( 'type ^C' );
print( 'done', loop:run() );

