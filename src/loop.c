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

#include "handler.h"

// define loop types
#define CORUN_NONE       0
#define CORUN_ONCE       1
#define CORUN_FOREVER    2

static int runloop( lua_State *L, loop_t *loop )
{
    lua_State *th = NULL;
    sentry_t *s = NULL;
    kevt_t *kevt = NULL;
    int nevt, i, narg, rc;
    
LOOP_CONTINUE:
    nevt = coevt_wait( loop );
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
    
    for( i = 0; i < nevt; i++ )
    {
        kevt = &loop->evs[i];
        s = coevt_getsentry( loop, kevt );
        if( !s ){
            coevt_remove( loop, kevt );
        }
        // create coroutine
        else if( !s->L && coevt_thread_alloc( L, s ) != 0 ){
            loop_exception( s, errno, lua_tostring( L, -1 ) );
        }
        else
        {
            th = s->L;
            // drain event data
            coevt_drain( s );
            
            // push callback function
            if( lua_status( th ) != LUA_YIELD ){
                lstate_pushref( th, s->fn );
                lstate_pushref( th, s->ctx );
                lstate_pushref( th, s->ref );
                lua_pushboolean( th, COEVT_IS_HUP( kevt ) );
                narg = 3;
            }
            else {
                narg = 0;
            }
            
            coevt_checkup( L, s, kevt );
            
            // run on coroutine
            rc = lua_resume( th, narg );
            switch( rc ){
                case 0:
                break;
                case LUA_YIELD:
                    if( !lstate_isref( s->ref ) ){
                        loop_exception( s, -rc, 
                            "could not suspend unregistered event"
                        );
                    }
                break;
                case LUA_ERRMEM:
                case LUA_ERRERR:
                case LUA_ERRSYNTAX:
                case LUA_ERRRUN:
                    loop_exception( s, -rc, lua_tostring( th, -1 ) );
                    coevt_thread_release( L, s );
                break;
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

LOOP_DONE:
    if( errno == 0 ){
        return 0;
    }
    
    // got error
    lua_pushinteger( L, errno );
    
    return 1;

}

static int run_lua( lua_State *L )
{
    loop_t *loop = luaL_checkudata( L, 1, COLOOP_MT );
    
    // set error
    if( loop->state != CORUN_ONCE || loop->state != CORUN_FOREVER )
    {
        // defaout timeout: 1 sec
        int timeout = 1;
        
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
            timeout = luaL_checkint( L, 3 );
            if( timeout < -1 ){
                timeout = 1;
            }
        }
        
        coevt_loop_timeout( loop, timeout );
        
        return runloop( L, loop );
    }

    errno = EALREADY;
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


static int size_lua( lua_State *L )
{
    loop_t *loop = luaL_checkudata( L, 1, COLOOP_MT );
    
    lua_pushinteger( L, loop->nevs );
    
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

static int len_lua( lua_State *L )
{
    loop_t *loop = luaL_checkudata( L, 1, COLOOP_MT );
    
    lua_pushinteger( L, loop->nreg );
    
    return 1;
}


static int tostring_lua( lua_State *L )
{
    return TOSTRING_MT( L, COLOOP_MT );
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
        ( loop->evs = pnalloc( (size_t)nevtbuf, kevt_t ) ) )
    {
        if( fdset_alloc( &loop->fds, (size_t)nevtbuf ) == 0 )
        {
            loop->fd = coevt_createfd();
            if( loop->fd != -1 ){
                lstate_setmetatable( L, COLOOP_MT );
                loop->L = L;
                loop->state = CORUN_NONE;
                loop->nevs = nevtbuf;
                loop->nreg = 0;
                // retain callback
                loop->ref_fn = lstate_ref( L, 2 );
                sigemptyset( &loop->signals );
                
                return 1;
            }
            
            fdset_dealloc( &loop->fds );
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
        { "__len", len_lua },
        { NULL, NULL }
    };
    struct luaL_Reg method[] = {
        { "run", run_lua },
        { "stop", stop_lua },
        { "size", size_lua },
        { NULL, NULL }
    };

    // define metatable
    coevt_define_mt( L, COLOOP_MT, mmethod, method );
    // add methods
    lua_pushcfunction( L, alloc_lua );

    return 1;
}
