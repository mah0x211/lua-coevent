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
 *  timer.c
 *  lua-coevent
 *
 *  Created by Masatoshi Teruya on 14/05/13.
 *  Copyright 2014 Masatoshi Teruya. All rights reserved.
 *
 */

#include "sentry.h"


static int watch_lua( lua_State *L )
{
    sentry_t *s = luaL_checkudata( L, 1, COTIMER_MT );
    
    // check arguments
    // arg#2 oneshot
    luaL_checktype( L, 2, LUA_TBOOLEAN );
    // arg#3 callback function
    luaL_checktype( L, 3, LUA_TFUNCTION );
    // arg#4 user-context
    
    if( sentry_isregistered( s ) ){
        errno = EALREADY;
    }
    else
    {
        struct kevent evt;
        struct timespec *ts = (struct timespec*)s->prop.ident;
        uint16_t flags = EV_ADD;
        
        if( lua_toboolean( L, 2 ) ){
            flags |= EV_ONESHOT;
        }
        
        // retain callback and usercontext
        s->ref_fn = lstate_ref( L, 3 );
        s->ref_ctx = lstate_ref( L, 4 );
        
        EV_SET( &evt, s->prop.ident, EVFILT_TIMER, flags, NOTE_NSECONDS, 
                ts->tv_sec * 1000000000 + ts->tv_nsec, (void*)s );
        
        // register sentry
        if( sentry_register( L, s, &evt ) == 0 ){
            lua_pushboolean( L, 1 );
            return 1;
        }
    }
    
    // got error
    lua_pushboolean( L, 0 );
    lua_pushnumber( L, errno );
    
    return 2;
}


static int unwatch_lua( lua_State *L )
{
    sentry_t *s = luaL_checkudata( L, 1, COTIMER_MT );
    
    if( sentry_isregistered( s ) )
    {
        struct kevent evt;
        
        EV_SET( &evt, s->prop.ident, EVFILT_TIMER, EV_DELETE, 0, 0, NULL );
        // deregister sentry
        if( sentry_unregister( L, s, &evt ) == 0 ){
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
    return tostring_mt( L, COTIMER_MT );
}


static int alloc_lua( lua_State *L )
{
    loop_t *loop = luaL_checkudata( L, 1, COLOOP_MT );
    double timeout = luaL_checknumber( L, 2 );
    // allocate timespec
    struct timespec *ts = palloc( struct timespec );
    
    if( ts )
    {
        // allocate sentry
        sentry_t *s = sentry_alloc( L, loop, COTIMER_MT, (uintptr_t)ts );
        if( s ){
            double2timespec( timeout, ts );
            return 1;
        }
        
        pdealloc( ts );
    }
    
    // got error
    lua_pushnil( L );
    lua_pushinteger( L, errno );
    
    return 2;
}


LUALIB_API int luaopen_coevent_timer( lua_State *L )
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
    
    define_mt( L, COTIMER_MT, mmethod, method );
    // add methods
    lua_pushcfunction( L, alloc_lua );
    
    return 1;
}

