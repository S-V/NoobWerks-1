/*
=============================================================================
	File:	Log.cpp
	Desc:	Log.
	ToDo:	cleanup, remove redundancy/complexity.
=============================================================================
*/
#include <Base/Base_PCH.h>
#pragma hdrstop
#include <Base/Base.h>

#include <inttypes.h>

#include <grisu/grisu3.h>
#pragma comment(lib,"grisu.lib")	// hotfix for MVC++ 2010


ALog::ALog()
{
	//
}

ALog::~ALog()
{
	//mxASSERT2( !mxGetLog().IsRedirectingTo( this ), "Did you forget to call ALogManager.Detach()?" );
}

EControlFlow DefaultLogCallback(
								const char* _fmt, va_list _args,
								const ELogLevel _level,
								const char* _file, unsigned int _line,
								const char* _function
								)
{
	if( _level > LL_Warn ) {
		if( IsInDebugger() ) {
			return CF_Break;
		}
	}
	return CF_Continue;
}

static F_LogCallback* g_userLogCallback = &DefaultLogCallback;

void TbSetLogCallback( ELogLevel _level, F_LogCallback* _callback )
{
	g_userLogCallback = _callback;
}

EControlFlow TbLog(
				   const char* _file, unsigned int _line,
				   const char* _function,
				   const ELogLevel _level,
				   const char* _fmt, ...
				   )
{
	mxASSERT_PTR(_fmt);
	va_list	args;
	va_start( args, _fmt );
	mxGetLog().PrintV( _level, _fmt, args );
	va_end( args );

	if( g_userLogCallback )
	{
		return (*g_userLogCallback)( _fmt, args, _level, _file, _line, _function );
	}
	return CF_Continue;
}

static class MainLogManager: public ALogManager, TSingleInstance< MainLogManager >
{
	ALog::Head		m_loggers;
	ELogLevel		m_threshhold;
	SpinWait		m_CS;
public:
	MainLogManager()
	{
		m_loggers = nil;
		m_threshhold = LL_Trace;
		m_CS.Initialize();
	}
	~MainLogManager()
	{
		m_CS.Shutdown();
	}
	virtual	void VWrite( ELogLevel _level, const char* _message, int _length ) override
	{
		mxASSERT_PTR(_message);
		mxASSERT(_length > 0);

		SpinWait::Lock	scopedLock( m_CS );

		if( _level >= m_threshhold )
		{
			//for debug output inside IDE
			::OutputDebugStringA( _message );
			::OutputDebugStringA( "\n" );

			ALog* current = m_loggers;
			while( current ) {
				current->VWrite( _level, _message, _length );
				current = current->_next;
			}
		}

		if( _level == ELogLevel::LL_Error )
		{
			static bool s_show_message_box = true;
			if( s_show_message_box )
			{
				const int result = ::MessageBoxA( nil, _message,
					"An unexpected error occurred - Do you want to exit?\n(Cancel - don't show this message box anymore.)",
					MB_YESNOCANCEL
				);
				if( IDYES == result ) {
					mxForceExit(-1);
				}
				if( IDCANCEL == result ) {
					s_show_message_box = false;
				}
			}
		}
		if( _level == ELogLevel::LL_Fatal ) {
			::MessageBoxA( nil, _message, "A critical error occurred,\nthe program will now exit.", MB_OK );
			mxForceExit(-1);
		}
	}
	virtual void Flush() override
	{
		SpinWait::Lock	scopedLock( m_CS );

		ALog* current = m_loggers;
		while( current ) {
			current->Flush();
			current = current->_next;
		}
	}
	virtual void Close() override
	{
		SpinWait::Lock	scopedLock( m_CS );

		ALog* current = m_loggers;
		while( current ) {
			current->Close();
			current = current->_next;
		}
	}
	virtual bool Attach( ALog* logger ) override
	{
		chkRET_FALSE_IF_NIL(logger);
		logger->AppendSelfToList( &m_loggers );
		return true;
	}
	virtual bool Detach( ALog* logger ) override
	{
		chkRET_FALSE_IF_NIL(logger);
		logger->RemoveSelfFromList( &m_loggers );
		return true;
	}
	virtual bool IsRedirectingTo( ALog* logger ) override
	{
		chkRET_FALSE_IF_NIL(logger);
		return logger->FindSelfInList(m_loggers);
	}
	virtual void SetThreshold( ELogLevel _level ) override
	{
		m_threshhold = _level;
	}
} g_MainLogManager;

ALogManager& mxGetLog()
{
	return g_MainLogManager;
}

/*================================
			ALog
================================*/

void ALog::PrintV( ELogLevel level, const char* fmt, va_list args )
{
	ptPRINT_VARARGS_BODY( fmt, args, this->VWrite(level, ptr_, len_) );
}
void ALog::PrintF( ELogLevel level, const char* fmt, ... )
{
	va_list	args;
	va_start( args, fmt );
	/*int len = */this->PrintV( level, fmt, args );
	va_end( args );
	/*mxUNUSED(len);*/
}

