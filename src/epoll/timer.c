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
 *  Created by Masatoshi Teruya on 14/04/02.
 *  Copyright 2014 Masatoshi Teruya. All rights reserved.
 *
 */

#include "sentry.h"


static int watch_lua( lua_State *L )
{
    sentry_t *s = luaL_checkudata( L, 1, COTIMER_MT );
    
    if( !COREFS_IS_REFERENCED( &s->refs ) )
    {
        struct timespec cur;
        struct itimerspec *ts = (struct itimerspec*)s->refs.data;
        
        // check arguments
        // arg#2 oneshot
        luaL_checktype( L, 2, LUA_TBOOLEAN );
        // arg#3 callback function
        luaL_checktype( L, 3, LUA_TFUNCTION );
        // arg#4 user-context
        
        // get current time
        clock_gettime( CLOCK_MONOTONIC, &cur );
        // set first invocation time
        ts->it_value = (struct timespec){ 
            .tv_sec = cur.tv_sec + ts->it_interval.tv_sec,
            .tv_nsec = cur.tv_nsec + ts->it_interval.tv_nsec
        };
        
        // set timespec
        if( timerfd_settime( (int)s->ident, TFD_TIMER_ABSTIME, ts, NULL ) == 0 )
        {
            struct epoll_event evt;
            
            // retain callback and usercontext
            s->refs.oneshot = lua_toboolean( L, 2 ) ? COEVT_FLG_ONESHOT : 0;
            s->refs.fn = lstate_ref( L, 3 );
            s->refs.ctx = lstate_ref( L, 4 );
            
            evt.data.ptr = (void*)s;
            evt.events = EPOLLRDHUP|EPOLLIN|s->refs.oneshot;

            // register event
            if( sentry_register( L, s, &s->refs, &evt ) == 0 ){
                return 0;
            }
        }
        
        // got error
        lua_pushnumber( L, errno );
        
        return 1;
    }
    
    return 0;
}


static int unwatch_lua( lua_State *L )
{
    sentry_t *s = luaL_checkudata( L, 1, COTIMER_MT );
    
    if( COREFS_IS_REFERENCED( &s->refs ) )
    {
        if( sentry_unregister( L, s, &s->refs, NULL ) != 0 ){
            // got error
            lua_pushnumber( L, errno );
            return 1;
        }
    }
    
    return 0;
}


static int tostring_lua( lua_State *L )
{
    tostring_mt( L, COTIMER_MT );
    return 1;
}


static int alloc_lua( lua_State *L )
{
    loop_t *loop = luaL_checkudata( L, 1, COLOOP_MT );
    double timeout = luaL_checknumber( L, 2 );
    // create timerfd
    int fd = timerfd_create( CLOCK_MONOTONIC, TFD_NONBLOCK|TFD_CLOEXEC );
    
    if( fd != -1 )
    {
        // allocate itimerspec
        struct itimerspec *ts = palloc( struct itimerspec );
        
        if( ts )
        {
            // allocate sentry
            sentry_t *s = sentry_alloc( L, loop, sentry_t, COTIMER_MT );
            
            if( s && sentry_refs_init( L, &s->refs ) == 0 ){
                s->ident = (coevt_ident_t)fd;
                COREFS_DRAIN_INIT( &s->refs, (uintptr_t)ts, COREFS_DRAIN_TIMER );
                ts->it_value = (struct timespec){ .tv_sec = 0, .tv_nsec = 0 };
                double2timespec( timeout, &ts->it_interval );
                return 1;
            }
            
            pdealloc( ts );
        }
        
        close( fd );
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
    
    // define metatable
    define_mt( L, COTIMER_MT, mmethod, method );
    // add methods
    lua_pushcfunction( L, alloc_lua );
    
    return 1;
}

