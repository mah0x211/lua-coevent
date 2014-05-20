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
    
    if( !COREFS_IS_REFERENCED( &s->refs ) )
    {
        struct kevent evt;
        struct timespec *ts = (struct timespec*)s->ident;
        
        // retain arguments
        sentry_retain_refs( L, s );
        
        EV_SET( &evt, s->ident, EVFILT_TIMER, s->refs.oneshot, 
                NOTE_NSECONDS, ts->tv_sec * 1000000000 + ts->tv_nsec, (void*)s);
        
        // register sentry
        if( sentry_register( s, &evt ) != 0 ){
            // got error
            sentry_release_refs( L, s );
            lua_pushnumber( L, errno );
            return 1;
        }
    }
    
    return 0;
}


static int unwatch_lua( lua_State *L )
{
    sentry_t *s = luaL_checkudata( L, 1, COTIMER_MT );
    
    if( COREFS_IS_REFERENCED( &s->refs ) )
    {
        struct kevent evt;
        
        // release references
        sentry_release_refs( L, s );
        
        EV_SET( &evt, s->ident, EVFILT_TIMER, 0, 0, 0, NULL );
        if( sentry_unregister( s, &evt ) != 0 ){
            // got error
            lua_pushnumber( L, errno );
            return 1;
        }
    }
    
    return 0;
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
        sentry_t *s = sentry_alloc( L, loop, sentry_t, COTIMER_MT );
        
        if( s && sentry_refs_init( L, &s->refs ) == 0 ){
            s->ident = (coevt_ident_t)ts;
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

