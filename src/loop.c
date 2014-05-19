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
 *  loop.c
 *  lua-coevent
 *
 *  Created by Masatoshi Teruya on 14/05/13.
 *  Copyright 2014 Masatoshi Teruya. All rights reserved.
 *
 */

#include "loop.h"
#include "sentry.h"


static int run_lua( lua_State *L )
{
    loop_t *loop = luaL_checkudata( L, 1, COLOOP_MT );
    
    if( loop->state == CORUN_ONCE || loop->state == CORUN_FOREVER ){
        errno = EALREADY;
    }
    else
    {
        // defaout timeout: 1 sec
        int ts = 1;
        sentry_t *s = NULL;
        sentry_refs_t *refs = NULL;
        coevt_t *evt = NULL;
        int nevt, i, narg, hup, rc;
        
        // check args
        // loop state (default once)
        loop->state = CORUN_ONCE;
        if( !lua_isnoneornil( L, 2 ) ){
            luaL_checktype( L, 2, LUA_TBOOLEAN );
            // set to RUNLOOP_FOREVER
            loop->state += lua_toboolean( L, 2 );
        }
        // timeout
        if( !lua_isnoneornil( L, 3 ) )
        {
            ts = luaL_checkint( L, 3 );
            if( ts < -1 ){
                ts = 1;
            }
        }
        
LOOP_CONTINUE:
        nevt = coevt_wait( loop, ts );
        
        // check errno
        if( nevt == -1 )
        {
            switch( errno ){
                // ignore
                case ENOENT:
                    errno = 0;
                break;
                
                case EACCES:
                case EFAULT:
                case EBADF:
                case EINTR:
                case ENOMEM:
                default:
                    goto LOOP_DONE;
            }
        }
        else
        {
            errno = 0;
            for( i = 0; i < nevt; i++ )
            {
                evt = &loop->evs[i];
                s = COEVT_UDATA( evt );
                refs = &s->refs;
                if( COREFS_IS_REFERENCED( refs ) )
                {
                    // drain event data
                    COREFS_DRAIN_DATA( s->ident, refs );
                    
                    hup = COEVT_IS_HUP( evt );
                    // push callback function
                    if( lua_status( refs->L ) != LUA_YIELD ){
                        lstate_pushref( refs->L, refs->fn );
                        lstate_pushref( refs->L, refs->ctx );
                        lstate_pushref( refs->L, s->ref );
                        lua_pushboolean( refs->L, hup );
                        narg = 3;
                    }
                    else {
                        narg = 0;
                    }
                    
                    // unregister hup/oneshot sentry
                    if( hup ){
                        sentry_unregister( L, s, evt );
                        COREFS_RELEASE_THREAD( L, refs );
                    }
                    else if( COREFS_IS_ONESHOT( refs ) ){
                        sentry_unregister( L, s, evt );
                    }
                    
                    // run on coroutine
                    rc = lua_resume( refs->L, narg );
                    switch( rc ){
                        case 0:
                        break;
                        case LUA_YIELD:
                            if( !COREFS_IS_REFERENCED( refs ) ){
                                loop_exception( 
                                    s, refs, -rc, 
                                    "could not suspend unregistered event"
                                );
                            }
                        break;
                        case LUA_ERRMEM:
                        case LUA_ERRERR:
                        case LUA_ERRSYNTAX:
                        case LUA_ERRRUN:
                            loop_exception( 
                                s, refs, -rc, lua_tostring( refs->L, -1 )
                            );
                            if( COREFS_IS_REFERENCED( refs ) )
                            {
                                // release coroutine
                                COREFS_RELEASE_THREAD( L, refs );
                                // create coroutine
                                refs->L = lua_newthread( L );
                                if( refs->L ){
                                    // retain coroutine
                                    refs->th = lstate_ref( L, -1 );
                                    lua_pop( loop->L, 1 );
                                }
                                else {
                                    loop_exception( 
                                        s, refs, errno, 
                                        "could not create coroutine"
                                    );
                                    // release sentry
                                    sentry_unregister( L, s, evt );
                                }
                            }
                        break;
                    }
                }
                // remove unregistered sentry
                else {
                    sentry_unregister( L, s, evt );
                    COREFS_RELEASE_THREAD( L, refs );
                }
            }
        }
        
        // check loop state
        switch( loop->state ){
            case CORUN_FOREVER:
                goto LOOP_CONTINUE;
            case CORUN_ONCE:
                if( loop->nreg > 0 ){
                    goto LOOP_CONTINUE;
                }
            break;
        }
    }
    
LOOP_DONE:
    if( errno == 0 ){
        return 0;
    }
    
    // got error
    lua_pushinteger( L, errno );
    
    return 1;
}


static int stop_lua( lua_State *L )
{
    loop_t *loop = luaL_checkudata( L, 1, COLOOP_MT );
    
    loop->state = CORUN_NONE;
    lua_pushboolean( L, 1 );
    
    return 1;
}


/* metamethods */
static int gc_lua( lua_State *L )
{
    loop_t *loop = lua_touserdata( L, 1 );
    
    lstate_unref( L, loop->ref_fn );
    
    if( loop->fd ){
        close( loop->fd );
    }
    pdealloc( loop->evs );
    
    return 0;
}


static int tostring_lua( lua_State *L )
{
    return tostring_mt( L, COLOOP_MT );
}


static int alloc_lua( lua_State *L )
{
    loop_t *loop = NULL;
    int nevtbuf = 128;
    
    // check arguments
    // arg#1 number of event buffer size
    if( !lua_isnoneornil( L, 1 ) )
    {
        lua_Integer nbuf = luaL_checkinteger( L, 1 );
        
        if( nbuf < 1 || nbuf > INT_MAX ){
            return luaL_error( 
                L, "event buffer value range must be 1 to %d", INT_MAX 
            );
        }
        nevtbuf = (int)nbuf;
    }
    // arg#2 callback function
    if( !lua_isnoneornil( L, 2 ) ){
        luaL_checktype( L, 2, LUA_TFUNCTION );
    }
    
    // create event descriptor and event buffer
    if( ( loop = lua_newuserdata( L, sizeof( loop_t ) ) ) &&
        ( loop->evs = pnalloc( (size_t)nevtbuf, coevt_t ) ) )
    {
        loop->fd = coevt_create();
        if( loop->fd != -1 ){
            lstate_setmetatable( L, COLOOP_MT );
            loop->L = L;
            loop->state = CORUN_NONE;
            loop->nevs = nevtbuf;
            loop->nreg = 0;
            // retain callback
            loop->ref_fn = lstate_ref( L, 2 );
            
            return 1;
        }
        pdealloc( loop->evs );
    }
    
    
    // got error
    lua_pushnil( L );
    lua_pushinteger( L, errno );
    
    return 2;
}


LUALIB_API int luaopen_coevent_loop( lua_State *L )
{
    struct luaL_Reg mmethod[] = {
        { "__gc", gc_lua },
        { "__tostring", tostring_lua },
        { NULL, NULL }
    };
    struct luaL_Reg method[] = {
        { "run", run_lua },
        { "stop", stop_lua },
        { NULL, NULL }
    };

    // define metatable
    define_mt( L, COLOOP_MT, mmethod, method );
    // add methods
    lua_pushcfunction( L, alloc_lua );

    return 1;
}
