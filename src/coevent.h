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
 *  coevent.h
 *  lua-coevent
 *
 *  Created by Masatoshi Teruya on 14/05/13.
 *  Copyright 2014 Masatoshi Teruya. All rights reserved.
 *
 */

#ifndef ___COEVENT_LUA___
#define ___COEVENT_LUA___

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <time.h>
// lualib
#include <lauxlib.h>
#include <lualib.h>

#include "config.h"
#include "coevent_types.h"

// data structure
// loop
typedef struct {
    int fd;
    int state;
    int ref_fn;
    int nevs;
    int nreg;
    lua_State *L;
    coevt_t *evs;
} loop_t;


// sentries
typedef struct {
    lua_State *L;
    int th;
    int fn;
    int ctx;
    // oneshot flag
    coevt_flag_t oneshot;
    // drain data
    COREFS_DRAIN_DEFS();
} sentry_refs_t;


typedef struct {
    int type;
    int ref;
    coevt_flag_t trigger;
    coevt_ident_t ident;
    loop_t *loop;
    sentry_refs_t refs;
} sentry_t;



#define COEVT_UNUSED(_a) ((void)_a)

// memory alloc/dealloc
#define palloc(t)       (t*)malloc( sizeof(t) )
#define pnalloc(n,t)    (t*)malloc( (n) * sizeof(t) )
#define pcalloc(n,t)    (t*)calloc( n, sizeof(t) )
#define prealloc(n,t,p) (t*)realloc( p, (n) * sizeof(t) )
#define pdealloc(p)     free((void*)p)

// print message to stdout
#define plog(fmt,...) \
    printf( fmt "\n", ##__VA_ARGS__ )

#define pflog(f,fmt,...) \
    printf( #f "(): " fmt "\n", ##__VA_ARGS__ )

// print message to stderr
#define pelog(fmt,...) \
    fprintf( stderr, fmt "\n", ##__VA_ARGS__ )

#define pfelog(f,fmt,...) \
    fprintf( stderr, #f "(): " fmt "\n", ##__VA_ARGS__ )

// print message to stderr with strerror
#define pelogerr(fmt,...) \
    fprintf( stderr, fmt " : %s\n", ##__VA_ARGS__, strerror(errno) )

#define pfelogerr(f,fmt,...) \
    fprintf( stderr, #f "(): " fmt " : %s\n", ##__VA_ARGS__, strerror(errno) )


// helper macros for lua_State
#define lstate_setmetatable(L,tname) \
    (luaL_getmetatable(L,tname),lua_setmetatable(L,-2))

#define lstate_ref(L,idx) \
    (lua_pushvalue(L,idx),luaL_ref( L, LUA_REGISTRYINDEX ))

#define lstate_isref(ref) \
    ((ref) > LUA_REFNIL)

#define lstate_pushref(L,ref) \
    lua_rawgeti( L, LUA_REGISTRYINDEX, ref )

#define lstate_unref(L,ref) \
    luaL_unref( L, LUA_REGISTRYINDEX, ref )

#define lstate_fn2tbl(L,k,v) do{ \
    lua_pushstring(L,k); \
    lua_pushcfunction(L,v); \
    lua_rawset(L,-3); \
}while(0)

#define lstate_str2tbl(L,k,v) do{ \
    lua_pushstring(L,k); \
    lua_pushstring(L,v); \
    lua_rawset(L,-3); \
}while(0)

#define lstate_num2tbl(L,k,v) do{ \
    lua_pushstring(L,k); \
    lua_pushnumber(L,v); \
    lua_rawset(L,-3); \
}while(0)


// define prototypes
LUALIB_API int luaopen_coevent( lua_State *L );
LUALIB_API int luaopen_coevent_loop( lua_State *L );
LUALIB_API int luaopen_coevent_reader( lua_State *L );
LUALIB_API int luaopen_coevent_writer( lua_State *L );
LUALIB_API int luaopen_coevent_signal( lua_State *L );
LUALIB_API int luaopen_coevent_timer( lua_State *L );


// define module names
#define COLOOP_MT   "coevent.loop"
#define COREADER_MT "coevent.reader"
#define COWRITER_MT "coevent.writer"
#define COSIGNAL_MT "coevent.signal"
#define COTIMER_MT  "coevent.timer"


// define loop types
#define CORUN_NONE       0
#define CORUN_ONCE       1
#define CORUN_FOREVER    2


static inline void double2timespec( double tval, struct timespec *ts )
{
    double sec = 0, nsec = 0;
    
    nsec = modf( tval, &sec );
    ts->tv_sec = (time_t)sec;
    ts->tv_nsec = (long)(nsec * 1000000000);
}


// metanames
// module definition register
static inline int define_mt( lua_State *L, const char *tname, 
                             struct luaL_Reg mmethod[], 
                             struct luaL_Reg method[] )
{
    int i;
    
    // create table __metatable
    luaL_newmetatable( L, tname );
    // metamethods
    i = 0;
    while( mmethod[i].name ){
        lstate_fn2tbl( L, mmethod[i].name, mmethod[i].func );
        i++;
    }
    // methods
    lua_pushstring( L, "__index" );
    lua_newtable( L );
    i = 0;
    while( method[i].name ){
        lstate_fn2tbl( L, method[i].name, method[i].func );
        i++;
    }
    lua_rawset( L, -3 );
    lua_pop( L, 1 );

    return 1;
}


// common metamethods
#define tostring_mt(L,tname) ({ \
    lua_pushfstring( L, tname ": %p", lua_touserdata( L, 1 ) ); \
    1; \
})


#endif

