// Framework for calling C++ functions from Lua.
// NOTE: only "light user data" return types are supported.
#pragma once

#include <Base/Object/Reflection/Reflection.h>

#include <Scripting/Lua/Lua_Helpers.h>


#define mxSCRIPT_CHECKS(X)		do { X } while(false)


#if mxSUPPORTS_VARIADIC_TEMPLATES

#include <tuple>

mxSTOLEN( "https://github.com/nem0/LumixEngine/src/engine/core/lua_wrapper.h" );
namespace LuaWrapper
{
	// A C function receives its arguments from Lua in its stack in direct order( the first argument is pushed first ).
	// The first argument( if any ) is at index 1 and its last argument is at index lua_gettop( L ).

	template< int NUM_ARGs >	// number of remaining arguments
	struct TCallFromLua
	{
		template< typename R, typename ...ARGs, typename ..._REST >
		static inline R BuildArgumentListAndCall(
			R( *f )(ARGs...),
			lua_State* L,
			_REST... rest )
		{
			const int param_index = (sizeof...(ARGs)) - NUM_ARGs + 1;	// Lua indices start at 1.

			typedef std::tuple_element<
				(sizeof...(ARGs)) - NUM_ARGs,
				std::tuple < ARGs... >
			> ::type	T;

			if( !checkParameterType<T>( L, param_index ) ) return R();

			T last = toType< T >( L, param_index );

			return TCallFromLua< NUM_ARGs - 1 >::BuildArgumentListAndCall( f, L, rest..., last );
		}
	};

	/// All arguments have been popped off the lua stack, and our function can now be called.
	template<>
	struct TCallFromLua< 0 >
	{
		template< typename R, typename ...ARGs, typename ..._REST >
		static inline R BuildArgumentListAndCall(
			R( *f )(ARGs...),
			lua_State* L,
			_REST... args )
		{
			return (*f)(args...);
		}
	};

	/// Returns 'void'.
	template <typename ...ARGs>
	inline int TCallFunctionWithSignature( void( *f )(ARGs...), lua_State* L )
	{
		constexpr int num_arguments = sizeof...(ARGs);
		mxASSERT2( num_arguments == lua_gettop( L ),
			"incorrect number of arguments: expected %d, but got %d", num_arguments, lua_gettop( L ) );
		TCallFromLua<num_arguments>::BuildArgumentListAndCall( f, L );
		return 0;
	}

	/// Returns 'R'.
	template <typename R, typename ...ARGs>
	inline int TCallFunctionWithSignature( R( *f )(ARGs...), lua_State* L )
	{
		constexpr int num_arguments = sizeof...(ARGs);
		mxASSERT2( num_arguments == lua_gettop( L ),
			"incorrect number of arguments: expected %d, but got %d", num_arguments, lua_gettop( L ) );
		R v = TCallFromLua<sizeof...(ARGs)>::BuildArgumentListAndCall( f, L );
		pushLua( L, v );
		return 1;
	}

	/// a wrapper compatible with lua_CFunction.
	/// generates a C function callable from a Lua script;
	template<
		typename FUNCTION_SIGNATURE,
		FUNCTION_SIGNATURE function_pointer
	>
	static inline int Callable_From_Lua( lua_State* L )
	{
		return TCallFunctionWithSignature( function_pointer, L );
	}

}//namespace LuaWrapper


#else


#define CHECK_PARAM_COUNT( num_arguments )\
	mxASSERT2( num_arguments == lua_gettop( L ),\
			"incorrect number of arguments: expected %d, but got %d", num_arguments, lua_gettop( L ) );

#define CHECK_PARAM_TYPE( param_index, TYPE )\
	if( !LuaWrapper::checkParameterType<TYPE>( L, param_index ) ) return 0;


namespace LuaWrapper
{
	/// Takes no parameters and returns nothing (void).
	struct VoidFunctionFactory0
	{
		template< void (*Fun)(void) >
		static inline int MakeLuaCallback( lua_State* L )
		{
			CHECK_PARAM_COUNT(0);
			(*Fun)();
			return 0;
		}
	};
	static inline VoidFunctionFactory0 GetFunctionFactory( void (*)(void) )
	{
		return VoidFunctionFactory0();
	}

	/// Takes one parameter (P1) and returns nothing (void).
	template< typename P1 >
	struct VoidFunctionFactory1
	{
		template< void (*Fun)(P1) >
		static inline int MakeLuaCallback( lua_State* L )
		{
			CHECK_PARAM_COUNT(1);
			CHECK_PARAM_TYPE(1, P1);
			P1 p1 = toType< P1 >( L, 1 );
			(*Fun)( p1 );
			return 0;
		}
	};
	template< typename P1 >
	static inline VoidFunctionFactory1< P1 > GetFunctionFactory( void (*)(P1) )
	{
		return VoidFunctionFactory1< P1 >;
	}

	/// Takes one parameter (P1) and returns R.
	template< typename R, typename P1 >
	struct FunctionFactory1
	{
		template< R (*Fun)(P1) >
		static inline int MakeLuaCallback( lua_State* L )
		{
			CHECK_PARAM_COUNT(1);
			CHECK_PARAM_TYPE(1, P1);
			P1 p1 = toType< P1 >( L, 1 );
			R v = (*Fun)( p1 );
			pushLua( L, v );
			return 1;
		}
	};
	template< typename R, typename P1 >
	static inline FunctionFactory1< R, P1 > GetFunctionFactory( R (*)(P1) )
	{
		return FunctionFactory1< R, P1 >;
	}

