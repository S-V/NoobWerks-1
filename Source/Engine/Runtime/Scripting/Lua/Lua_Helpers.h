#pragma once

#include <Core/ConsoleVariable.h>
#include <Scripting/ForwardDecls.h>


// Include the Lua API header files.
extern "C"{
	#include "Lua/lua.h"
	#include "Lua/lauxlib.h"
	#include "Lua/lualib.h"
	#include "Lua/lgc.h"
}//extern "C"

// Lua G(L) crap (conflicts with R,G,B,A)
#undef G


#if MX_AUTOLINK
mxHACK("Premake doesn't add Lua dependency in MVC++ 12.0");
#pragma comment( lib, "Lua.lib" )
#endif



ERet Lua_GetErrorCode( int luaErr );
void Lua_PrintStack( lua_State* L );

#define LUA_TRY( X, L )\
	mxMACRO_BEGIN\
		const int __result = (X);\
		if( __result )\
		{\
			ptWARN("[SCRIPT] Lua error: %s\n", lua_tostring(L, -1));\
			lua_pop(L, 1);  /* pop error message from the stack */\
			mxASSERT2(false, #X);\
			return Lua_GetErrorCode(__result);\
		}\
	mxMACRO_END




class LuaStackGuard {
	lua_State *	m_L;
	int			m_top;
public:
	inline LuaStackGuard( lua_State* L ) {
		m_L = L;
		m_top = lua_gettop( L );
	}
	inline ~LuaStackGuard() {
		lua_settop( m_L, m_top );
	}
};

#define LUA_GUARD_STACK(L)	LuaStackGuard __guard__(L);


#if MX_DEBUG
	class LuaStackValidator {
		const char*	m_text;
		lua_State*	m_L;
		int	m_top;
	public:
		LuaStackValidator( lua_State* L, const char *name )
		{
			m_text = name;
			m_L = L;
			m_top = lua_gettop( L );
		}
		~LuaStackValidator()
		{
			const int top = lua_gettop( m_L );
			if( m_top != top )
			{
				mxASSERT( 0 && "Lua Stack Validation Failed" );
				lua_settop( m_L, m_top );
			}
		}
	};
	#define LUA_DBG_CHECK_STACK(L)	LuaStackValidator __stackCheck((L),__FUNCTION__);
#else
	#define LUA_DBG_CHECK_STACK(L)
#endif


struct CLuaTable {
	//...
};


// see ScriptAnyValue in CryEngine
struct ScriptVariant
{
	enum Type {
		Nil,
		Boolean,
		Number,
		String,
		LuaTable,
		Function,
		UserData,
		LuaThread,
		Undefined,
	};
	Type	type;
	union {
		bool			b;
		char			c;
		float			n;
		const char *	s;
		lua_CFunction 	f;
		const void *	p;
		CLuaTable *		t;
		void *			U;
	};

	static const ScriptVariant FromInt( const bool _i );
	static const ScriptVariant FromBool( const bool _b );
	static const ScriptVariant FromFloat( const float _f );
	static const ScriptVariant FromDouble( const double _d );
	static const ScriptVariant FromString( const char* _str );

	inline const char* ToStringPtr() const
	{
		return (type == String) ? s : nil;
	}

	// Generic get
	template< typename TYPE > TYPE To() { return 0; }

	// Specializations
	template<> bool To< bool >() {
		mxASSERT(type == Boolean);
		return b;
	}
	template<> int To< int >() {
		mxASSERT(type == Number);
		return (int) n;
	}
	template<> float To< float >() {
		mxASSERT(type == Number);
		return n;
	}
	template<> const char* To< const char* >() {
		mxASSERT(type == String);
		return s;
	}
};

ScriptVariant::Type GetValueType( int luaValType );

void Lua_Push( lua_State* L, const ScriptVariant& _value );
const ScriptVariant Lua_Pop( lua_State* L );

struct LuaModule {
	const char*		name;
	const luaL_Reg*	functions;
	int				numFunctions;
};

// Makes sure that a global table with the given name exists.
void Lua_EnsureGlobalTableExists( lua_State* L, const char* table );

void Lua_RegisterModule( lua_State* L, const LuaModule& module );




mxSTOLEN("https://github.com/leafo/aroma/src/common.h");

#define setint(name,val)\
	lua_pushinteger(l, val);\
	lua_setfield(l, -2, name)

#define setnumber(name,val)\
	lua_pushnumber(l, val);\
	lua_setfield(l, -2, name)

#define setbool(name,val)\
	lua_pushboolean(l, val);\
	lua_setfield(l, -2, name)

#define setfunction(name,val)\
	lua_pushcfunction(l, val);\
	lua_setfield(l, -2, name)

#define newuserdata(type)\
	((type*)lua_newuserdata(l, sizeof(type)))

#define getself(type)\
	((type*)luaL_checkudata(l, 1, #type))

#define getselfi(type, i)\
	((type*)luaL_checkudata(l, i, #type))

#define set_new_func(name, func)\
	lua_pushlightuserdata(l, this);\
	lua_pushcclosure(l, func, 1);\
	lua_setfield(l, -2, name)

#define upvalue_self(type)\
	((type*)lua_touserdata(l, lua_upvalueindex(1)))


ERet AssignConVar( const AConsoleVariable* _convar, const ScriptVariant& _value );

mxSTOLEN( "https://github.com/nem0/LumixEngine/src/engine/core/lua_wrapper.h" );
namespace LuaWrapper
{
	template <typename T> inline T toType( lua_State* L, int index )
	{
		return (T) lua_touserdata( L, index );
	}
	template <> inline I32 toType( lua_State* L, int index )
	{
		return (I32) lua_tointeger( L, index );
	}
	template <> inline U32 toType( lua_State* L, int index )
	{
		return (U32) lua_tointeger( L, index );
	}
	template <> inline U64 toType( lua_State* L, int index )
	{
		return (U64) lua_tointeger( L, index );
	}
	template <> inline I64 toType( lua_State* L, int index )
	{
		return (I64) lua_tointeger( L, index );
	}
	template <> inline bool toType( lua_State* L, int index )
	{
		return lua_toboolean( L, index ) != 0;
	}
	template <> inline float toType( lua_State* L, int index )
	{
		return (float) lua_tonumber( L, index );
	}
	template <> inline double toType( lua_State* L, int index )
	{
		return (double) lua_tonumber( L, index );
	}
	template <> inline const char* toType( lua_State* L, int index )
	{
		return lua_tostring( L, index );
	}
	template <> inline void* toType( lua_State* L, int index )
	{
		return lua_touserdata( L, index );
	}



	template <typename T> inline bool isType( lua_State* L, int index )
	{
		return lua_islightuserdata( L, index ) != 0;
	}
	template <> inline bool isType<I32>( lua_State* L, int index )
	{
		return lua_isinteger( L, index ) != 0;
	}
	template <> inline bool isType<U32>( lua_State* L, int index )
	{
		return lua_isinteger( L, index ) != 0;
	}
	template <> inline bool isType<I64>( lua_State* L, int index )
	{
		return lua_isinteger( L, index ) != 0;
	}
	template <> inline bool isType<U64>( lua_State* L, int index )
	{
		return lua_isinteger( L, index ) != 0;
	}
	template <> inline bool isType<bool>( lua_State* L, int index )
	{
		return lua_isboolean( L, index ) != 0;
	}
	template <> inline bool isType<float>( lua_State* L, int index )
	{
		return lua_isnumber( L, index ) != 0;
	}
	template <> inline bool isType<double>( lua_State* L, int index )
	{
		return lua_isnumber( L, index ) != 0;
	}
	template <> inline bool isType<const char*>( lua_State* L, int index )
	{
		return lua_isstring( L, index ) != 0;
	}
	template <> inline bool isType<void*>( lua_State* L, int index )
	{
		return lua_islightuserdata( L, index ) != 0;
	}


	template <typename T> inline const char* typeToString()
	{
		return "userdata";
	}
	template <> inline const char* typeToString<int>()
	{
		return "number";
	}
	template <> inline const char* typeToString<U32>()
	{
		return "number";
	}
	template <> inline const char* typeToString<I64>()
	{
		return "number";
	}
	template <> inline const char* typeToString<U64>()
	{
		return "number";
	}
	template <> inline const char* typeToString<float>()
	{
		return "number";
	}
	template <> inline const char* typeToString<double>()
	{
		return "number";
	}
	template <> inline const char* typeToString<const char*>()
	{
		return "string";
	}
	template <> inline const char* typeToString<bool>()
	{
		return "boolean";
	}


	template <typename T> inline void pushLua( lua_State* L, T value )
	{
		lua_pushlightuserdata( L, value );
	}
	template <> inline void pushLua( lua_State* L, float value )
	{
		lua_pushnumber( L, value );
	}
	template <> inline void pushLua( lua_State* L, bool value )
	{
		lua_pushboolean( L, value );
	}
	template <> inline void pushLua( lua_State* L, const char* value )
	{
		lua_pushstring( L, value );
	}
	template <> inline void pushLua( lua_State* L, int value )
	{
		lua_pushinteger( L, value );
	}
	template <> inline void pushLua( lua_State* L, void* value )
	{
		lua_pushlightuserdata( L, value );
	}


	inline const char* luaTypeToString( int type )
	{
		switch( type )
		{
		case LUA_TNUMBER: return "number";
		case LUA_TBOOLEAN: return "boolean";
		case LUA_TFUNCTION: return "function";
		case LUA_TLIGHTUSERDATA: return "light userdata";
		case LUA_TNIL: return "nil";
		case LUA_TSTRING: return "string";
		case LUA_TTABLE: return "table";
		case LUA_TUSERDATA: return "userdata";
		}
		return "Unknown";
	}
	template <typename T>
	bool checkParameterType( lua_State* L, int index )
	{
		if( !isType<T>( L, index ) )
		{
			int depth = 0;
			lua_Debug entry;

			int type = lua_type( L, index );
			LogStream er(LL_Warn);
			er << "Wrong argument " << index << " of type " << luaTypeToString( type )
				<< " in:\n";
			while( lua_getstack( L, depth, &entry ) )
			{
				int status = lua_getinfo( L, "Sln", &entry );
				mxASSERT( status );
				er << entry.short_src << "(" << entry.currentline
					<< "): " << (entry.name ? entry.name : "?") << "\n";
				depth++;
			}
			er << typeToString<T>() << " expected\n";
			return false;
		}
		return true;
	}
}//namespace LuaWrapper


namespace MyLua
{
	ERet LoadChunk(
		const NwScript&	script
		, lua_State* L
		);
}//namespace MyLua
