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

  example/writable_ev.lua
  lua-coevent
  Created by Masatoshi Teruya on 14/05/13.
  
--]]

local coevent = require('coevent').new();

local function callback( ctx, ev, evtype, hup )
    ctx.count = ctx.count + 1;
    print( 'callback', ev, evtype, hup, ctx.count );
    if ctx.count > 2 then
        ev:unwatch();
    end
end

-- create handler
local h = assert( coevent:createHandler( callback, { count = 0 } ) );
-- create signal SIGINT watcher
local oneshot = false;
local edge = false;
local ev = assert( h:watchWritable( 1, oneshot, edge ) );


print(
    'calling callback function by handler <' ..
    tostring(h) ..
    '> via <' ..
    tostring(ev) ..
    '> on fd is writable'
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
