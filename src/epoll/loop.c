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


static void loop_error( loop_t *loop, sentry_t *s, int err, const char *msg )
{
    if( loop->ref_fn < 0 ){
        pelog( "error %d: %s", err, msg );
    }
    else
    {
        // push callback function
        lstate_pushref( loop->L, loop->ref_fn );
        // push context and sentry
        lstate_pushref( loop->L, s->ref_ctx );
        lstate_pushref( loop->L, s->ref );
        // push error info table
        lua_createtable( loop->L, 0, 3 );
        lstate_num2tbl( loop->L, "errno", err );
        lstate_str2tbl( loop->L, "message", msg );
        
        // get debug info
        if( s->L )
        {
            lua_Debug ar;
            
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


static int run_lua( lua_State *L )
{
    loop_t *loop = luaL_checkudata( L, 1, COLOOP_MT );
    
    if( loop->state == RUNLOOP_ONCE || loop->state == RUNLOOP_FOREVER ){
        errno = EALREADY;
    }
    else
    {
        // defaout timeout: 1000 ms
        int ts = 1000;
        sentry_t *s = NULL;
        struct epoll_event *evt = NULL;
        int nevt, i, narg, hup, rc;
        
        // check args
        // loop state (default once)
        loop->state = RUNLOOP_ONCE;
        if( !lua_isnoneornil( L, 2 ) ){
            luaL_checktype( L, 2, LUA_TBOOLEAN );
            // set to RUNLOOP_FOREVER
            loop->state += lua_toboolean( L, 2 );
        }
        // timeout
        if( !lua_isnoneornil( L, 3 ) )
        {
            double timeout = luaL_checknumber( L, 3 );
            if( timeout >= 0 ){
                ts = (int)(timeout * 1000);
            }
        }
        
LOOP_CONTINUE:
        nevt = epoll_pwait( loop->fd, loop->evs, (int)loop->nreg, ts, NULL );
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
                s = (sentry_t*)evt->data.ptr;
                if( sentry_isregistered( s ) )
                {
                    // read event data
                    if( s->rsize ){
                        void *rdata = alloca( s->rsize );
                        read( s->fd, rdata, s->rsize );
                    }
                    
                    // check hup
                    hup = evt->events & (EPOLLRDHUP|EPOLLHUP);
                    // push callback function
                    if( lua_status( s->L ) != LUA_YIELD ){
                        lstate_pushref( s->L, s->ref_fn );
                        lstate_pushref( s->L, s->ref_ctx );
                        lstate_pushref( s->L, s->ref );
                        lua_pushboolean( s->L, hup );
                        narg = 3;
                    }
                    else {
                        narg = 0;
                    }
                    
                    // release sentry if oneshot
                    if( s->oneshot )
                    {
                        // to decrement number of registered
                        loop->nreg--;
                        // release sentry
                        lstate_unref( L, s->ref );
                        s->ref = LUA_NOREF;
                        if( s->data ){
                            close( s->fd );
                            s->fd = 0;
                        }
                    }
                    
                    // run on coroutine
                    rc = lua_resume( s->L, narg );
                    switch( rc ){
                        case 0:
                        break;
                        case LUA_YIELD:
                            if( !sentry_isregistered( s ) ){
                                loop_error( 
                                    loop, s, -rc, "could not yield oneshot event"
                                );
                            }
                        break;
                        case LUA_ERRMEM:
                        case LUA_ERRERR:
                        case LUA_ERRSYNTAX:
                        case LUA_ERRRUN:
                            loop_error( 
                                loop, s, -rc, lua_tostring( s->L, -1 )
                            );
                            if( sentry_isregistered( s ) )
                            {
                                // release coroutine
                                lstate_unref( loop->L, s->ref_th );
                                // create coroutine
                                s->L = lua_newthread( loop->L );
                                if( s->L ){
                                    // retain coroutine
                                    s->ref_th = lstate_ref( loop->L, -1 );
                                    lua_pop( loop->L, 1 );
                                }
                                else {
                                    loop_error( 
                                        loop, s, errno, 
                                        "could not create coroutine"
                                    );
                                    // release sentry
                                    sentry_unregister( s );
                                }
                            }
                        break;
                    }
                }
                // remove unregistered sentry
                else {
                    sentry_unregister( s );
                }
            }
        }
        
        // check loop state
        switch( loop->state ){
            case RUNLOOP_FOREVER:
                goto LOOP_CONTINUE;
            case RUNLOOP_ONCE:
                if( loop->nreg > 0 ){
                    goto LOOP_CONTINUE;
                }
            break;
        }
    }
    
LOOP_DONE:
    if( errno ){
        lua_pushboolean( L, 0 );
        lua_pushinteger( L, errno );
        return 2;
    }
    
    lua_pushboolean( L, 1 );
    
    return 1;
}


static int stop_lua( lua_State *L )
{
    loop_t *loop = luaL_checkudata( L, 1, COLOOP_MT );
    
    loop->state = RUNLOOP_NONE;
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
    tostring_mt( L, COLOOP_MT );
    return 1;
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
    
    // create event descriptor
    if( ( loop = lua_newuserdata( L, sizeof( loop_t ) ) ) && 
        ( loop->evs = pnalloc( (size_t)nevtbuf, struct epoll_event ) ) )
    {
#if HAVE_EPOLL_CREATE1
        if( ( loop->fd = epoll_create1( EPOLL_CLOEXEC ) ) != -1 )
#else
        if( ( loop->fd = epoll_create( nevtbuf ) ) != -1 )
#endif
        {
            luaL_getmetatable( L, COLOOP_MT );
            lua_setmetatable( L, -2 );
            loop->L = L;
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
