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

#define COREFS_IS_REFERENCED(refs) ((refs)->fn > LUA_REFNIL)
#define COREFS_IS_ONESHOT(refs)    ((refs)->oneshot)

// release coroutine
#define COREFS_RELEASE_THREAD(L,refs) do { \
    lstate_unref( L, (refs)->th ); \
    (refs)->th = LUA_NOREF; \
}while(0)


static inline int sentry_refs_init( lua_State *L, sentry_refs_t *refs )
{
    // create coroutine
    refs->L = lua_newthread( L );
    if( refs->L ){
        // retain coroutine
        refs->th = lstate_ref( L, -1 );
        lua_pop( L, 1 );
        refs->oneshot = 0;
        refs->fn = refs->ctx = LUA_NOREF;
        
        return 0;
    }
    
    return -1;
}


static inline void sentry_refs_release( lua_State *L, sentry_refs_t *refs )
{
    lstate_unref( L, refs->fn );
    lstate_unref( L, refs->ctx );
    refs->fn = refs->ctx = LUA_NOREF;
}



int sentry_gc( lua_State *L );
int sentry_dealloc_gc( lua_State *L );

static inline sentry_t *sentry_alloc( lua_State *L, loop_t *loop,
                                       const char *tname )
{
    // allocate sentry data
    sentry_t *s = lua_newuserdata( L, sizeof( sentry_t ) );
    
    if( s ){
        s->ref = LUA_NOREF;
        s->ident = 0;
        s->loop = loop;
        // set metatable
        lstate_setmetatable( L, tname );
    }
    
    return s;
}


static inline void sentry_release( lua_State *L, sentry_t *s )
{
    lstate_unref( L, s->ref );
    s->ref = LUA_NOREF;
    sentry_refs_release( L, &s->refs );
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
        sentry_refs_release( L, &s->refs );
    }
    
    return rc;
}

static inline int sentry_unregister( lua_State *L, sentry_t *s, coevt_t *evt )
{
    int rc = loop_unregister( s->loop, s, evt );
    
    sentry_release( L, s );
    
    return rc;
}


#endif
