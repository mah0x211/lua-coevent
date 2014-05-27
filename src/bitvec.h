/*
 *  bitvec.h
 *
 *  Created by Masatoshi Teruya on 13/12/25.
 *  Copyright 2013 Masatoshi Teruya. All rights reserved.
 *
 */
#ifndef ___BITVEC___
#define ___BITVEC___

#include <limits.h>
#include <stdint.h>

#define BV_TYPE     uint64_t
#define BV_BYTE     sizeof(BV_TYPE)
static const uint8_t BV_BIT = CHAR_BIT * BV_BYTE;

typedef struct {
    size_t nbit;
    size_t nvec;
    BV_TYPE *vec;
} bitvec_t;

#define BIT2VEC_SIZE(nbit) \
    (nbit < BV_BIT ? 1 :( nbit / BV_BIT ) + ( nbit % BV_BIT ))

static inline int bitvec_alloc( bitvec_t *bv, size_t nbit )
{
    bv->nbit = nbit;
    bv->nvec = BIT2VEC_SIZE( nbit );
    if( ( bv->vec = calloc( BV_BYTE, bv->nvec ) ) ){
        bv->nbit = bv->nvec * BV_BIT;
        return 0;
    }
    
    return -1;
}

static inline int bitvec_realloc( bitvec_t *bv, size_t nbit )
{
    if( nbit > bv->nbit )
    {
        size_t nvec = BIT2VEC_SIZE( nbit );
        
        if( nvec > bv->nvec )
        {
            BV_TYPE *vec = realloc( bv->vec, BV_BYTE * nvec );
            
            if( vec ){
                memset( vec + bv->nvec, 0, ( nvec - bv->nvec ) * BV_BYTE );
                bv->vec = vec;
                bv->nvec = nvec;
                bv->nbit = nvec * BV_BIT;
                return 0;
            }
            
            return -1;
        }
    }
    
    return 0;
}

static inline void bitvec_dealloc( bitvec_t *bv )
{
    free( bv->vec );
}

static inline int bitvec_get( bitvec_t *bv, BV_TYPE pos )
{
    if( pos <= bv->nbit ){
        return (int)((((BV_TYPE*)bv->vec)[pos/BV_BIT] >> (pos % BV_BIT)) & (BV_TYPE)1);
    }
    
    return -1;
}

static inline int bitvec_set( bitvec_t *bv, BV_TYPE pos )
{
    if( pos <= bv->nbit ){
        ((BV_TYPE*)bv->vec)[pos / BV_BIT] |= (BV_TYPE)1 << ( pos % BV_BIT );
        return 0;
    }
    
    return -1;
}

static inline int bitvec_unset( bitvec_t *bv, BV_TYPE pos )
{
    if( pos <= bv->nbit ){
        ((BV_TYPE*)bv->vec)[pos / BV_BIT] &= ~( (BV_TYPE)1 << ( pos % BV_BIT ) );
        return 0;
    }
    
    return -1;
}



#endif

