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
 *  Created by Masatoshi Teruya on 14/05/13.
 *  Copyright 2014 Masatoshi Teruya. All rights reserved.
 *
 */

#include "sentry.h"
#include <signal.h>


static int watch_lua( lua_State *L )
{
    sentry_t *s = luaL_checkudata( L, 1, COSIGNAL_MT );
    
    // check arguments
    // arg#2 oneshot
    luaL_checktype( L, 2, LUA_TBOOLEAN );
    // arg#3 callback function
    luaL_checktype( L, 3, LUA_TFUNCTION );
    // arg#4 user-context
    
    if( COREFS_IS_REFERENCED( &s->refs ) ){
        errno = EALREADY;
    }
    else
    {
        struct epoll_event evt;
        
        // retain callback and usercontext
        s->refs.fn = lstate_ref( L, 3 );
        s->refs.ctx = lstate_ref( L, 4 );
        s->refs.oneshot = lua_toboolean( L, 2 ) ? COEVT_FLG_ONESHOT : 0;
        
        evt.data.ptr = (void*)s;
        evt.events = EPOLLRDHUP|EPOLLIN|s->refs.oneshot;
        
        // register event
        if( sentry_register( L, s, &evt ) == 0 ){
            return 0;
        }
    }
    
    // got error
    lua_pushnumber( L, errno );
    
    return 1;
}


static int unwatch_lua( lua_State *L )
{
    sentry_t *s = luaL_checkudata( L, 1, COSIGNAL_MT );
    
    if( COREFS_IS_REFERENCED( &s->refs ) )
    {
        if( sentry_unregister( L, s, NULL ) != 0 ){
            // got error
            lua_pushnumber( L, errno );
            return 1;
        }
    }
    
    return 0;
}


static int tostring_lua( lua_State *L )
{
    tostring_mt( L, COSIGNAL_MT );
    return 1;
}


static int alloc_lua( lua_State *L )
{
    int argc = lua_gettop( L );
    loop_t *loop = luaL_checkudata( L, 1, COLOOP_MT );
    int signo = 0;
    int fd = 0;
    sigset_t ss;
    
    // error
    if( argc < 2 ){
        luaL_checktype( L, 2, LUA_TNUMBER );
    }
    
    // clear
    sigemptyset( &ss );
    while( argc > 1 )
    {
        signo = luaL_checkint( L, argc );
        // new signo
        if( !sigismember( &ss, signo ) )
        {
            if( sigaddset( &ss, signo ) != 0 ){
                return luaL_argerror( L, argc, "invalid signal number" );
            }
        }
        argc--;
    }
    
    // create signalfd with sigset_t
    fd = signalfd( -1, (sigset_t*)&ss, SFD_NONBLOCK|SFD_CLOEXEC );
    if( fd != -1 )
    {
        // allocate sentry
        sentry_t *s = sentry_alloc( L, loop, sentry_t, COSIGNAL_MT );
        
        if( s && sentry_refs_init( L, &s->refs ) == 0 ){
            s->ident = (coevt_ident_t)fd;
            COREFS_DRAIN_INIT( &s->refs, 0, COREFS_DRAIN_SIGNAL );
            return 1;
        }
        
        close( fd );
    }
    
    // got error
    lua_pushnil( L );
    lua_pushinteger( L, errno );
    
    return 2;
}


LUALIB_API int luaopen_coevent_signal( lua_State *L )
{
    struct luaL_Reg mmethod[] = {
        { "__gc", sentry_dealloc_gc },
        { "__tostring", tostring_lua },
        { NULL, NULL }
    };
    struct luaL_Reg method[] = {
        { "watch", watch_lua },
        { "unwatch", unwatch_lua },
        { NULL, NULL }
    };
    
    // define metatable
    define_mt( L, COSIGNAL_MT, mmethod, method );
    // add methods
    lua_pushcfunction( L, alloc_lua );
    
    return 1;
}

