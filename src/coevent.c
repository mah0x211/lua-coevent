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
 *  coevent.c
 *  lua-coevent
 *
 *  Created by Masatoshi Teruya on 14/05/13.
 *  Copyright 2014 Masatoshi Teruya. All rights reserved.
 *
 */

#include "coevent.h"

LUALIB_API int luaopen_coevent( lua_State *L )
{
    // add methods
    lua_newtable( L );
    // loop
    luaopen_coevent_loop( L );
    lua_setfield( L, -2, "loop" );
    // signal
    luaopen_coevent_signal( L );
    lua_setfield( L, -2, "signal" );
    // timer
    luaopen_coevent_timer( L );
    lua_setfield( L, -2, "timer" );
    // input
    luaopen_coevent_input( L );
    lua_setfield( L, -2, "input" );
    // output
    luaopen_coevent_output( L );
    lua_setfield( L, -2, "output" );
    // input/output
    luaopen_coevent_io( L );
    lua_setfield( L, -2, "io" );
    
    return 1;
}


