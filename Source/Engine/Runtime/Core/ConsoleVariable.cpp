#include <Core/Core_PCH.h>
#pragma hdrstop
#include <Core/Core.h>
#include <Core/ConsoleVariable.h>

//AConsoleVariable* AConsoleVariable::s_list = nil;

const AConsoleVariable* AConsoleVariable::FindCVar( const char* _name )
{
	const AConsoleVariable* current = AConsoleVariable::Head();
	while(PtrToBool( current ))
	{
		if( 0 == strcmp( current->m_name.c_str(), _name ) ) {
			return current;
		}
		// move to the next statically declared cvar
		current = current->Next();
	}
	return nil;
}

ERet AConsoleVariable::SetInt( const AConsoleVariable* _var, const int _value )
{
	void* valuePtr = mxAddByteOffset( c_cast(char*)_var, CONVAR_VALUE_OFFSET );
	mxDO(Reflection::SetIntegerValue( valuePtr, _var->m_type, _value ));
	return ALL_OK;
}
ERet AConsoleVariable::SetBool( const AConsoleVariable* _var, const bool _value )
{
	void* valuePtr = mxAddByteOffset( c_cast(char*)_var, CONVAR_VALUE_OFFSET );
	mxDO(Reflection::SetBooleanValue( valuePtr, _var->m_type, _value ));
	return ALL_OK;
}

namespace cvars
{
TCVar< U32 >	LogLevel("LogLevel", 0);
TCVar< U32 >	FrameStackSizeMb("FrameStackSizeMb", 4);
TCVar< U32 >	NumberOfWorkerThreads("NumberOfWorkerThreads", 0);
}//namespace cvars

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
