--[[

  Copyright (C) 2016 Masatoshi Teruya

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

  lib/runq.lua
  lua-coevent
  Created by Masatoshi Teruya on 16/03/17.

--]]

-- assign to local
local setmetatable = setmetatable;

-- class
local RunQ = {};


--- add
-- @param   co
function RunQ:add( co )
    if not self.queue[co] then
        self.nqueue = self.nqueue + 1;
        self.queue[co] = true;
    end
end


--- remove
-- @param   co
function RunQ:remove( co )
    if self.queue[co] then
        self.nqueue = self.nqueue - 1;
        self.queue[co] = nil;
    end
end


--- consume
-- @return queue
function RunQ:consume()
    local queue = self.queue;

    self.nqueue = 0;
    self.queue = {};

    return queue;
end


--- protection
local function assert_newindex()
    error('attempt to add value to protected table', 2 );
end


-- create new RunQ
local function new()
    return setmetatable({
        nqueue = 0,
        queue = {}
    },{
        __newindex = assert_newindex,
        __index = RunQ
    });
end


--- exports
return {
    new = new;
};


