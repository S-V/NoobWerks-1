#pragma once

struct AConsoleVariable : TAutoList< AConsoleVariable >
{
	const TbMetaType &	m_type;
	const Chars			m_name;

public:
	inline AConsoleVariable( const TbMetaType& _type, const Chars& _name )
		: m_type( _type ), m_name( _name )
	{
	}

	static const AConsoleVariable* FindCVar( const char* _name );

	static ERet SetInt( const AConsoleVariable* _var, const int _value );
	static ERet SetBool( const AConsoleVariable* _var, const bool _value );
};

enum { CONVAR_VALUE_OFFSET = sizeof(AConsoleVariable) };

template< typename TYPE >
struct TCVar : AConsoleVariable
{
	TYPE	value;
	//TYPE	m_min;
	//TYPE	m_max;

public:
	inline TCVar( const Chars& _name, const TYPE& _default_value )
		: AConsoleVariable( T_DeduceTypeInfo< TYPE >(), _name )
		, value( _default_value )
	{
		mxSTATIC_ASSERT( OFFSET_OF(TCVar, value) == CONVAR_VALUE_OFFSET );
	}
};

namespace cvars
{
//; Indicates the level of logging output, from 0 to 3. Higher values show more information.
//; 0 - no log
//; 1 - info 
//; 2 - debug (only if exe was compiled with debug info)
//; 3 - trace (only if exe was compiled with debug info)
extern TCVar< U32 >	LogLevel;

//; super-fast linear stack-based single-frame scratch memory allocator
//; capacity in megabytes, several MiBs is a good size
//; large sizes are needed for tools
extern TCVar< U32 >	FrameStackSizeMb;

//; 0 = auto, max = 16, high number doesn't make sense because of contention
extern TCVar< U32 >	NumberOfWorkerThreads;

}//namespace cvars

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
