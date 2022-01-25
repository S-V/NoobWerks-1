#include <Core/Core_PCH.h>
#pragma hdrstop
#include <Core/Core.h>
#include <Core/Memory.h>
#include <Core/Memory/MemoryHeaps.h>
#include <Core/Util/Tweakable.h>

#if MX_EDITOR

namespace Tweaking
{
	namespace
	{
		struct PrivateData
		{
			TweakableVars	tweakables;
			ProxyAllocator	proxyAllocator;
		public:
			PrivateData()
				: proxyAllocator( "Tweaking", MemoryHeaps::global() )
				, tweakables( proxyAllocator )
			{
			}
		};

		static TPtr< PrivateData >	gData;
	}//namespace

	ERet Setup()
	{
		gData.ConstructInPlace();
		mxDO(gData->tweakables.resize(1024));
		return ALL_OK;
	}

	void Close()
	{
		gData->tweakables.Clear();
		gData.Destruct();
	}

	TweakableVars& GetAllTweakables()
	{
		return gData->tweakables;
	}

	static void initTweakableVar( TweakableVar &newTweakableVar_,
		TweakableVar::Type type, void * ptr, const char* name, const char* file, int line )
	{
		newTweakableVar_.type = type;
		newTweakableVar_.name = name;
		newTweakableVar_.file = file;
		newTweakableVar_.line = line;
		newTweakableVar_.ptr = ptr;
	}

	void tweakVariable( bool * var, const char* name, const char* file, int line )
	{
		TweakableVar* existingEntry = gData->tweakables.FindValue( var );
		if( !existingEntry )
		{
			TweakableVar	newEntry;
			initTweakableVar( newEntry, TweakableVar::Bool, var, name, file, line );

			gData->tweakables.Insert( var, newEntry );
		}
	}

	void tweakVariable( int * var, const char* name, const char* file, int line )
	{
		TweakableVar* existingEntry = gData->tweakables.FindValue( var );
		if( !existingEntry )
		{
			TweakableVar	newEntry;
			initTweakableVar( newEntry, TweakableVar::Int, var, name, file, line );

			gData->tweakables.Insert( var, newEntry );
		}
	}

	void tweakVariable( float * var, const char* name, const char* file, int line )
	{
		TweakableVar* existingEntry = gData->tweakables.FindValue( var );
		if( !existingEntry )
		{
			TweakableVar	newEntry;
			initTweakableVar( newEntry, TweakableVar::Float, var, name, file, line );

			gData->tweakables.Insert( var, newEntry );
		}
	}

	void tweakVariable( double * var, const char* name, const char* file, int line )
	{
		TweakableVar* existingEntry = gData->tweakables.FindValue( var );
		if( !existingEntry )
		{
			TweakableVar	newEntry;
			initTweakableVar( newEntry, TweakableVar::Double, var, name, file, line );

			gData->tweakables.Insert( var, newEntry );
		}
	}

	void tweakVariable(
		V3d * var
		, const char* name, const char* file, int line
		)
	{
		TweakableVar* existingEntry = gData->tweakables.FindValue( var );
		if( !existingEntry )
		{
			TweakableVar	newEntry;
			initTweakableVar( newEntry, TweakableVar::Double3, var, name, file, line );

			gData->tweakables.Insert( var, newEntry );
		}
	}

}//namespace Tweaking

#endif // MX_EDITOR

mxNO_EMPTY_FILE
