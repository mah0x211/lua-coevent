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

int sentry_gc( lua_State *L );
int sentry_dealloc_gc( lua_State *L );


// allocate sentry data
static inline sentry_t *sentry_alloc( lua_State *L, loop_t *loop, int type )
{
    sentry_t *s = (sentry_t*)lua_newuserdata( L, sizeof( sentry_t ) );
    
    if( s )
    {
        switch( type ){
            case COSENTRY_T_SIGNAL:
                lstate_setmetatable( L, COSIGNAL_MT );
            break;
            case COSENTRY_T_TIMER:
                lstate_setmetatable( L, COTIMER_MT );
            break;
            case COSENTRY_T_READER:
                lstate_setmetatable( L, COREADER_MT );
            break;
            case COSENTRY_T_WRITER:
                lstate_setmetatable( L, COWRITER_MT );
            break;
            default:
                errno = EINVAL;
                lua_pop( L, 1 );
                return NULL;
        }
        
        s->loop = loop;
        s->type = type;
        s->L = NULL;
        s->ref = s->th = s->fn = s->ctx = LUA_NOREF;
    }
    
    return s;
}


static inline void sentry_retain( lua_State *L, sentry_t *s )
{
    if( !lstate_isref( s->ref ) ){
        s->ref = lstate_ref( L, 1 );
    }
}

static inline void sentry_release( lua_State *L, sentry_t *s )
{
    if( lstate_isref( s->ref ) ){
        lstate_unref( L, s->ref );
        s->ref = LUA_NOREF;
    }
}


#endif
