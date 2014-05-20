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

#define COREFS_IS_REFERENCED(refs) (lstate_isref((refs)->fn))
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


static inline void sentry_refs_retain( lua_State *L, sentry_refs_t *refs )
{
    // check arguments
    // arg#2 oneshot
    luaL_checktype( L, 2, LUA_TBOOLEAN );
    // arg#3 callback function
    luaL_checktype( L, 3, LUA_TFUNCTION );
    // arg#4 user-context
    
    // retain callback and usercontext
    refs->oneshot = lua_toboolean( L, 2 ) ? COEVT_FLG_ONESHOT : 0;
    refs->fn = lstate_ref( L, 3 );
    refs->ctx = lstate_ref( L, 4 );
}


static inline void sentry_refs_release( lua_State *L, sentry_refs_t *refs )
{
    lstate_unref( L, refs->fn );
    lstate_unref( L, refs->ctx );
    refs->fn = refs->ctx = LUA_NOREF;
}


int sentry_gc( lua_State *L );
int sentry_dealloc_gc( lua_State *L );


// allocate sentry data
#define sentry_alloc(L,loop,t,tname)({ \
    t *_s = (t*)lua_newuserdata( L, sizeof(t) ); \
    if(_s){ \
        _s->type = sizeof(t); \
        _s->ref = LUA_NOREF; \
        _s->trigger = 0; \
        _s->ident = 0; \
        _s->loop = loop; \
        lstate_setmetatable( L, tname ); \
    } \
    _s; \
})


static inline void sentry_retain( lua_State *L, sentry_t *s )
{
    s->ref = lstate_ref( L, 1 );
}

static inline void sentry_retain_refs( lua_State *L, sentry_t *s )
{
    // NOTE: should call sentry_refs_retain at first because this function 
    //       would raise an exception error if illegal arguments passed.
    sentry_refs_retain( L, &s->refs );
    sentry_retain( L, s );
}


static inline void sentry_release( lua_State *L, sentry_t *s )
{
    lstate_unref( L, s->ref );
    s->ref = LUA_NOREF;
}

static inline void sentry_release_refs( lua_State *L, sentry_t *s )
{
    sentry_refs_release( L, &s->refs );
    sentry_release( L, s );
}

static inline void sentry_dispose( lua_State *L, sentry_t *s )
{
    sentry_release_refs( L, s );
    COREFS_RELEASE_THREAD( L, &s->refs );
}


#define sentry_register(s,evs) \
    loop_register((s)->loop, s, evs)

#define sentry_unregister(s,evt) \
    loop_unregister((s)->loop, s, evt)



#endif
