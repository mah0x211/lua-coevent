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
 *  loop.h
 *  lua-coevent
 *
 *  Created by Masatoshi Teruya on 14/05/13.
 *  Copyright 2014 Masatoshi Teruya. All rights reserved.
 *
 */

#ifndef ___LOOP_LUA___
#define ___LOOP_LUA___

#include "coevent.h"

static inline int loop_increase( loop_t *loop, uint8_t incr )
{
    // no buffer
    if( ( INT_MAX - loop->nreg - incr ) <= 0 ){
        errno = ENOBUFS;
        return -1;
    }
    else if( ( loop->nreg + incr ) > loop->nevs )
    {
        // realloc event container
        kevt_t *evs = prealloc( (size_t)loop->nreg + incr, kevt_t, loop->evs );
        
        if( !evs ){
            return -1;
        }
        loop->nevs = loop->nreg + incr;
        loop->evs = evs;
    }
    
    return 0;
}

static inline int get_traceback( lua_State *L, lua_State *th, const char *msg )
{
    // get stack trace
#if LUA_VERSION_NUM >= 502
    luaL_traceback( L, th, msg, 1 );

#else
    // get debug module
    lua_getfield( th, LUA_GLOBALSINDEX, "debug" );
    if( lua_istable( th, -1 ) == 0 ){
        return 0;
    }
    lua_getfield( th, -1, "traceback" );
    if( lua_isfunction( th, -1 ) == 0 ){
        return 0;
    }
    lua_pushstring( th, msg );
    lua_call( th, 1, 1 );
    lua_xmove( th, L, 1 );
#endif

    return 1;
}

static inline void loop_exception( sentry_t *s, int err, const char *msg )
{
    loop_t *loop = s->loop;
    
    if( loop->ref_fn < 0 ){
        pelog( "error %d: %s", err, msg );
    }
    else
    {
        // push callback function
        lstate_pushref( loop->L, loop->ref_fn );
        // push context and sentry
        lstate_pushref( loop->L, s->ctx );
        lstate_pushref( loop->L, s->ref );
        // push error info table
        lua_createtable( loop->L, 0, 3 );
        lstate_num2tbl( loop->L, "errno", err );
        
        if( s->L )
        {
            lua_Debug ar;
            
            // get debug info
            if( lua_getstack( s->L, 1, &ar ) == 1 && 
                lua_getinfo( s->L, "nSl", &ar ) != 0 ){
                // debug info
                lua_pushstring( loop->L, "info" );
                lua_createtable( loop->L, 0, 5 );
                lstate_str2tbl( loop->L, "source", ar.source );
                lstate_str2tbl( loop->L, "what", ar.what );
                lstate_num2tbl( loop->L, "currentline", ar.currentline );
                lstate_str2tbl( loop->L, "name", ar.name );
                lstate_str2tbl( loop->L, "namewhat", ar.namewhat );
                lua_rawset( loop->L, -3 );
            }
            else {
                lstate_str2tbl( loop->L, "info", "could not get debug info" );
            }
            // set trackback
            if( get_traceback( loop->L, s->L, msg ) ){
                lua_setfield( loop->L, -2, "message" );
            }
            else {
                lstate_str2tbl( loop->L, "message", msg );
            }
        }
        
        // call error function
        switch( lua_pcall( loop->L, 3, 0, 0 ) ){
            case 0:
            break;
            // LUA_ERRRUN:
            // LUA_ERRMEM:
            // LUA_ERRERR:
            default:
                pelog( "error %d: %s (%s)", err, msg, 
                       lua_tostring( loop->L, -1 ) );
        }
    }
}



#endif
