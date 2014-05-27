/*
 *  fdset.h
 *  lua-coevent
 *
 *  Created by Masatoshi Teruya on 14/05/26.
 *  Copyright 2014 Masatoshi Teruya. All rights reserved.
 *
 */

#ifndef ___FDSET___
#define ___FDSET___

#include "bitvec.h"

typedef bitvec_t fdset_t;

enum FDSET_MEMBER_TYPE {
    FDSET_READ  = 0x1,
    FDSET_WRITE = 0x2,
    FDEST_RDWR  = FDSET_READ|FDSET_WRITE
};

static inline int fdset_alloc( fdset_t *set, size_t nfd )
{
    return bitvec_alloc( set, nfd + 1 );
}

static inline int fdset_realloc( fdset_t *set, int fd )
{
    if( fd < 0 ){
        errno = EINVAL;
        return -1;
    }
    
    return bitvec_realloc( set, ( fd << 1 ) + 1 );
}

static inline void fdset_dealloc( fdset_t *set )
{
    bitvec_dealloc( set );
}

static inline int fdismember( fdset_t *set, int fd, int type )
{
    if( fd < 0 || type & ~FDEST_RDWR ){
        errno = EINVAL;
        return -1;
    }
    else if( type == FDSET_READ ){
        return bitvec_get( set, fd << 1 );
    }
    
    return bitvec_get( set, ( fd << 1 ) + 1 );
}

static inline int fdaddset( fdset_t *set, int fd, int type )
{
    if( fd < 0 || type & ~FDEST_RDWR ){
        errno = EINVAL;
        return -1;
    }
    else if( type == FDSET_READ ){
        return bitvec_set( set, fd << 1 );
    }
    
    return bitvec_set( set, ( fd << 1 ) + 1 );
}

static inline int fddelset( fdset_t *set, int fd, int type )
{
    if( fd < 0 || type & ~FDEST_RDWR ){
        errno = EINVAL;
        return -1;
    }
    else if( type == FDSET_READ ){
        return bitvec_unset( set, fd << 1 );
    }
    
    return bitvec_unset( set, ( fd << 1 ) + 1 );
}


#endif
