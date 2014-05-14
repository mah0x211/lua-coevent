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


// event types
#define COEVT_READ      EPOLLIN
#define COEVT_WRITE     EPOLLOUT


typedef struct epoll_event  coevt_t;

#define COEVT_UDATA(evt)        ((sentry_t*)(evt)->data.ptr)
#define COEVT_IS_ONESHOT(evt)   ((COEVT_UDATA(evt))->prop.oneshot)
#define COEVT_IS_HUP(evt)       ((evt)->events & (EPOLLRDHUP|EPOLLHUP))


// drain types
#define PROP_DRAIN_SIGNAL   1
#define PROP_DRAIN_TIMER    2

typedef struct {
    // target fd for epoll
    int fd;
    // 1 = oneshot event
    int oneshot;
    // pointer that allocated data by some of sentry type.
    // this pointer must deallocate at gc.
    // this will be 0 if sentry used external fd. (e.g. reader/writer)
    uintptr_t data;
    // drain type
    uint8_t drain;
} coevt_prop_t;


#define COEVT_PROP_INIT(p,_fd,_data,_drain) do { \
    (p)->fd = (_fd); \
    (p)->oneshot = 0; \
    (p)->data = (_data); \
    (p)->drain = (_drain); \
}while(0)


static uint8_t _drain_storage[sizeof( struct signalfd_siginfo )];

#define COEVT_PROP_DRAIN(p) do{ \
    switch( (p)->drain ){ \
        case PROP_DRAIN_SIGNAL: \
            read( (p)->fd, _drain_storage, sizeof( struct signalfd_siginfo ) ); \
        break; \
        case PROP_DRAIN_TIMER: \
            read( (p)->fd, _drain_storage, sizeof( uint64_t ) ); \
        break; \
    } \
}while(0)


#define COEVT_PROP_CLEAR(p) do { \
    if( (p)->fd && (p)->data ){ \
        close( (p)->fd ); \
        (p)->fd = 0; \
    } \
}while(0)


#define COEVT_PROP_FREE(p) do { \
    COEVT_PROP_CLEAR(p); \
    pdealloc( (p)->data ); \
}while(0)


#endif
