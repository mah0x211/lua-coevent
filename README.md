# lua-coevent

coroutine based kqueue/epoll module.

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

- loop, errno = coevent.loop( evtbuf:number(default: 128), exceptionHandler:function )

**Methods**

- errno = loop:run( timeout:number )
- ok = loop:stop()


## coevent.signal

**Functions**

- watcher, errno = coevent.signal( loop, signo:number )

**Methods**

- errno = watcher:watch( oneshot:boolean, callback:function, context:[any type] )
- errno = watcher:unwatch()


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

- watcher, errno = coevent.timer( loop, timeout:number )

**Methods**

- errno = watcher:watch( oneshot:boolean, callback:function, context:[any type] )
- errno = watcher:unwatch()


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

- watcher, errno = coevent.input( loop, fd:number [, edgeTrigger:boolean] )

**Methods**

- errno = watcher:watch( oneshot:boolean, callback:function, context:[any type] )
- errno = watcher:unwatch()


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

- watcher, errno = coevent.output( loop, fd:number [, edgeTrigger:boolean] )

**Methods**

- errno = watcher:watch( oneshot:boolean, callback:function, context:[any type] )
- errno = watcher:unwatch()


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
