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

#include <sys/event.h>


typedef struct kevent       coevt_t;
typedef uintptr_t           coevt_ident_t;

// event types
#define COEVT_READ          EVFILT_READ
#define COEVT_WRITE         EVFILT_WRITE
typedef int16_t             coevt_type_t;

// event flags
#define COEVT_FLG_ONESHOT   EV_ONESHOT
#define COEVT_FLG_EDGE      EV_CLEAR
typedef uint16_t            coevt_flag_t;


#define COEVT_UDATA(evt)    ((sentry_t*)(evt)->udata)
#define COEVT_IS_HUP(evt)   ((evt)->flags & EV_EOF)
#define COEVT_IS_READ(evt)  ((evt)->filter == EVFILT_READ)
#define COEVT_IS_WRITE(evt) ((evt)->filter == EVFILT_WRITE)


#define SENTRY_FREE(s) do { \
    pdealloc( (s)->ident ); \
}while(0)


// do nothing
#define COREFS_DRAIN_DEFS()
#define COREFS_DRAIN_INIT(...)
#define COREFS_DRAIN_DATA(...)


#endif
