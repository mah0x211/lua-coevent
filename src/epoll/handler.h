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

#include "sentry.h"


#if HAVE_EPOLL_CREATE1
#define coevt_createfd()  epoll_create1(EPOLL_CLOEXEC)
#else
#define coevt_createfd()  epoll_create(1)
#endif


static inline sentry_t *coevt_getsentry( loop_t *loop, kevt_t *kevt )
{
    sentry_t *s = (sentry_t*)fdismember( &loop->fds, kevt->data.fd );
    
    if( s && !( kevt->events & s->type ) ){
        s = (sentry_t*)s->evt.sibling;
    }
    
    return s;
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
static inline void coevt_drain( sentry_t *s )
{
    static uint8_t drain_storage[sizeof( struct signalfd_siginfo )];
    
    switch( s->type ){
        case COSENTRY_T_SIGNAL:
            read( s->evt.ev.data.fd, drain_storage, sizeof( struct signalfd_siginfo ) );
        break;
        case COSENTRY_T_TIMER:
            read( s->evt.ev.data.fd, drain_storage, sizeof( uint64_t ) );
        break;
    }
}


static inline int coevt_wait( loop_t *loop, int sec )
{
    if( sec > 0 ){
        sec *= 1000;
    }
    
    return epoll_pwait( loop->fd, loop->evs, (int)loop->nreg, sec, NULL );
}


static inline int coevt_add( lua_State *L, sentry_t *s, int oneshot )
{
    int rc = loop_increase( s->loop, 1 );
    
    if( rc == 0 )
    {
        if( oneshot ){
            s->evt.flags |= EPOLLONESHOT;
        }
        else {
            s->evt.flags &= ~EPOLLONESHOT;
        }

        rc = epoll_ctl( s->loop->fd, EPOLL_CTL_ADD, s->evt.ev.data.fd, &s->evt.ev );
        if( rc == 0 ){
            coevt_retain( L, s );
            sentry_retain( L, s );
            s->loop->nreg++;
        }
    }
    
    return rc;
}


static inline int coevt_madd( lua_State *L, sentry_t *s, int oneshot )
{
    int rc = 0;
    kevt_t kevt = s->evt.ev;
    
    if( oneshot ){
        s->evt.flags |= EPOLLONESHOT;
    }
    else {
        s->evt.flags &= ~EPOLLONESHOT;
    }
    
    kevt.events |= EPOLLIN|EPOLLOUT;
    rc = epoll_ctl( s->loop->fd, EPOLL_CTL_MOD, s->evt.ev.data.fd, &kevt );
    if( rc == 0 ){
        coevt_retain( L, s );
        sentry_retain( L, s );
    }
    
    return rc;
}


static inline void coevt_cleanup( lua_State *L, sentry_t *s )
{
    kevt_t *evt = &s->evt.ev;
    int decr = 1;
    int op = EPOLL_CTL_DEL;
    
    switch( s->type )
    {
        case COSENTRY_T_SIGNAL:
            sigdelset( &s->loop->signals, s->evt.ident );
        case COSENTRY_T_TIMER:
            close( s->evt.ev.data.fd );
            fddelset( &s->loop->fds, evt->data.fd );
        break;
        case COSENTRY_T_READER:
        case COSENTRY_T_WRITER:
            if( s->evt.sibling ){
                sentry_t *sibling = (sentry_t*)s->evt.sibling;
                
                decr = 0;
                op = EPOLL_CTL_MOD;
                evt = &sibling->evt.ev;
                s->evt.sibling = sibling->evt.sibling = NULL;
                fdaddset( &s->loop->fds, evt->data.fd, (void*)sibling );
            }
            else {
                fddelset( &s->loop->fds, evt->data.fd );
            }
        break;
    }
    
    epoll_ctl( s->loop->fd, op, evt->data.fd, evt );
    s->evt.ev.data.fd = -1;
    s->loop->nreg -= decr;
    coevt_release( L, s );
    sentry_release( L, s );
}


static inline void coevt_del( lua_State *L, sentry_t *s )
{
    if( lstate_isref( s->ref ) ){
        coevt_cleanup( L, s );
    }
}


static inline int coevt_remove( loop_t *loop, kevt_t *evt )
{
    return epoll_ctl( loop->fd, EPOLL_CTL_DEL, evt->data.fd, evt );
}


static inline void coevt_checkup( lua_State *L, sentry_t *s, kevt_t *evt )
{
    if( COEVT_IS_ONESHOT( &s->evt ) || COEVT_IS_HUP( evt ) ){
        coevt_cleanup( L, s );
    }
}


// MARK: alloc/watch
static inline int coevt_timer( lua_State *L, loop_t *loop, double timeout )
{
    sentry_t *s = sentry_alloc( L, loop, COSENTRY_T_TIMER );
    struct timespec *ival = NULL;
    
    if( s )
    {
        if( ( ival = palloc( struct timespec ) ) ){
            s->evt.ident = (uintptr_t)ival;
            s->evt.flags = 0;
            s->evt.sibling = NULL;
            s->evt.ev.data.fd = -1;
            s->evt.ev.events = EPOLLRDHUP|EPOLLIN;
            coevt_double2timespec( timeout, ival );
            return 1;
        }
    }
    
    // got error
    lua_pushnil( L );
    lua_pushinteger( L, errno );
    
    return 2;
}

static inline int coevt_timer_watch( lua_State *L, sentry_t *s, int oneshot )
{
    if( s->evt.ev.data.fd != -1 ){
        errno = EEXIST;
    }
    else
    {
        int fd = timerfd_create( CLOCK_MONOTONIC, TFD_NONBLOCK|TFD_CLOEXEC );
        
        if( fd != -1 )
        {
            s->evt.ev.data.fd = fd;
            if( fdset_realloc( &s->loop->fds, fd ) == 0 )
            {
                struct timespec *interval = (struct timespec*)s->evt.ident;
                struct timespec now;
                struct itimerspec spec;
                
                // get current time
                clock_gettime( CLOCK_MONOTONIC, &now );
                // set first invocation time
                spec.it_interval = *interval;
                spec.it_value = (struct timespec){
                    .tv_sec = now.tv_sec + interval->tv_sec,
                    .tv_nsec = now.tv_nsec + interval->tv_nsec
                };
                
                // set timespec and register sentry
                if( timerfd_settime( fd, TFD_TIMER_ABSTIME, &spec, NULL ) == 0 &&
                    coevt_add( L, s, oneshot ) == 0 ){
                    fdaddset( &s->loop->fds, fd, (void*)s );
                    return 0;
                }
            }
            
            s->evt.ev.data.fd = -1;
            close( fd );
        }
    }
    
    // got error
    lua_pushnumber( L, errno );
    
    return 1;
}



static inline int coevt_signal( lua_State *L, loop_t *loop, int signo )
{
    sentry_t *s = sentry_alloc( L, loop, COSENTRY_T_SIGNAL );

    if( s ){
        s->evt.ident = signo;
        s->evt.flags = 0;
        s->evt.sibling = NULL;
        s->evt.ev.data.fd = -1;
        s->evt.ev.events = EPOLLRDHUP|EPOLLIN;
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
    if( s->evt.ev.data.fd != -1 ){
        errno = EEXIST;
    }
    else
    {
        int fd = 0;
        sigset_t ss;
        
        sigemptyset( &ss );
        sigaddset( &ss, s->evt.ident );
        
        // create signalfd with sigset_t
        fd = signalfd( -1, &ss, SFD_NONBLOCK|SFD_CLOEXEC );
        if( fd != -1 )
        {
            s->evt.ev.data.fd = fd;
            if( fdset_realloc( &s->loop->fds, fd ) == 0 && 
                coevt_add( L, s, oneshot ) == 0 ){
                fdaddset( &s->loop->fds, fd, (void*)s );
                sigaddset( &s->loop->signals, s->evt.ident );
                return 0;
            }
            
            s->evt.ev.data.fd = -1;
            close( fd );
        }
    }
    
    // got error
    lua_pushinteger( L, errno );
    
    return 1;
}


static inline int coevt_simplex( lua_State *L, loop_t *loop, int fd, int type, 
                                 int trigger )
{
    sentry_t *s = sentry_alloc( L, loop, type );

    if( s )
    {
        uint32_t events = EPOLLRDHUP|(( trigger ) ? EPOLLET : 0);
        
        switch( type )
        {
            case COSENTRY_T_READER:
                events |= EPOLLIN;
            break;
            case COSENTRY_T_WRITER:
                events |= EPOLLOUT;
            break;
            default:
                errno = EINVAL;
                goto FAILED;
        }
        
        s->evt.ident = fd;
        s->evt.flags = 0;
        s->evt.sibling = NULL;
        s->evt.ev.data.fd = -1;
        s->evt.ev.events = events;
        
        return 1;
    }

FAILED:
    // got error
    lua_pushnil( L );
    lua_pushinteger( L, errno );
    
    return 2;
}


static inline int coevt_simplex_watch( lua_State *L, sentry_t *s, int oneshot )
{
    if( s->evt.ev.data.fd != -1 ){
        errno = EEXIST;
    }
    else if( fdset_realloc( &s->loop->fds, s->evt.ident ) == 0 )
    {
        sentry_t *sreg = fdismember( &s->loop->fds, s->evt.ident );
        
        s->evt.ev.data.fd = s->evt.ident;
        
        if( sreg )
        {
            if( sreg->evt.ev.events & s->type ){
                errno = EEXIST;
            }
            // register sentry
            else if( coevt_madd( L, s, oneshot ) == 0 ){
                sreg->evt.sibling = s;
                s->evt.sibling = sreg;
                return 0;
            }
        }
        // register sentry
        else if( coevt_add( L, s, oneshot ) == 0 ){
            fdaddset( &s->loop->fds, s->evt.ident, (void*)s );
            return 0;
        }
        
        s->evt.ev.data.fd = -1;
    }
    
    // got error
    lua_pushnumber( L, errno );
    
    return 1;
}


#endif
