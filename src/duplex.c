/*
 *  Copyright (C) 2014 Masatoshi Teruya
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 *  THE SOFTWARE.
 *
 *
 *  simplex.c
 *  lua-coevent
 *
 *  Created by Masatoshi Teruya on 14/05/22.
 *  Copyright 2014 Masatoshi Teruya. All rights reserved.
 *
 */

#include "handler.h"


static int alloc_lua( lua_State *L )
{
    loop_t *loop = luaL_checkudata( L, 1, COLOOP_MT );
    int fd = luaL_checkint( L, 2 );
    int trigger = 0;
    
    // check arguments
    // arg#2
    if( fd < 0 || fd > INT_MAX ){
        return luaL_argerror( L, 2, 
            "fd value range must be 0 to " MSTRCAT(INT_MAX) 
        );
    }
    // arg#3 edge-trigger (default level-trigger)
    else if( !lua_isnoneornil( L, 3 ) ){
        luaL_checktype( L, 3, LUA_TBOOLEAN );
        trigger = lua_toboolean( L, 3 );
    }

    return coevt_duplex( L, loop, fd, trigger );
}

LUALIB_API int luaopen_coevent_io( lua_State *L )
{
    // add methods
    lua_pushcfunction( L, alloc_lua );
    
    return 1;
}


