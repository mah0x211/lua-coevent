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
 *  sentry.c
 *  lua-coevent
 *
 *  Created by Masatoshi Teruya on 14/05/13.
 *  Copyright 2014 Masatoshi Teruya. All rights reserved.
 *
 */

#include "sentry.h"

sentry_t *sentry_alloc( lua_State *L, loop_t *loop, const char *tname, 
                        uintptr_t ident )
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
            s->ident = ident;
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


#define GET_SENTRY_AND_UNREF(L,s) do { \
    s = lua_touserdata( L, 1 ); \
    lstate_unref( L, s->ref_th ); \
    lstate_unref( L, s->ref_fn ); \
    lstate_unref( L, s->ref_ctx ); \
}while(0)

int sentry_gc( lua_State *L )
{
    sentry_t *s = NULL;
    
    GET_SENTRY_AND_UNREF( L, s );
    
    return 0;
}

int sentry_dealloc_gc( lua_State *L )
{
    sentry_t *s = NULL;
    
    GET_SENTRY_AND_UNREF( L, s );
    pdealloc( s->ident );
    
    return 0;
}

