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
 *  sentry.h
 *  lua-coevent
 *
 *  Created by Masatoshi Teruya on 14/05/13.
 *  Copyright 2014 Masatoshi Teruya. All rights reserved.
 *
 */

#ifndef ___SENTRY_LUA___
#define ___SENTRY_LUA___

#include "loop.h"


#define SENTRY_INIT_ARG 2


sentry_t *sentry_alloc( lua_State *L, loop_t *loop, const char *tname, 
                        uintptr_t ident );

int sentry_gc( lua_State *L );
int sentry_dealloc_gc( lua_State *L );

#define sentry_isregistered(s)  (s->ref > LUA_REFNIL)

static inline int sentry_register( lua_State *L, sentry_t *s, 
                                   struct kevent *evs )
{
    // register event
    int rc = loop_register( s->loop, s, evs );
    
    if( rc == 0 ){
        // retain sentry
        s->ref = lstate_ref( L, 1 );
    }
    
    return rc;
}

static inline int sentry_unregister( lua_State *L, sentry_t *s, 
                                     struct kevent *evt )
{
    int rc = loop_unregister( s->loop, s, evt );
    
    if( rc == 0 ){
        // release sentry
        lstate_unref( L, s->ref );
        lstate_unref( L, s->ref_ctx );
        s->ref = s->ref_ctx = LUA_NOREF;
    }
    
    return rc;
}


static inline int sentry_rw_alloc( lua_State *L, const char *tname )
{
    loop_t *loop = luaL_checkudata( L, 1, COLOOP_MT );
    lua_Integer fd = luaL_checkinteger( L, 2 );
    sentry_t *s = NULL;
    
    // check argument
    if( fd < 0 || fd > INT_MAX ){
        return luaL_error( L, "fd value range must be 0 to %d", INT_MAX );
    }
    // allocate sentry
    else if( ( s = sentry_alloc( L, loop, tname, (uintptr_t)fd ) ) ){
        return 1;
    }
    
    // got error
    lua_pushnil( L );
    lua_pushinteger( L, errno );
    
    return 2;
}


static inline int sentry_rw_watch( lua_State *L, const char *tname, 
                                       int16_t filter )
{
    sentry_t *s = luaL_checkudata( L, 1, tname );
    
    // check arguments
    // arg#2 oneshot
    luaL_checktype( L, 2, LUA_TBOOLEAN );
    // arg#3 edge-trigger
    luaL_checktype( L, 3, LUA_TBOOLEAN );
    // arg#4 callback function
    luaL_checktype( L, 4, LUA_TFUNCTION );
    // arg#5 user-context
    
    if( sentry_isregistered( s ) ){
        errno = EALREADY;
    }
    else
    {
        struct kevent evt;
        uint16_t flags = EV_ADD;
        
        if( lua_toboolean( L, 2 ) ){
            flags |= EV_ONESHOT;
        }
        if( lua_toboolean( L, 3 ) ){
            flags |= EV_CLEAR;
        }
        
        // retain callback and usercontext
        s->ref_fn = lstate_ref( L, 4 );
        s->ref_ctx = lstate_ref( L, 5 );
        
        EV_SET( &evt, s->ident, filter, flags, 0, 0, (void*)s );
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


static inline int sentry_rw_unwatch( lua_State *L, const char *tname, 
                                     int16_t filter )
{
    sentry_t *s = luaL_checkudata( L, 1, tname );
    
    if( sentry_isregistered( s ) )
    {
        struct kevent evt;
        
        EV_SET( &evt, s->ident, filter, EV_DELETE, 0, 0, NULL );
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



#endif
