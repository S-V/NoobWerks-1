#pragma once


///
///	DbgNamedObject - named object.
///
///	NOTE: its memory layout and size are different in debug and release versions!
///
template< size_t BUFFER_SIZE = 32 >
struct DbgNamedObject
{
#if MX_DEBUG || MX_DEVELOPER
		// NOTE: the name will be truncated to the size of the embedded buffer
		inline void DbgSetName( const char* _str )
		{
			chkRET_IF_NIL(_str);
			const size_t len = smallest( strlen(_str), mxCOUNT_OF(m__DebugName)-1 );
			strncpy( m__DebugName, _str, len );
			m__DebugName[len] = '\0';
		}
		inline const char* DbgGetName() const
		{
			return m__DebugName;
		}
	protected:
		inline DbgNamedObject()
		{
			m__DebugName[0] = '?';
			m__DebugName[1] = '\0';
		}
	private:
		char	m__DebugName[ BUFFER_SIZE ];
#else
		inline void DbgSetName( const char* _str )
		{
			mxUNUSED(_str);
		}
		inline const char* DbgGetName() const
		{
			return "";
		}
#endif // MX_DEBUG
};//struct DbgNamedObject
