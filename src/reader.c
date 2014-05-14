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
 *  reader.c
 *  lua-coevent
 *
 *  Created by Masatoshi Teruya on 14/05/13.
 *  Copyright 2014 Masatoshi Teruya. All rights reserved.
 *
 */

#include "sentry.h"


static int watch_lua( lua_State *L )
{
    return sentry_rw_watch( L, COREADER_MT, COEVT_READ );
}


static int unwatch_lua( lua_State *L )
{
    return sentry_rw_unwatch( L, COREADER_MT, COEVT_READ );
}


static int tostring_lua( lua_State *L )
{
    return tostring_mt( L, COREADER_MT );
}


static int alloc_lua( lua_State *L )
{
    return sentry_rw_alloc( L, COREADER_MT );
}


LUALIB_API int luaopen_coevent_reader( lua_State *L )
{
    struct luaL_Reg mmethod[] = {
        { "__gc", sentry_gc },
        { "__tostring", tostring_lua },
        { NULL, NULL }
    };
    struct luaL_Reg method[] = {
        { "watch", watch_lua },
        { "unwatch", unwatch_lua },
        { NULL, NULL }
    };

    define_mt( L, COREADER_MT, mmethod, method );
    // add methods
    lua_pushcfunction( L, alloc_lua );
    
    return 1;
}



