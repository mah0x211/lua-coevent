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

-- https://github.com/mah0x211/lua-signal
local signal = require('signal');
local coevent = assert( require('coevent').default );
-- block SIGINT
signal.block( signal.SIGINT );


local function callback( ctx, ev, evtype, hup, handler )
    local a;

    ctx.count = ctx.count + 1;
    print( 'callback', ev, evtype, hup, handler, ctx.count );
    -- throw an exception error
    a = a + ctx;

    if ctx.count > 2 then
        ev:unwatch();
    end
end


-- exception handler
local function exception( ctx, handler, err, ev, evtype, hup )
    print( 'got exception', ctx, handler, err, ev, evtype, hup );
    -- unwatch event
    print( ev );
    ev:unwatch();
end


-- create handler
local h = assert( coevent:createHandler( callback, { count = 0 }, exception ) );
-- create signal SIGINT watcher
local oneshot = false;
local ev = assert( h:watchSignal( signal.SIGINT, oneshot ) );


print(
    'calling callback function by handler <' ..
    tostring(h) ..
    '> via <' ..
    tostring(ev) ..
    '> when typed "^C"'
);

repeat
    local handler, ev, evtype, ishup, err = coevent:getevent();

    if err then
        print( err );
        break;
    elseif handler then
        handler:invoke( ev, evtype, ishup );
    end
until not handler;

print( 'done' );
