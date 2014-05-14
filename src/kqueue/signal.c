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
    
    if( SENTRY_IS_REGISTERED( s ) ){
        errno = EALREADY;
    }
    else
    {
        int *sigset = (int*)s->prop.ident;
        uint16_t flags = EV_ADD;
        struct kevent evt;
        int i = 0;
        
        if( lua_toboolean( L, 2 ) ){
            flags |= EV_ONESHOT;
        }
        
        // retain callback and usercontext
        s->ref_fn = lstate_ref( L, 3 );
        s->ref_ctx = lstate_ref( L, 4 );
        
        while( sigset[i] )
        {
            EV_SET( &evt, sigset[i], EVFILT_SIGNAL, flags, 0, 0, (void*)s );
            // register sentry
            if( sentry_register( L, s, &evt ) ){
                goto REGISTER_FAILURE;
            }
            i++;
        }
        
        lua_pushboolean( L, 1 );
        return 1;
        
REGISTER_FAILURE:
        while( --i <= 0 ){
            EV_SET( &evt, sigset[i], EVFILT_SIGNAL, EV_DELETE, 0, 0, NULL );
            // deregister sentry
            sentry_unregister( L, s, &evt );
        }
    }
    
    // got error
    lua_pushboolean( L, 0 );
    lua_pushnumber( L, errno );
    
    return 2;
}


static int unwatch_lua( lua_State *L )
{
    sentry_t *s = luaL_checkudata( L, 1, COSIGNAL_MT );
    
    if( SENTRY_IS_REGISTERED( s ) )
    {
        int rc = 0;
        int *sigset = (int*)s->prop.ident;
        struct kevent evt;
        
        while( *sigset )
        {
            EV_SET( &evt, *sigset, EVFILT_SIGNAL, EV_DELETE, 0, 0, NULL );
            // deregister sentry
            if( sentry_unregister( L, s, &evt ) != 0 ){
                rc = -1;
            }
            sigset++;
        }
        
        if( !rc ){
            lua_pushboolean( L, 1 );
            return 1;
        }
    }
    else {
        errno = ENOENT;
    }

    // got error
    lua_pushboolean( L, 0 );
    lua_pushnumber( L, errno );
    
    return 2;
}


static int tostring_lua( lua_State *L )
{
    return tostring_mt( L, COSIGNAL_MT );
}


static int alloc_lua( lua_State *L )
{
    int argc = lua_gettop( L );
    loop_t *loop = luaL_checkudata( L, 1, COLOOP_MT );
    int sigs[NSIG] = {0};
    int nsig = 0;
    int signo = 0;
    int *sigset = NULL;
    sigset_t ss;
    
    // error
    if( argc < SENTRY_INIT_ARG ){
        luaL_checktype( L, SENTRY_INIT_ARG, LUA_TNUMBER );
    }
    else if( argc > NSIG ){
        return luaL_error( L, "could not register more than %u signals", NSIG );
    }
    
    // clear
    sigemptyset( &ss );
    while( argc > 1 )
    {
        signo = (int)luaL_checkinteger( L, argc );
        // new signo
        if( !sigismember( &ss, signo ) )
        {
            if( sigaddset( &ss, signo ) != 0 ){
                return luaL_argerror( L, argc, "invalid signal number" );
            }
            sigs[nsig++] = signo;
        }
        argc--;
    }
    
    // allocate signal set
    if( ( sigset = pnalloc( (size_t)nsig + 1, int ) ) )
    {
        // allocate sentry
        sentry_t *s = sentry_alloc( L, loop, COSIGNAL_MT, (uintptr_t)sigset );
        
        if( s ){
            sigset[nsig] = 0;
            memcpy( (void*)sigset, &sigs, sizeof( int ) * (size_t)nsig );
            return 1;
        }
        
        pdealloc( sigset );
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

