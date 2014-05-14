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


#if HAVE_EPOLL_CREATE1
#define coevt_create()  epoll_create1(EPOLL_CLOEXEC)
#else
#define coevt_create()  epoll_create(1)
#endif


static inline int coevt_wait( loop_t *loop, int sec )
{
    if( sec > 0 ){
        sec *= 1000;
    }
    
    return epoll_pwait( loop->fd, loop->evs, (int)loop->nreg, sec, NULL );
}


static inline void coevt_rw_init( coevt_t *evt, sentry_t *s, int type, 
                                  uint8_t oneshot, uint8_t edge )
{
    evt->data.ptr = (void*)s;
    evt->events = type|EPOLLRDHUP;
    if( oneshot ){
        s->prop.oneshot = 1;
        evt->events |= EPOLLONESHOT;
    }
    // edge-trigger
    if( edge ){
        evt->events |= EPOLLET;
    }
}


static inline int coevt_register( loop_t *loop, sentry_t *s, coevt_t *evt )
{
    return epoll_ctl( loop->fd, EPOLL_CTL_ADD, s->prop.fd, evt );
}

static inline int coevt_unregister( loop_t *loop, sentry_t *s, coevt_t *evt )
{
    coevt_t _evt;
    
    return epoll_ctl( loop->fd, EPOLL_CTL_DEL, s->prop.fd, &_evt );
}


#endif
