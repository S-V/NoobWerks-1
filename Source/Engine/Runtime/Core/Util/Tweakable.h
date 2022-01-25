/*
=============================================================================
	File:	Tweakable.h
	Desc:	Tweakable constants for debugging and rapid prototyping.
	See:

	http://blogs.msdn.com/b/shawnhar/archive/2009/05/01/motogp-tweakables.aspx

	http://www.gamedev.net/page/resources/_/technical/game-programming/tweakable-constants-r2731
	http://www.gamedev.net/topic/559658-tweakable-constants/

	https://mollyrocket.com/forums/viewtopic.php?p=3355
	https://mollyrocket.com/forums/viewtopic.php?t=556

	http://www.pouet.net/topic.php?which=7126&page=1&x=19&y=10

	http://gamesfromwithin.com/remote-game-editing

=============================================================================
*/
#pragma once

#include <Base/Template/Containers/HashMap/TPointerMap.h>
#include <Base/Math/MiniMath.h>	// V3d


//--------------------------------------------------
//	Definitions of useful macros.
//--------------------------------------------------


/// replaces nwTWEAKABLE_CONST once the constant is determined
#define nwNON_TWEAKABLE_CONST( TYPE, NAME, INITIAL_VALUE )\
	const TYPE NAME = INITIAL_VALUE;


#if MX_DEVELOPER

	// NOTE: use only on static variables
	#define HOT_VAR( x )		Tweaking::tweakVariable( &x, #x, __FILE__, __LINE__ )
	#define HOT_VAR2( x, name )	Tweaking::tweakVariable( &x, name, __FILE__, __LINE__ )

	#define nwTWEAKABLE_CONST( TYPE, NAME, INITIAL_VALUE )\
		static TYPE NAME = INITIAL_VALUE;\
		Tweaking::tweakVariable( &NAME, #NAME, __FILE__, __LINE__ )

#else

	#define HOT_VAR( x )
	#define HOT_VAR2( x, name )

	#define nwTWEAKABLE_CONST( TYPE, NAME, INITIAL_VALUE )\
		nwNON_TWEAKABLE_CONST(TYPE, NAME, INITIAL_VALUE)

#endif // MX_DEVELOPER


#if MX_DEVELOPER

/*
-----------------------------------------------------------------------------
	Tweaking
-----------------------------------------------------------------------------
*/
namespace Tweaking
{
	struct TweakableVar
	{
		enum Type
		{
			Bool,
			Int,
			Float,
			Double,
			Double3,
		};
		Type		type;
		const char*	name;
		const char*	file;
		int			line;
		union
		{
			bool *		b;
			int *		i;
			float *		f;
			double *	d;
			void *		ptr;
			V3d *		v3d;
		};
	};

	typedef TPointerMap< TweakableVar > TweakableVars;

	ERet Setup();
	void Close();

	void tweakVariable( bool * var, const char* name, const char* file, int line );
	void tweakVariable( int * var, const char* name, const char* file, int line );
	void tweakVariable( float * var, const char* name, const char* file, int line );
	void tweakVariable( double * var, const char* name, const char* file, int line );

	void tweakVariable(
		V3d * var
		, const char* name, const char* file, int line
		);

	TweakableVars& GetAllTweakables();

}//namespace Tweaking

#endif // MX_DEVELOPER
