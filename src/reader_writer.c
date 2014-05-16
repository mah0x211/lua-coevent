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
 *  reader.c
 *  lua-coevent
 *
 *  Created by Masatoshi Teruya on 14/05/13.
 *  Copyright 2014 Masatoshi Teruya. All rights reserved.
 *
 */

#include "sentry.h"


static int rw_watch( lua_State *L, const char *tname, int type )
{
    sentry_t *s = luaL_checkudata( L, 1, tname );
    
    // check arguments
    // arg#2 oneshot
    luaL_checktype( L, 2, LUA_TBOOLEAN );
    // arg#3 edge-trigger
    luaL_checktype( L, 3, LUA_TBOOLEAN );
    // arg#4 callback function
    luaL_checktype( L, 4, LUA_TFUNCTION );
    // arg#5 user-context
    
    if( SENTRY_IS_REGISTERED( s ) ){
        errno = EALREADY;
    }
    else
    {
        coevt_t evt;
        
        coevt_rw_init( &evt, s, type, lua_toboolean( L, 2 ), 
                       lua_toboolean( L, 3 ) );
        
        // retain callback and usercontext
        s->ref_fn = lstate_ref( L, 4 );
        s->ref_ctx = lstate_ref( L, 5 );
        // register sentry
        if( sentry_register( L, s, &evt ) == 0 ){
            return 0;
        }
    }
    
    // got error
    lua_pushnumber( L, errno );
    
    return 1;
}

static int reader_watch_lua( lua_State *L )
{
    return rw_watch( L, COREADER_MT, COEVT_READ );
}

static int writer_watch_lua( lua_State *L )
{
    return rw_watch( L, COWRITER_MT, COEVT_WRITE );
}



static int rw_unwatch( lua_State *L, const char *tname, int type )
{
    sentry_t *s = luaL_checkudata( L, 1, tname );
    
    if( SENTRY_IS_REGISTERED( s ) )
    {
        coevt_t evt;
        
        coevt_rw_init( &evt, s, type, 0, 0 );
        if( sentry_unregister( L, s, &evt ) != 0 ){
            // got error
            lua_pushnumber( L, errno );
            return 1;
        }
    }
    
    return 0;
}

static int reader_unwatch_lua( lua_State *L )
{
    return rw_unwatch( L, COREADER_MT, COEVT_READ );
}

static int writer_unwatch_lua( lua_State *L )
{
    return rw_unwatch( L, COWRITER_MT, COEVT_WRITE );
}


static int rw_alloc( lua_State *L, const char *tname )
{
    loop_t *loop = luaL_checkudata( L, 1, COLOOP_MT );
    lua_Integer fd = luaL_checkinteger( L, 2 );
    sentry_t *s = NULL;
    
    // check argument
    if( fd < 0 || fd > INT_MAX ){
        return luaL_error( L, "fd value range must be 0 to %d", INT_MAX );
    }
    // allocate sentry
    else if( ( s = sentry_alloc( L, loop, tname, fd, 0, 0 ) ) ){
        return 1;
    }
    
    // got error
    lua_pushnil( L );
    lua_pushinteger( L, errno );
    
    return 2;
}

static int reader_alloc_lua( lua_State *L )
{
    return rw_alloc( L, COREADER_MT );
}

static int writer_alloc_lua( lua_State *L )
{
    return rw_alloc( L, COWRITER_MT );
}


static int reader_tostring_lua( lua_State *L )
{
    return tostring_mt( L, COREADER_MT );
}

static int writer_tostring_lua( lua_State *L )
{
    return tostring_mt( L, COWRITER_MT );
}


LUALIB_API int luaopen_coevent_reader( lua_State *L )
{
    struct luaL_Reg mmethod[] = {
        { "__gc", sentry_gc },
        { "__tostring", reader_tostring_lua },
        { NULL, NULL }
    };
    struct luaL_Reg method[] = {
        { "watch", reader_watch_lua },
        { "unwatch", reader_unwatch_lua },
        { NULL, NULL }
    };

    define_mt( L, COREADER_MT, mmethod, method );
    // add methods
    lua_pushcfunction( L, reader_alloc_lua );
    
    return 1;
}


LUALIB_API int luaopen_coevent_writer( lua_State *L )
{
    struct luaL_Reg mmethod[] = {
        { "__gc", sentry_gc },
        { "__tostring", writer_tostring_lua },
        { NULL, NULL }
    };
    struct luaL_Reg method[] = {
        { "watch", writer_watch_lua },
        { "unwatch", writer_unwatch_lua },
        { NULL, NULL }
    };

    define_mt( L, COWRITER_MT, mmethod, method );
    // add methods
    lua_pushcfunction( L, writer_alloc_lua );
    
    return 1;
}

