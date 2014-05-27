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
 *  handler.h
 *  lua-coevent
 *
 *  Created by Masatoshi Teruya on 14/05/14.
 *  Copyright 2014 Masatoshi Teruya. All rights reserved.
 *
 */

#ifndef ___COEVENT_HANDLER___
#define ___COEVENT_HANDLER___

#include "coevent.h"
#include "sentry.h"


#define coevt_createfd()  kqueue()

static inline sentry_t *coevt_getsentry( loop_t *loop, kevt_t *evt )
{
    switch( evt->filter ){
        case EVFILT_TIMER:
            return (sentry_t*)evt->udata;
        break;
        case EVFILT_SIGNAL:
            if( sigismember( &loop->signals, evt->ident ) ){
                return (sentry_t*)evt->udata;
            }
        break;
        case EVFILT_READ:
            if( fdismember( &loop->fds, evt->ident, COSENTRY_T_READER ) == 1 ){
                return (sentry_t*)evt->udata;
            }
        break;
        case EVFILT_WRITE:
            if( fdismember( &loop->fds, evt->ident, COSENTRY_T_WRITER ) == 1 ){
                return (sentry_t*)evt->udata;
            }
        break;
    }
    
    return NULL;
}


// MARK: allocation/deallocation
static inline void coevt_dealloc( lua_State *L, sentry_t *s )
{
    if( s->type == COSENTRY_T_TIMER ){
        pdealloc( s->evt.ident );
    }
    coevt_release( L, s );
}


// MARK: event handler
#define coevt_drain(...)

static inline void coevt_loop_timeout( loop_t *loop, int timeout )
{
    loop->timeout = (struct timespec){ timeout, 0 };
}


static inline int coevt_wait( loop_t *loop )
{
    return kevent( loop->fd, NULL, 0, loop->evs, (int)loop->nreg, 
                   &loop->timeout );
}

static inline int coevt_add( lua_State *L, sentry_t *s, int oneshot )
{
    int rc = loop_increase( s->loop, 1 );
    
    if( rc == 0 )
    {
        s->evt.flags &= ~( EV_DELETE|EV_ONESHOT );
        s->evt.flags |= EV_ADD|( oneshot ? EV_ONESHOT : 0 );
        rc = kevent( s->loop->fd, &s->evt, 1, NULL, 0, NULL );
        if( rc == 0 ){
            coevt_retain( L, s );
            sentry_retain( L, s );
            s->loop->nreg++;
        }
    }
    
    return rc;
}

static inline void coevt_cleanup( lua_State *L, sentry_t *s )
{
    coevt_release( L, s );
    sentry_release( L, s );
    s->loop->nreg--;
    switch( s->type )
    {
        case COSENTRY_T_SIGNAL:
            sigdelset( &s->loop->signals, s->evt.ident );
        break;
        case COSENTRY_T_READER:
        case COSENTRY_T_WRITER:
            fddelset( &s->loop->fds, s->evt.ident, s->type );
        break;
    }
}

static inline int coevt_remove( loop_t *loop, kevt_t *evt )
{
    evt->flags &= ~( EV_ADD|EV_ONESHOT );
    evt->flags |= EV_DELETE;
    return kevent( loop->fd, evt, 1, NULL, 0, NULL );
}


static inline void coevt_del( lua_State *L, sentry_t *s )
{
    if( lstate_isref( s->ref ) ){
        coevt_remove( s->loop, &s->evt );
        coevt_cleanup( L, s );
    }
}


static inline void coevt_checkup( lua_State *L, sentry_t *s, coevt_t *evt )
{
    if( COEVT_IS_ONESHOT( &s->evt ) ){
        coevt_cleanup( L, s );
    }
    else if( COEVT_IS_HUP( evt ) ){
        coevt_del( L, s );
    }
}


// MARK: alloc/watch
static inline int coevt_timer( lua_State *L, loop_t *loop, double timeout )
{
    sentry_t *s = sentry_alloc( L, loop, COSENTRY_T_TIMER );
    // interval timespec
    struct timespec *ival = NULL;
    
    if( s && ( ival = palloc( struct timespec ) ) )
    {
        coevt_double2timespec( timeout, ival );
        EV_SET( &s->evt, (uintptr_t)ival, EVFILT_TIMER, 0, NOTE_NSECONDS, 
                ival->tv_sec * 1000000000 + ival->tv_nsec, (void*)s );
        return 1;
    }
    
    // got error
    lua_pushnil( L );
    lua_pushinteger( L, errno );
    
    return 2;
}

static inline int coevt_timer_watch( lua_State *L, sentry_t *s, int oneshot )
{
    // register sentry
    if( coevt_add( L, s, oneshot ) == 0 ){
        return 0;
    }
    
    // got error
    lua_pushnumber( L, errno );
    
    return 1;
}


static inline int coevt_signal( lua_State *L, loop_t *loop, int signo )
{
    sentry_t *s = sentry_alloc( L, loop, COSENTRY_T_SIGNAL );
    
    if( s ){
        EV_SET( &s->evt, (uintptr_t)signo, EVFILT_SIGNAL, 0, 0, 0, (void*)s );
        return 1;
    }
    
    // got error
    lua_pushnil( L );
    lua_pushinteger( L, errno );
    
    return 2;
}

static inline int coevt_signal_watch( lua_State *L, sentry_t *s, int oneshot )
{
    // set error
    if( sigismember( &s->loop->signals, s->evt.ident ) ){
        errno = EEXIST;
    }
    // register sentry
    else if( coevt_add( L, s, oneshot ) == 0 ){
        sigaddset( &s->loop->signals, s->evt.ident );
        return 0;
    }
    
    // got error
    lua_pushnumber( L, errno );
    
    return 1;
}


static inline int coevt_simplex( lua_State *L, loop_t *loop, int fd, int type, 
                                 int trigger )
{
    sentry_t *s = sentry_alloc( L, loop, type );
    
    if( s && fdset_realloc( &s->loop->fds, fd ) == 0 )
    {
        int16_t filter = 0;
        
        switch( type ){
            case COSENTRY_T_READER:
                filter = EVFILT_READ;
            break;
            case COSENTRY_T_WRITER:
                filter = EVFILT_WRITE;
            break;
        }
        
        EV_SET( &s->evt, (uintptr_t)fd, filter, trigger ? EV_CLEAR : 0, 0, 0, 
                (void*)s );
        
        return 1;
    }
    
    // got error
    lua_pushnil( L );
    lua_pushinteger( L, errno );
    
    return 2;
}

static inline int coevt_simplex_watch( lua_State *L, sentry_t *s, int oneshot )
{
    // set error
    if( fdismember( &s->loop->fds, s->evt.ident, s->type ) == 1 ){
        errno = EEXIST;
    }
    // register sentry
    else if( fdaddset( &s->loop->fds, s->evt.ident, s->type ) == 0 )
    {
        if( coevt_add( L, s, oneshot ) == 0 ){
            return 0;
        }
        fddelset( &s->loop->fds, s->evt.ident, s->type );
    }
    
    // got error
    lua_pushnumber( L, errno );
    
    return 1;
}



#endif

