# Scripting module.



### Features

- Lua scripting backend with coroutine scheduler for minimal frame load


### Lua

Lessons after a year with Lua
December 7, 2014
https://www.ilikebigbits.com/2014_12_07_lessons_from_lua.html

Sol - Typesafe Lua
Static type checker and (optional) gradual typing for Lua.
Sol is to Lua as Typescript is to JS.
https://github.com/emilk/sol

Good Lua tips (especially for writing Lua libraries):
http://kiki.to/

LuaBridge 2.6 Reference Manual
http://vinniefalco.github.io/LuaBridge/Manual.html

A curated list of awesome Lua frameworks, libraries and software.
https://github.com/uhub/awesome-lua


Something else I thought I’d share that I found which would probably be useful for others reading this: you can kill an individual thread, but you don’t want to use lua_close to do it. You first get the refkey when you create the thread, then kill the reference to kill the thread.

For example:

// when creating the thread
int result = lua_pcall(state, 0, 0, 0);
thread->m_state = lua_newthread(state);
thread->m_refkey = luaL_ref(state, LUA_REGISTRYINDEX);

// when you want to kill it.
lua_unref(m_state, m_refkey);




### Quirrel

Quirrel
https://quirrel.io/

Good intro to Squirrel:
https://developer.electricimp.com/squirrel/

Game engines using Squirrel (examples):
https://code.camijo.de/gmueller/bluecore/src/branch/master/engine/ScriptSystem_Application.cpp


### Stackless Python

https://github.com/stackless-dev/stackless/wiki
repo:
https://github.com/stackless-dev/stackless/tree/master-slp/Stackless

Embedding
https://stackless.readthedocs.io/en/2.7-slp/extending/embedding.html

https://stackless.readthedocs.io/en/3.7-slp/library/py_compile.html



## References:


### Half-Life Alyx Scripting API

https://developer.valvesoftware.com/wiki/Half-Life_Alyx_Scripting_API


### Squirrel:
http://wiki.squirrel-lang.org/mainsite/Wiki/default.aspx/SquirrelWiki/Embedding%20Getting%20Started.html
