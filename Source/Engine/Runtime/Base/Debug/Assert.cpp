/*
=============================================================================
	File:	Debug.cpp
	Desc:	Code for debug utils, assertions and other very useful things.
	ToDo:	print stack trace
=============================================================================
*/

#include <Base/Base_PCH.h>
#pragma hdrstop
#include <Base/Base.h>

namespace Debug
{
	enum
	{
		MAX_ASSERTION_TEXT_BUFFER_SIZE = mxKiB(4)
	};

	template< int SIZE >
	int tMy_snprintfcat( char (&buf)[SIZE], char const* fmt, ... )
	{
		int result;
		va_list args;
		int len = strnlen( buf, SIZE);

		va_start( args, fmt);
		result = My_vsnprintf( buf + len, SIZE - len, fmt, args);
		va_end( args);

		return result + len;
	}

	static EAssertBehavior DefaultUserAssertCallback(
		const char* _file, int _line, const char* _function,
		const char* _expression,
		const char* _format, va_list _args
		)
	{
#if (mxPLATFORM == mxPLATFORM_WINDOWS)

		char		buffer[ MAX_ASSERTION_TEXT_BUFFER_SIZE ];

		const int	short_length = sprintf_s( buffer,
			"%s(%d): Assertion failed in '%s', '%s'\n\n",
			_file, _line, _expression, _function ? _function : "<?>"
		);

		const int	message_length = _vsnprintf_s(
			buffer + short_length, sizeof(buffer) - short_length, _TRUNCATE,
			_format, _args
		);

		mxGetLog().VWrite( LL_Warn, buffer, short_length + message_length );
		mxGetLog().Flush();

		::MessageBeep(0);

		if( ::IsDebuggerPresent() )
		{
			//sprintf_s( buffer, "Assertion failed:\n\n '%s'\n\n in _file %s, function '%s', _line %d, %s.\n",
			//	_expression, _file, _function, _line, message ? message : "no additional info" );
			strcat_s( buffer, "\n\nDo you wish to debug?\nYes - 'Debug Break', No - 'Exit', Cancel or Close - 'Don't bother me again!'\n" );
			const int result = ::MessageBoxA( NULL, buffer, "Assertion failed", MB_YESNOCANCEL | MB_ICONERROR );
			if( IDYES == result ) {
				return AB_Break;
			}
			elif( IDCANCEL == result ) {
				return AB_Ignore;
			}
		}

		return AB_FatalError;
#else

	#error Unsupported platform!

#endif

	}

	static UserAssertCallback userAssertCallback = &DefaultUserAssertCallback;

}//Debug

UserAssertCallback SetUserAssertCallback( UserAssertCallback cb )
{
	UserAssertCallback prevCallback
		= (Debug::userAssertCallback == Debug::DefaultUserAssertCallback)
		? NULL
		: Debug::userAssertCallback
		;

	Debug::userAssertCallback = cb ? cb : Debug::DefaultUserAssertCallback;

	return prevCallback;
}

#if MX_DEBUG
	void mxVARARGS DBGOUT_IMPL( const char* format, ... )
	{
		if( !IsInDebugger() ) {
			return;
		}
		va_list	args;
		va_start( args, format );
		mxGetLog().PrintV( LL_Debug, format, args );
		va_end( args );
	}
#endif // MX_DEBUG

/*
================================
		ZZ_OnAssertionFailed
================================
*/
int ZZ_OnAssertionFailed(
	const char* _file, int _line, const char* _function,
	const char* expression,
	const char* message, ...
	)
{
	// If we're exiting, don't report errors.
	if(mxUNLIKELY( G_isExitRequested() ))
	{
		return AB_Continue;
	}

	va_list	args;
	va_start( args, message );
	const EAssertBehavior action = (*Debug::userAssertCallback)(
		_file, _line, _function,
		expression,
		message, args
	);
	va_end( args );

	if( action == AB_FatalError )
	{
		ptERROR( "Assertion failed in %s(%d): '%s'", _file, _line, expression );
		return true;
	}

	return action;
}
