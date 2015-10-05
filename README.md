lua-coevent
===

coroutine based kqueue/epoll module.

**NOTE: Do not use this module. this module is under heavy development.**

***

## Dependencies

- reco: https://github.com/mah0x211/lua-reco
- sentry: https://github.com/mah0x211/lua-sentry


## Installation

```sh
luarocks install coevent --from=http://mah0x211.github.io/rocks/
```

## Create an CoEvent Object.

### co = coevent.new()

returns a new `CoEvent` object.

**Returns**

- `co:table`: coevent object.

**Example**

```lua
local coevent = require('coevent');
local co = coevent.new();
```


## CoEvent Methods

### h, err = co:createHandler( exceptionCb, eventCb [, ctx, ...] )

returns a new `CoEventHandler` object that associated with `CoEvent` object.


**Parameters**

- `exceptionCb:function`: callback function for exception `default: default exceptionCb`
- `eventCb:function`: callback function for event.
- `ctx:any`: first argument for callback function.
- `...`: any other arguments for callback function.


**Returns**

- `h:table`: event handler object on success, or nil on failure.
- `err:string`: error message.


**Example**

```lua
local co = require('coevent').new();
-- https://github.com/mah0x211/lua-signal
local signal = require('signal');

-- block SIGINT
signal.block( signal.SIGINT );

local function callback( ctx, ev, evtype, hup, handler )
    ctx.count = ctx.count + 1;
    print( 'callback', ev, evtype, hup, handler, ctx.count );
    -- throw an exception error
    --local a;
    --a = a + ctx;

    if ctx.count > 2 then
        ev:unwatch();
    end
end

-- exception handler
local function exception( ctx, err, ev, evtype, hup, handler )
    print( 'got exception', ctx, err, ev, evtype, hup, handler );
    -- unwatch event
    ev:unwatch();
end

-- create handler
local h = assert( co:createHandler( exception, callback, { count = 0 } ) );
```

---

### err = co:start()

run the event loop until the registered events exits.

**Returns**

- `err:string`: error message.


**Example**

```lua
co:start();
```

---

### co:stop()

stop the event loop immediately.

**Example**

```lua
co:stop();
```


## CoEventHandler Methods

### h:close()

close all events of handler.

---

### ev, err = h:watchTimer( ival:number [, oneshot:boolean] )

returns a new timer event object that associated with `CoEventHandler` object.


**Parameters**

- `ival:number`: timeout seconds.
- `oneshot:boolean`: automatically unregister this event when event occurred.  `default false`

**Returns**

- `ev:userdata`: event object on success, or nil on failure.
- `err:string`: error message.


**Example**

```lua
local ev = assert( h:watchTimer( 1, true ) );
```

---


### ev, err = h:watchSignal( signo:int [, oneshot:boolean] )

returns a signal event object that associated with `CoEventHandler` object.


**Parameters**

- `signo:int`: signal number.
- `oneshot:boolean`: automatically unregister this event when event occurred. `default false`

**Returns**

- `ev:userdata`: event object on success, or nil on failure.
- `err:string`: error message.


**Example**

```lua
-- https://github.com/mah0x211/lua-signal
local signal = require('signal');
local ev = assert( h:watchSignal( signal.SIGINT, true ) );
```

---

### ev, err = h:watchReadable( fd:int [, oneshot:boolean [, edge:boolean]] )

returns a readable event object that associated with `CoEventHandler` object.

**Parameters**

- `fd:int`: descriptor.
- `oneshot:boolean`: automatically unregister this event when event occurred.  `default false`
- `edge:boolean`: using an edge-trigger if specified a true. `default false`

**Returns**

- `ev:userdata`: event object on success, or nil on failure.
- `err:string`: error message.


**Example**

```lua
local ev = assert( h:watchReadable( 0, true ) );
```

---

### ev, err = h:watchWritable( fd:int [, oneshot:boolean [, edge:boolean]] )

returns a a writable event object that associated with `CoEventHandler` object.


**Parameters**

- `fd:int`: descriptor.
- `oneshot:boolean`: automatically unregister this event when event occurred.  `default false`
- `edge:boolean`: using an edge-trigger if specified a true. `default false`


**Returns**

- `ev:userdata`: event object on success, or nil on failure.
- `err:string`: error message.


**Example**

```lua
local ev = assert( h:watchWritable( 1, true ) );
```


## Event Methods

please refer to the following document;  
https://github.com/mah0x211/lua-sentry#event-methods

