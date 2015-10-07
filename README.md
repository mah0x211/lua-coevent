lua-coevent
===

coroutine based kqueue/epoll module.

**NOTE: Do not use this module. this module is under heavy development.**

***

## Dependencies

- reco: https://github.com/mah0x211/lua-reco
- sentry: https://github.com/mah0x211/lua-sentry

---

## Installation

```sh
luarocks install coevent --from=http://mah0x211.github.io/rocks/
```

---

## Constants

**Event Types**

the following constants are same of the event types constants of sentry module.   
https://github.com/mah0x211/lua-sentry#constants

- `coevent.EV_READABLE` readable event.
- `coevent.EV_WRITABLE` writable event.
- `coevent.EV_TIMER` timer event.
- `coevent.EV_SIGNAL` signal event.

---

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

---


## CoEvent Methods

### h, err = co:createHandler( eventCb [, ctx, [ exceptionCb]] )

returns a new `CoEventHandler` object that associated with `CoEvent` object.


**Parameters**

- `eventCb:function`: callback function for event.
- `ctx:any`: first argument for callback function.
- `exceptionCb:function`: callback function for exception. `default: default exceptionCb`


**Returns**

- `h:table`: event handler object on success, or nil on failure.
- `err:string`: error message.


**Example**

```lua
local co = require('coevent').new();

local function callback( ctx, ... )
    ctx.count = ctx.count + 1;
    print( 'callback', ctx.count, ... );
    
    -- throw an exception error
    local a;
    a = a + ctx;
end

-- exception handler
local function exception( ctx, handler, err, ... )
    print( 'got exception', handler, ctx, err, ... );
    -- close handler
    handler:close();
end

-- create handler
local h = assert( co:createHandler( callback, { count = 0 }, exception ) );
```


### handler, ev, evtype, ishup, err = co:getevent()

returns an event sequentially.

**Returns**

- `handler:table`: event handler object.
- `ev:userdata`: event object.
- `evtype:int`: event type.
- `ishup:boolean`: hang-up status.
- `err:string`: error message.


**Example**

```lua
local handler, ev, evtype, ishup, err;

repeat
    handler, ev, evtype, ishup, err = co:getevent();
    if err then
        print( err );
        break;
    elseif handler then
        handler:invoke( ev, evtype, ishup );
    end
until not handler;
```

---


## CoEventHandler Methods

### h:close()

close all events of handler.


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

---

## Event Methods

please refer to the following document;  
https://github.com/mah0x211/lua-sentry#event-methods
