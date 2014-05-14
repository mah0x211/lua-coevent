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

#if defined(HAVE_SYS_TIMERFD_H)
#include <sys/timerfd.h>
#endif

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
        // create timerfd
        int fd = timerfd_create( CLOCK_MONOTONIC, TFD_NONBLOCK|TFD_CLOEXEC );
        
        if( fd != -1 )
        {
            struct timespec cur;
            struct itimerspec *ts = (struct itimerspec*)s->prop.data;
            
            // get current time
            clock_gettime( CLOCK_MONOTONIC, &cur );
            // set first invocation time
            ts->it_value = (struct timespec){ 
                .tv_sec = cur.tv_sec + ts->it_interval.tv_sec,
                .tv_nsec = cur.tv_nsec + ts->it_interval.tv_nsec
            };
            
            // set timespec
            if( timerfd_settime( fd, TFD_TIMER_ABSTIME, ts, NULL ) == -1 ){
                close( fd );
            }
            else
            {
                struct epoll_event evt;
                
                evt.data.ptr = (void*)s;
                evt.events = EPOLLRDHUP|EPOLLIN;
                if( ( s->prop.oneshot = lua_toboolean( L, 2 ) ) ){
                    evt.events |= EPOLLONESHOT;
                }
                
                // retain callback and usercontext
                s->ref_fn = lstate_ref( L, 3 );
                s->ref_ctx = lstate_ref( L, 4 );
                s->prop.fd = fd;
                
                // register event
                if( sentry_register( L, s, &evt ) == 0 ){
                    lua_pushboolean( L, 1 );
                    return 1;
                }
                else {
                    s->prop.fd = 0;
                    close( fd );
                }
            }
        }
    }
    
    // got error
    lua_pushboolean( L, 0 );
    lua_pushnumber( L, errno );
    
    return 2;
}


static int unwatch_lua( lua_State *L )
{
    return sentry_unwatch( L, COTIMER_MT );
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
    // allocate itimerspec
    struct itimerspec *ts = palloc( struct itimerspec );
    
    if( ts )
    {
        // allocate sentry
        sentry_t *s = sentry_alloc( L, loop, COTIMER_MT, 0, (uintptr_t)ts, 
                                    sizeof( uint64_t ) );
        
        if( s ){
            ts->it_value = (struct timespec){ .tv_sec = 0, .tv_nsec = 0 };
            double2timespec( timeout, &ts->it_interval );
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
    
    // define metatable
    define_mt( L, COTIMER_MT, mmethod, method );
    // add methods
    lua_pushcfunction( L, alloc_lua );
    
    return 1;
}