/*
-------------------------------------------------------------------------
	ATextStream
-------------------------------------------------------------------------
*/
ATextStream& ATextStream::PrintV( const char* fmt, va_list args )
{
	ptPRINT_VARARGS_BODY( fmt, args, this->VWrite(ptr_, len_) );
	return *this;
}

ATextStream& ATextStream::PrintF( const char* fmt, ... )
{
	va_list args;
	va_start(args, fmt);
	this->PrintV( fmt, args );
	va_end(args);
	return *this;
}

ATextStream& ATextStream::operator << (const void* p)
{
	this->PrintF("%p",p);
	return *this;
}

ATextStream& ATextStream::operator << (bool b)
{
	return (*this) << (b ? Chars("true") : Chars("false"));
}

ATextStream& ATextStream::operator << (char c)
{
	this->PrintF("%c",c);
	return *this;
}

ATextStream& ATextStream::operator << (const char* s)
{
	this->PrintF("%s",s);
	return *this;
}

ATextStream& ATextStream::operator << (const signed char* s)
{
	this->PrintF("%s",s);
	return *this;
}

ATextStream& ATextStream::operator << (const unsigned char* s)
{
	this->PrintF("%s",s);
	return *this;
}

ATextStream& ATextStream::operator << (short s)
{
	this->PrintF("%d",(INT)s);
	return *this;
}

ATextStream& ATextStream::operator << (unsigned short s)
{
	this->PrintF("%u",(UINT)s);
	return *this;
}

ATextStream& ATextStream::operator << (int i)
{
	this->PrintF("%d",i);
	return *this;
}

ATextStream& ATextStream::operator << (unsigned int u)
{
	this->PrintF("%u",u);
	return *this;
}

ATextStream& ATextStream::operator << (float f)
{
	this->PrintF("%f",f);
	return *this;
}

ATextStream& ATextStream::operator << (double d)
{
#if 0
	this->PrintF("%lf",d);
#else
	char buf[32];
	int len = dtoa_grisu3( d, buf );
	this->VWrite( buf, len );
#endif
	return *this;
}

ATextStream& ATextStream::operator << (INT64 i)
{
	this->PrintF("%"PRId64,i);
	return *this;
}

ATextStream& ATextStream::operator << (UINT64 u)
{
	this->PrintF("%"PRIu64,u);
	return *this;
}

ATextStream& ATextStream::operator << (const String& _str)
{
	this->VWrite( _str.c_str(), _str.rawSize() );
	return *this;
}

ATextStream& ATextStream::operator << (const Chars& _str)
{
	this->VWrite( _str.c_str(), _str.rawSize() );
	return *this;
}

ATextStream& ATextStream::Repeat( char c, int count )
{
	while( count-- ) {
		*this << c;
	}
	return *this;
}

/*================================
		StringStream
================================*/

StringStream::StringStream( String &_string )
	: m_string(_string)
{
}
StringStream::~StringStream()
{
}
void StringStream::VWrite( const char* text, int length )
{
	//@todo: insert the terminating null?
	const int offset = m_string.num();
	m_string.resize(offset + length);
	char* start = m_string.raw();
	char* destination = start + offset;
	strncpy(destination, text, length);
}
//ERet StringStream::Write( const void* buffer, size_t size )
//{
//	this->VWrite( (char*)buffer, (int)size );
//	return ALL_OK;
//}
const String& StringStream::GetString() const
{
	return m_string;
}


StringWriter::StringWriter( String &_string )
	: m_string(_string)
{
}
StringWriter::~StringWriter()
{
}
ERet StringWriter::Write( const void* buffer, size_t size )
{
	const int offset = m_string.num();
	m_string.resize(offset + size);
	char* start = m_string.raw();
	char* destination = start + offset;
	memcpy(destination, buffer, size);
	return ALL_OK;
}

const String& StringWriter::GetString() const
{
	return m_string;
}


/*================================
		LogStream
================================*/

LogStream::LogStream( ELogLevel level )
	: StringStream(m_buffer), m_level(level)
{
}
LogStream::~LogStream()
{
	this->Flush();
}
void LogStream::Flush()
{
	if( m_buffer.num() ) {
		mxGetLog().VWrite(m_level, m_buffer.raw(), m_buffer.num());
		m_buffer.empty();
	}
}

/*================================
		TextStream
================================*/

TextStream::TextStream( AWriter& stream )
	: mStream( stream )
{
}

TextStream::~TextStream()
{
}

void TextStream::VWrite( const char* text, int length )
{
	mStream.Write( text, length );
}


/*================================
		StdOut_Log
================================*/

StdOut_Log::StdOut_Log()
{
	//
}

StdOut_Log::~StdOut_Log()
{
	mxGetLog().Detach( this );
}

void StdOut_Log::VWrite( ELogLevel _level, const char* _message, int _length )
{
	//for stdout/console
	::puts(_message);
}

//----------------------------------------------------------//
//				End Of File.									//
//----------------------------------------------------------//
