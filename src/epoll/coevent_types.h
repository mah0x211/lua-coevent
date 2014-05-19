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
 *  types.h
 *  lua-coevent
 *
 *  Created by Masatoshi Teruya on 14/05/14.
 *  Copyright 2014 Masatoshi Teruya. All rights reserved.
 *
 */

#ifndef ___COEVENT_TYPES___
#define ___COEVENT_TYPES___

#include <sys/epoll.h>
#include <sys/signalfd.h>
#include <sys/timerfd.h>


// event types
#define COEVT_READ      EPOLLIN
#define COEVT_WRITE     EPOLLOUT

typedef struct epoll_event  coevt_t;
typedef int                 coevt_ident_t;

#define COEVT_UDATA(evt)    ((sentry_t*)(evt)->data.ptr)
#define COEVT_IS_HUP(evt)   ((evt)->events & (EPOLLRDHUP|EPOLLHUP))

#define SENTRY_FREE(s) do { \
    close( (s)->ident ); \
    if( (s)->refs.data ){ \
        pdealloc( (s)->refs.data ); \
    } \
}while(0)


// drain types
#define COREFS_DRAIN_SIGNAL     1
#define COREFS_DRAIN_TIMER      2

#define COREFS_DRAIN_DEFS() \
    /* drain type */ \
    int drain; \
    uintptr_t data


#define COREFS_DRAIN_INIT(refs,_data,_drain) do { \
    (refs)->drain = (_drain); \
    (refs)->data = (_data); \
}while(0)


static uint8_t _drain_storage[sizeof( struct signalfd_siginfo )];

#define COREFS_DRAIN_DATA(ident,refs) do{ \
    switch( (refs)->drain ){ \
        case COREFS_DRAIN_SIGNAL: \
            read((ident), _drain_storage, sizeof(struct signalfd_siginfo)); \
        break; \
        case COREFS_DRAIN_TIMER: \
            read((ident), _drain_storage, sizeof(uint64_t)); \
        break; \
    } \
}while(0)


#endif
