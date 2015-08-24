# lua-coevent

coroutine based kqueue/epoll module.

**NOTE: Do not use this module. this module is under heavy development.**

## Installation

```sh
luarocks install --from=http://mah0x211.github.io/rocks/ coevent
```

or 

```sh
git clone https://github.com/mah0x211/lua-coevent.git
cd lua-coevent
luarocks make
```

## Functions

all function will throw lua exception if an invalid argument is passed. (usually, argument type error and etc.)

**Note:** exception handler cannot handle that errors.

## coevent.loop

**Functions**

- loop, err = coevent.loop( evtbuf:number(default: 128), exceptionHandler:function )

**Methods**

- err = loop:run( timeout:number )
- loop:stop()


## coevent.signal

**Functions**

- watcher, err = coevent.signal( loop, signo:number )

**Methods**

- signo = watcher:ident()
- err = watcher:watch( oneshot:boolean, callback:function, context:[any type] )
- watcher:unwatch()


**Example**

```lua
local event = require('coevent');
-- https://github.com/mah0x211/lua-signal
local signal = require('signal');
-- block SIGINT
signal.block( signal.INT );

local function callback( ctx, watcher, hup )
    ctx.count = ctx.count + 1;
    print( 'callback', ctx.count );
    if ctx.count > 2 then
        watcher:unwatch();
    end
end

-- create loop
local loop = event.loop();
-- create signal SIGINT watcher
local watcher = event.signal( loop, signal.INT );
local oneshot = false;

watcher:watch( oneshot, callback, { count = 0 } );
print( 'type ^C' );
print( 'done', loop:run() );
```


## coevent.timer

**Functions**

- watcher, err = coevent.timer( loop, timeout:number )

**Methods**

- timeout = watcher:ident()
- err = watcher:watch( oneshot:boolean, callback:function, context:[any type] )
- watcher:unwatch()


**Example**

```lua
local event = require('coevent');

local function callback( ctx, watcher, hup )
    ctx.count = ctx.count + 1;
    print( 'callback', ctx.count );
    if ctx.count > 2 then
        watcher:unwatch();
    end
end

-- create loop
local loop = event.loop();
-- create timer watcher
local watcher = event.timer( loop, 2 );
local oneshot = false;

watcher:watch( oneshot, callback, { count = 0 } );
print( 'calling callback function every 2 sec.' );
print( 'done', loop:run() );
```


## coevent.input

**Functions**

- watcher, err = coevent.input( loop, fd:number [, edgeTrigger:boolean] )

**Methods**

- fd = watcher:ident()
- err = watcher:watch( oneshot:boolean, callback:function, context:[any type] )
- watcher:unwatch()


**Example**

```lua
local event = require('coevent');

local function callback( ctx, watcher, hup )
    ctx.count = ctx.count + 1;
    print( 'callback', ctx.count );
    if ctx.count > 2 then
        watcher:unwatch();
    end
end

-- create loop
local loop = event.loop();
-- create io watcher: 0 = stdin
local oneshot = false;
local edgeTrigger = true;
local watcher = event.input( loop, 0, edgeTrigger );

watcher:watch( oneshot, callback, { count = 0 } );
print( 'type some keys then press enter.' );
print( 'done', loop:run() );
```

## coevent.output

**Functions**

- watcher, err = coevent.output( loop, fd:number [, edgeTrigger:boolean] )

**Methods**

- fd = watcher:ident()
- err = watcher:watch( oneshot:boolean, callback:function, context:[any type] )
- watcher:unwatch()


**Example**

```lua
local event = require('coevent');

local function callback( ctx, watcher, hup )
    ctx.count = ctx.count + 1;
    print( 'callback', ctx.count );
    if ctx.count > 2 then
        watcher:unwatch();
    end
end

-- create loop
local loop = event.loop();
-- create io watcher: 1 = stdout
local oneshot = false;
local edgeTrigger = true;
local watcher = event.output( loop, 1, edgeTrigger );

watcher:watch( oneshot, callback, { count = 0 } );
print( 'done', loop:run() );
```


## callback function to be wrapped in coroutine.

**Example**

```lua
local event = require('coevent');
-- https://github.com/mah0x211/lua-signal
local signal = require('signal');
-- block SIGINT
signal.block( signal.INT );

local function callback( ctx, watcher, hup )
    ctx.count = ctx.count + 1;
    print( 'yield' );
    coroutine.yield();
    print( 'resume' );
    print( 'callback', ctx.count );
    if ctx.count > 2 then
        watcher:unwatch();
    end
end

-- create loop
local loop = event.loop();
-- create signal SIGINT watcher
local watcher = event.signal( loop, signal.INT );
local oneshot = false;

watcher:watch( oneshot, callback, { count = 0 } );
print( 'type ^C' );
print( 'done', loop:run() );
```


but if that event is oneshot event, that throw an exception error.

**Example**

```lua
local event = require('coevent');
-- https://github.com/mah0x211/lua-signal
local signal = require('signal');
-- block SIGINT
signal.block( signal.INT );

local function callback( ctx, watcher, hup )
    ctx.count = ctx.count + 1;
    print( 'yield' );
    -- throw an exception error.
    coroutine.yield();
    print( 'resume' );
    print( 'callback', ctx.count );
    if ctx.count > 2 then
        watcher:unwatch();
    end
end

-- exeception handler
local function exception( ctx, watcher, info )
    print( 'got exception' );
    -- show error info
    for k,v in pairs( info ) do
        print( k, v );
    end
end

-- create loop: use default limit
local loop = event.loop( nil, exception );
-- create signal SIGINT watcher
local watcher = event.signal( loop, signal.INT );
local oneshot = true;

watcher:watch( oneshot, callback, { count = 0 } );
print( 'type ^C' );
print( 'done', loop:run() );
```

## TODO

- reduce redundancy.
