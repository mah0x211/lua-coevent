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
 *  signal.c
 *  lua-coevent
 *
 *  Created by Masatoshi Teruya on 14/05/22.
 *  Copyright 2014 Masatoshi Teruya. All rights reserved.
 *
 */

#include "handler.h"


static int ident_lua( lua_State *L )
{
    sentry_t *s = luaL_checkudata( L, 1, COSIGNAL_MT );

    lua_pushinteger( L, s->evt.ident );
    
    return 1;
}


static int watch_lua( lua_State *L )
{
    sentry_t *s = luaL_checkudata( L, 1, COSIGNAL_MT );
    int oneshot = coevt_checkargs( L );
    
    return coevt_signal_watch( L, s, oneshot );
}


static int unwatch_lua( lua_State *L )
{
    sentry_t *s = luaL_checkudata( L, 1, COSIGNAL_MT );

    coevt_del( L, s );
    return 0;
}


static int tostring_lua( lua_State *L )
{
    return TOSTRING_MT( L, COSIGNAL_MT );
}


static int alloc_lua( lua_State *L )
{
    loop_t *loop = luaL_checkudata( L, 1, COLOOP_MT );
    int signo = luaL_checkint( L, 2 );
    sigset_t ss;
    
    // check arguments
    sigemptyset( &ss );
    if( sigaddset( &ss, signo ) != 0 ){
        return luaL_argerror( L, 2, "invalid signal number" );
    }
    
    return coevt_signal( L, loop, signo );
}


LUALIB_API int luaopen_coevent_signal( lua_State *L )
{
    struct luaL_Reg mmethod[] = {
        { "__gc", sentry_gc },
        { "__tostring", tostring_lua },
        { NULL, NULL }
    };
    struct luaL_Reg method[] = {
        { "ident", ident_lua },
        { "watch", watch_lua },
        { "unwatch", unwatch_lua },
        { NULL, NULL }
    };

    // define metatable
    coevt_define_mt( L, COSIGNAL_MT, mmethod, method );
    // add methods
    lua_pushcfunction( L, alloc_lua );
    
    return 1;
}

