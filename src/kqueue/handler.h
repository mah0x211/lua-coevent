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

#define coevt_create()  kqueue()


static inline int coevt_wait( loop_t *loop, int sec )
{
    // defaout timeout: 1 sec
    struct timespec ts = { sec, 0 };
    
    return kevent( loop->fd, NULL, 0, loop->evs, (int)loop->nreg, &ts );
}


static inline void coevt_rw_init( coevt_t *evt, sentry_t *s, coevt_type_t type, 
                                  coevt_flag_t flg )
{
    EV_SET( evt, s->ident, type, flg, 0, 0, (void*)s );
}


static inline int coevt_add( loop_t *loop, sentry_t *s, coevt_t *evt )
{
    COEVT_UNUSED(s);
    int rc = 0;
    
    evt->flags |= EV_ADD;
    if( ( rc = kevent( loop->fd, evt, 1, NULL, 0, NULL ) ) == 0 ){
        loop->nreg++;
    }
    
    return rc;
}

static inline int coevt_del( loop_t *loop, sentry_t *s, coevt_t *evt )
{
    COEVT_UNUSED(s);
    int rc = 0;
    
    evt->flags = EV_DELETE;
    if( ( rc = kevent( loop->fd, evt, 1, NULL, 0, NULL ) ) == 0 ){
        loop->nreg--;
    }
    
    return rc;
}


#endif
