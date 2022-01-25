#pragma once


// "Lua headers must be included prior to LuaBridge ones"

// Include the Lua API header files.
extern "C"{
	#include "Lua/lua.h"
	#include "Lua/lauxlib.h"
	#include "Lua/lualib.h"
	#include "Lua/lgc.h"
}//extern "C"



// Undefine Lua G(L) crap

// conflicts with R,G,B,A
#undef G

// cast(t, exp) in "llimits.h"
#undef cast



#include <LuaBridge/LuaBridge.h>
