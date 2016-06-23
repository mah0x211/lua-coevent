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

  lib/thread.lua
  lua-coevent
  Created by Masatoshi Teruya on 16/06/21.

--]]

-- assign to local
local newco = coroutine.create;
local resume = coroutine.resume;
local status = coroutine.status;
local traceback = debug.traceback;
-- constants
local OK = 0;
local YIELD = 1;
local ERR = 2;
-- variables
local start;


local function again( tbl, ... )
    local ok, err = resume( tbl.co, ... );

    if not ok then
        tbl[0].__call = start;
        return ERR, err, traceback( tbl.co, err );
    elseif status( tbl.co ) == 'suspended' then
        return YIELD;
    end

    tbl[0].__call = start;
    return OK;
end


start = function( tbl, ... )
    local ok, err = resume( tbl.co, tbl.ctx, ... );

    if not ok then
        return ERR, err, traceback( tbl.co, err );
    elseif status( tbl.co ) == 'suspended' then
        tbl[0].__call = again;
        return YIELD;
    end

    return OK;
end


local function init( tbl, fn, ctx )
    tbl.co = newco( fn );
    tbl.ctx = ctx;
    tbl[0].__call = start;
end


-- create new thread
local function new( fn, ctx )
    local state = {
        __call = start,
        __index = {
            init = init
        }
    };

    return setmetatable({
        co = newco( fn ),
        ctx = ctx,
        [0] = state
    }, state );
end


--- exports
return {
    new = new,
    OK = OK,
    ERR = ERR,
    YIELD = YIELD
};

