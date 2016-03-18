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
local inspect = require('util').inspect;
local sentry = require('sentry');
local pairs = pairs;
local setmetatable = setmetatable;
-- variables
local EvLoop = assert( sentry.default() );
local RunQNum = 0;
local RunQ = {};
local RunQChanges = {};
local InInvoking = false;

--- init - initialize global variables
local function init()
    EvLoop = assert( sentry.default() );
    RunQNum = 0;
    RunQ = setmetatable({},{
        __mode = 'k'
    });
    RunQChanges = {};
end


--- add
-- @param   co
local function add( co )
    if not RunQ[co] then
        RunQNum = RunQNum + 1;
        if InInvoking then
            RunQChanges[#RunQChanges + 1] = { co, true };
        else
            RunQ[co] = true;
        end
    end
end


--- remove
-- @param   co
local function remove( co )
    if RunQ[co] then
        RunQNum = RunQNum - 1;
        if InInvoking then
            RunQChanges[#RunQChanges + 1] = { co };
        else
            RunQ[co] = nil;
        end
    end
end


--- invoke
-- @param invocator
-- @return runqnum
local function invoke( invocator )
    InInvoking = true;

    while RunQNum > 0 do
        for co in pairs( RunQ ) do
            invocator( co );
        end

        -- update RunQ
        if #RunQChanges > 0 then
            for i = 1, #RunQChanges do
                RunQ[RunQChanges[i][1]], RunQChanges[i] = RunQChanges[i][2], nil;
            end
        end

        -- if event waiting
        if #EvLoop > 0 then
            break;
        end
    end

    InInvoking = false;

    return RunQNum;
end


--- protection
local function assert_newindex()
    error('attempt to add value to protected table', 2 );
end


--- exports
return {
    init    = init,
    add     = add,
    remove  = remove,
    invoke  = invoke
};

