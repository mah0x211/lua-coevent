/*
 *  fdset.h
 *  lua-coevent
 *
 *  Created by Masatoshi Teruya on 14/05/27.
 *  Copyright 2014 Masatoshi Teruya. All rights reserved.
 *
 */

#ifndef ___FDSET___
#define ___FDSET___

#include "coevent_types.h"


typedef struct {
    int nevs;
    void **evs;
} fdset_t;

#define FV_SIZE sizeof(void*)


enum FDSET_MEMBER_TYPE {
    FDSET_READ  = EPOLLIN,
    FDSET_WRITE = EPOLLOUT
};


static inline int fdset_alloc( fdset_t *set, size_t nfd )
{
    set->evs = pcalloc( (size_t)nfd, void* );
    
    if( set->evs ){
        set->nevs = nfd;
        return 0;
    }
    
    return -1;
}

static inline int fdset_realloc( fdset_t *set, int fd )
{
    if( fd < 0 ){
        errno = EINVAL;
        return -1;
    }
    else if( fd >= set->nevs )
    {
        // realloc event container
        void **evs = prealloc( fd + 1, void*, set->evs );
        
        if( !evs ){
            return -1;
        }
        memset( evs + FV_SIZE * set->nevs, 0, ( fd - set->nevs ) * FV_SIZE );
        set->nevs = fd;
        set->evs = evs;
    }
    
    return 0;
}

static inline void fdset_dealloc( fdset_t *set )
{
    pdealloc( set->evs );
}


static inline void *fdismember( fdset_t *set, int fd )
{
    if( fd < 0 || fd > set->nevs ){
        errno = EINVAL;
        return NULL;
    }
    
    return set->evs[fd];
}


static inline int fdaddset( fdset_t *set, int fd, void *evt )
{
    if( fd < 0 || fd > set->nevs ){
        errno = EINVAL;
        return -1;
    }
    
    set->evs[fd] = evt;
    
    return 0;
}

static inline int fddelset( fdset_t *set, int fd )
{
    if( fd < 0 || fd > set->nevs ){
        errno = EINVAL;
        return -1;
    }
    
    set->evs[fd] = NULL;
    
    return 0;
}


#endif
