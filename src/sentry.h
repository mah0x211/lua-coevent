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


int sentry_gc( lua_State *L );
int sentry_dealloc_gc( lua_State *L );


static inline sentry_t *_sentry_alloc( lua_State *L, loop_t *loop, 
                                       const char *tname )
{
    // allocate sentry data
    sentry_t *s = lua_newuserdata( L, sizeof( sentry_t ) );
    
    if( s )
    {
        // create coroutine
        if( ( s->L = lua_newthread( L ) ) )
        {
            // retain coroutine
            s->ref_th = lstate_ref( L, -1 );
            lua_pop( L, 1 );
            // init
            s->loop = loop;
            s->ref = s->ref_fn = s->ref_ctx = LUA_NOREF;
            // set metatable
            luaL_getmetatable( L, tname );
            lua_setmetatable( L, -2 );
        }
        else {
            // remove userdata
            lua_pop( L, 1 );
            s = NULL;
        }
    }
    
    return s;
}

#define sentry_alloc(L,loop,tname, ... ) ({ \
    sentry_t *s = _sentry_alloc( L, loop, tname ); \
    if( s ){ \
        COEVT_PROP_INIT( &s->prop, __VA_ARGS__ ); \
    } \
    s; \
})


#define SENTRY_IS_REGISTERED(s) (s->ref > LUA_REFNIL)

static inline void sentry_release( lua_State *L, sentry_t *s )
{
    lstate_unref( L, s->ref );
    lstate_unref( L, s->ref_fn );
    lstate_unref( L, s->ref_ctx );
    s->ref = s->ref_fn = s->ref_ctx = LUA_NOREF;
    COEVT_PROP_CLEAR( &s->prop );
}

static inline int sentry_register( lua_State *L, sentry_t *s, coevt_t *evs )
{
    // register event
    int rc = loop_register( s->loop, s, evs );
    
    if( rc == 0 ){
        // retain sentry
        s->ref = lstate_ref( L, 1 );
    }
    else {
        // release
        lstate_unref( L, s->ref_fn );
        lstate_unref( L, s->ref_ctx );
    }
    
    return rc;
}

static inline int sentry_unregister( lua_State *L, sentry_t *s, coevt_t *evt )
{
    int rc = loop_unregister( s->loop, s, evt );
    
    if( rc == 0 ){
        sentry_release( L, s );
    }
    
    return rc;
}


#endif
