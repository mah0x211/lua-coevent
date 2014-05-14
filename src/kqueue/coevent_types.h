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

// event types
#define COEVT_READ      EVFILT_READ
#define COEVT_WRITE     EVFILT_WRITE


typedef struct kevent   coevt_t;

typedef struct {
    uintptr_t ident;
} coevt_prop_t;


#define COEVT_PROP_INIT(p,_ident,...) do { \
    (p)->ident = (_ident); \
}while(0)

// do nothing
#define COEVT_PROP_CLEAR(p)

#define COEVT_PROP_FREE(p) do { \
    COEVT_PROP_CLEAR(p); \
    pdealloc( (p)->ident ); \
}while(0)


#endif