	/// Takes 2 parameters (P1 and P2) and returns R.
	///
	template< typename R, typename P1, typename P2 >
	struct FunctionFactory2
	{
		template< R (*Fun)(P1,P2) >
		static inline int MakeLuaCallback( lua_State* L )
		{
			CHECK_PARAM_COUNT(2);
			CHECK_PARAM_TYPE(1, P1);
			CHECK_PARAM_TYPE(2, P2);
			P1 p1 = toType< P1 >( L, 1 );
			P2 p2 = toType< P2 >( L, 2 );
			R v = (*Fun)( p1, p2 );
			pushLua( L, v );
			return 1;
		}
	};
	template< typename R, typename P1, typename P2 >
	static inline FunctionFactory2< R, P1, P2 > GetFunctionFactory( R (*)(P1,P2) )
	{
		return FunctionFactory2< R, P1, P2 >;
	}


	/// Takes 3 parameters (P1, P2 and P3) and returns R.
	///
	template< typename R, typename P1, typename P2, typename P3 >
	struct FunctionFactory3
	{
		template< R (*Fun)(P1,P2,P3) >
		static inline int MakeLuaCallback( lua_State* L )
		{
			CHECK_PARAM_COUNT(3);
			CHECK_PARAM_TYPE(1, P1);
			CHECK_PARAM_TYPE(2, P2);
			CHECK_PARAM_TYPE(3, P3);
			P1 p1 = toType< P1 >( L, 1 );
			P2 p2 = toType< P2 >( L, 2 );
			P3 p3 = toType< P3 >( L, 3 );
			R v = (*Fun)( p1, p2, p3 );
			pushLua( L, v );
			return 1;
		}
	};
	template< typename R, typename P1, typename P2, typename P3 >
	static inline FunctionFactory3< R, P1, P2, P3 > GetFunctionFactory( R (*)(P1,P2,P3) )
	{
		return FunctionFactory3< R, P1, P2, P3 >;
	}

	//////////////////////////////////////////////////////////////////////////
	template< typename R, typename P1, typename P2, typename P3, typename P4 >
	struct FunctionFactory4
	{
		template< R (*Fun)(P1,P2,P3,P4) >
		static inline int MakeLuaCallback( lua_State* L )
		{
			CHECK_PARAM_COUNT(4);
			CHECK_PARAM_TYPE(1, P1);
			CHECK_PARAM_TYPE(2, P2);
			CHECK_PARAM_TYPE(3, P3);
			CHECK_PARAM_TYPE(4, P4);
			P1 p1 = toType< P1 >( L, 1 );
			P2 p2 = toType< P2 >( L, 2 );
			P3 p3 = toType< P3 >( L, 3 );
			P4 p4 = toType< P4 >( L, 4 );
			R v = (*Fun)( p1, p2, p3, p4 );
			pushLua( L, v );
			return 1;
		}
	};
	template< typename R, typename P1, typename P2, typename P3, typename P4 >
	static inline FunctionFactory4< R, P1, P2, P3, P4 > GetFunctionFactory( R (*)(P1,P2,P3,P4) )
	{
		return FunctionFactory4< R, P1, P2, P3, P4 >;
	}


	//////////////////////////////////////////////////////////////////////////
	template< typename R, typename P1, typename P2, typename P3, typename P4, typename P5 >
	struct FunctionFactory5
	{
		template< R (*Fun)(P1,P2,P3,P4,P5) >
		static inline int MakeLuaCallback( lua_State* L )
		{
			CHECK_PARAM_COUNT(5);
			CHECK_PARAM_TYPE(1, P1);
			CHECK_PARAM_TYPE(2, P2);
			CHECK_PARAM_TYPE(3, P3);
			CHECK_PARAM_TYPE(4, P4);
			CHECK_PARAM_TYPE(5, P5);
			P1 p1 = toType< P1 >( L, 1 );
			P2 p2 = toType< P2 >( L, 2 );
			P3 p3 = toType< P3 >( L, 3 );
			P4 p4 = toType< P4 >( L, 4 );
			P5 p5 = toType< P5 >( L, 5 );
			R v = (*Fun)( p1, p2, p3, p4, p5 );
			pushLua( L, v );
			return 1;
		}
	};
	template< typename R, typename P1, typename P2, typename P3, typename P4, typename P5 >
	static inline FunctionFactory5< R, P1,P2,P3,P4,P5 > GetFunctionFactory( R (*)(P1,P2,P3,P4,P5) )
	{
		return FunctionFactory5< R, P1,P2,P3,P4,P5 >;
	}


}//namespace LuaWrapper



#undef CHECK_PARAM_COUNT
#undef CHECK_PARAM_TYPE

#endif



#if mxSUPPORTS_VARIADIC_TEMPLATES
#define TO_LUA_CALLBACK( FUNCTION )\
	(&(LuaWrapper::Callable_From_Lua< decltype(&FUNCTION), &FUNCTION>))
#else
#define TO_LUA_CALLBACK( FUNCTION )\
	(&(LuaWrapper::GetFunctionFactory( &FUNCTION ).MakeLuaCallback< &FUNCTION >))
#endif


//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
