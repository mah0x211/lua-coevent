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

  lib/queue.lua
  lua-coevent
  Created by Masatoshi Teruya on 16/12/22.

--]]
--- file scope variables
local next = next;


--- class Queue
local Queue = {};


--- len
-- @return len
function Queue:len()
    return self.nobj;
end


--- push
-- @param data
function Queue:push( data )
    local item = next( self.pool );

    -- found at pool
    if item then
        self.pool[item] = nil;
        item.data = data;
    -- create new item
    else
        item = { data = data };
    end

    -- set item as head and tail
    if self.nobj == 0 then
        self.nobj = 1;
        self.head = item;
        self.tail = item;
    -- insert into tail
    else
        self.nobj = self.nobj + 1;
        self.tail.next = item;
        self.tail = item;
    end
end


--- pop
-- @return data
function Queue:pop()
    local item = self.head;

    if item then
        local data = item.data;

        -- remove head and tail
        if self.nobj == 1 then
            self.nobj = 0;
            self.head = nil;
            self.tail = nil;
        -- set head to next
        else
            self.nobj = self.nobj - 1;
            self.head = item.next;
        end
        -- remove references
        item.next = nil;
        item.data = nil;
        self.pool[item] = true;

        return data;
    end
end


--- new
-- @return queue
local function new()
    return setmetatable({
        nobj = 0,
        pool = setmetatable({},{ __mode = 'k' })
    }, {
        __index = Queue
    });
end


return {
    new = new
};
