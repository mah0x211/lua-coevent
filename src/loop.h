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
#include "handler.h"


static inline int loop_register( loop_t *loop, sentry_t *s, coevt_t *evt )
{
    int rc = -1;
    // expand receive events container
    // no buffer
    if( loop->nreg == INT_MAX ){
        errno = ENOBUFS;
        return rc;
    }
    else if( ( loop->nreg + 1 ) > loop->nevs )
    {
        // realloc event container
        coevt_t *evs = prealloc( (size_t)loop->nevs + 1, coevt_t, loop->evs );
        
        if( !evs ){
            return rc;
        }
        loop->nevs++;
        loop->evs = evs;
    }
    
    // register event
    rc = coevt_register( loop, s, evt );
    if( rc == 0 ){
        loop->nreg++;
    }
    
    return rc;
}


static inline int loop_unregister( loop_t *loop, sentry_t *s, coevt_t *evt )
{
    int rc = coevt_unregister( loop, s, evt );
    
    if( rc == 0 ){
        loop->nreg--;
    }
    
    return rc;
}


static inline void loop_exception( sentry_t *s, sentry_refs_t *refs, 
                                   int err, const char *msg )
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
        lstate_pushref( loop->L, refs->ctx );
        lstate_pushref( loop->L, s->ref );
        // push error info table
        lua_createtable( loop->L, 0, 3 );
        lstate_num2tbl( loop->L, "errno", err );
        lstate_str2tbl( loop->L, "message", msg );
        
        // get debug info
        if( refs->L )
        {
            lua_Debug ar;
            
            if( lua_getstack( refs->L, 1, &ar ) == 1 && 
                lua_getinfo( refs->L, "nSl", &ar ) != 0 ){
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
