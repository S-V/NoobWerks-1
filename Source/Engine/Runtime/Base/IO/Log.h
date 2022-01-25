/*
=============================================================================
	File:	Log.h
	Desc:	Logging.
	References:
	"Efficient logging in multithreaded C++ server" by Shuo Chen
=============================================================================
*/
#pragma once

#include <Base/Template/Containers/LinkedList/TLinkedList.h>

///
///	ELogLevel - specifies the importance of logging information.
///	See: http://stackoverflow.com/questions/7839565/logging-levels-logback-rule-of-thumb-to-assign-log-levels
///
enum ELogLevel
{
	LL_Trace,	// An extremely detailed and verbose information. (Begin method X, end method X etc.)
	LL_Debug,	// A detailed information on the flow through the system. (Executed queries, user authenticated, session expired.)
	LL_Info,	// An informational message. (Normal behavior like mail sent, user updated profile etc.)
	LL_Warn,	// A warning, used for non critical failures. (Incorrect behavior but the application can continue.)
	LL_Error,	// A normal error, used for potential serious failures. (For example application crashes / exceptions.)
	//LL_Assert,
	LL_Fatal,	// A critical alert, used for failures which are threats to stability. (Highest level: important stuff down.)
	LL_Default = LL_Info
};

enum ELogColor
{
	LC_BLACK        = 0,
	LC_DARK_BLUE    = 1,
	LC_DARK_GREEN   = 2,
	LC_DARK_GYAN    = 3,
	LC_DARK_RED     = 4,
	LC_DARK_MAGENTA = 5,
	LC_DARK_YELLOW  = 6,
	LC_GRAY         = 7,
	LC_DARK_GRAY    = 8,
	LC_BLUE         = 9,
	LC_GREEN        = 10,
	LC_CYAN         = 11,
	LC_RED          = 12,
	LC_MAGENTA      = 13,
	LC_YELLOW       = 14,
	LC_WHITE        = 15  
};
struct LogSource : FileLine
{
	ELogColor	color;
};

enum EControlFlow
{
	CF_Continue = 0,	//!< continue execution
	CF_Break = 1,		//!< trigger a breakpoint
};

typedef EControlFlow F_LogCallback(
								   const char* _fmt, va_list _args,
								   const ELogLevel _level,
								   // these are valid only if debug information is present:
								   const char* _file, unsigned int _line,
								   const char* _function
								   );

/// Registers a function to be called on log messages. Pass NULL to remove the log handler.
void TbSetLogCallback( ELogLevel _level, F_LogCallback* _callback );


EControlFlow TbLog(
				   const char* _file, unsigned int _line,
				   const char* _function,
				   const ELogLevel _level,
				   const char* _fmt, ...
				   );

/*
=================================================================
	A very simple logging device.
	
	Todo:	Push/Pop log levels?
			Overload '<<' for vectors,matrices, etc?
			Pass string sizes to improve speed?
=================================================================
*/

///
///	ALog - base class for logging devices.
///
struct ALog
	: public TLinkedList< ALog >
	, NonCopyable
{
	virtual void VWrite( ELogLevel _level, const char* _message, int _length ) = 0;

	void PrintV( ELogLevel level, const char* fmt, va_list args );
	void PrintF( ELogLevel level, const char* fmt, ... );

	/// Writes any pending output.
	virtual void Flush() {}

	/// Prepares for destruction.
	virtual void Close() {}

	virtual void VWriteLinePrefix( ELogLevel _level, const char* _prefix, int _length )
	{
		this->VWrite( _level, _prefix, _length );
	}

protected:
	ALog();
	virtual ~ALog();
};

///
///	ALogManager
///
class ALogManager: public ALog
{
public:
	virtual bool Attach( ALog* logger ) = 0;
	virtual bool Detach( ALog* logger ) = 0;
	virtual bool IsRedirectingTo( ALog* logger ) = 0;

	/// Set the level below which messages are ignored.
	virtual void SetThreshold( ELogLevel _level ) = 0;

	//// Enters a new (optionally) named scope.
	//virtual void PushScope(const char* name) = 0;

	//// Exit the current scope.
	//virtual void PopScope() = 0;

protected:
	virtual	~ALogManager()
	{}
};

mxREMOVE_THIS
ALogManager& mxGetLog();

/*
//
//	LogManipulator - controls behaviour of a logging device.
//
class LogManipulator
{
public:
	virtual ALog & operator () ( ALog& logger ) const = 0;
};

// executes the log manipulator on the logger (the stream)
//
inline ALog & operator << ( ALog& logger, const LogManipulator& manip )
{
	manip( logger );
	return logger;
}

	//virtual void PushLevel();
	//virtual void PopLevel();
	virtual void SetSeverity( ELogLevel level );

//
//	LogLevel
//
//	Usage:
//
//	Logger::get_ref() << LogLevel(LL_Info) << "X = " << X << endl;
//
class LogLevel: public LogManipulator
{
	const ELogLevel	mLogLevel;

public:
	LogLevel( ELogLevel ll )
		: mLogLevel( ll )
	{}
	virtual ALog & operator () ( ALog& logger ) const
	{
		logger.SetLogLevel( mLogLevel );
		return logger;
	};
};

//
//	ScopedLogLevel - automatically restores saved logging level.
//
class ScopedLogLevel
{
	ALog &	mLogger;
	ELogLevel	mSavedLogLevel;

public:
	ScopedLogLevel( ALog& logger )
		: mLogger( logger ), mSavedLogLevel( logger.GetLogLevel() )
	{}
	~ScopedLogLevel()
	{
		mLogger.SetLogLevel( mSavedLogLevel );
	}
};
*/

/*
-------------------------------------------------------------------------
	ATextStream/ATextOutput

The API should feel like streams to relieve users from string formatting issues.

The logical record can be defined as the longest sequence of chained calls to stream insertion operator,
so that the following expression:
log << a << b << c << " and " << x() << y() << z();
defines a logging unit that is atomic in the resulting log output.

See: Atomic Log Stream for C++ http://www.inspirel.com/articles/Atomic_Log_Stream.html
-------------------------------------------------------------------------
*/
class ATextStream {
public:
	/// all text writing is redirected to this function
	virtual void VWrite( const char* text, int length ) = 0;

	/// Outputs a string with a variable arguments list.
	ATextStream& PrintV( const char* fmt, va_list args );

	/// Outputs formatted data.
	ATextStream& PrintF( const char* fmt, ... );

	/// Outputs a hex address.
	ATextStream& operator << (const void* p);

	/// Outputs a bool.
	ATextStream& operator << (bool b);

	/// Outputs a char.
	ATextStream& operator << (char c);

	/// Outputs a string.
	ATextStream& operator << (const char* s);

	/// Outputs a string.
	ATextStream& operator << (const signed char* s);

	/// Outputs a string.
	ATextStream& operator << (const unsigned char* s);

	/// Outputs a short.
	ATextStream& operator << (short s);

	/// Outputs an unsigned short
	ATextStream& operator << (unsigned short s);

	/// Outputs an int
	ATextStream& operator << (int i);

	/// Outputs an unsigned int.
	ATextStream& operator << (unsigned int u);

	/// Outputs a float.
	ATextStream& operator << (float f);

	/// Outputs a double.
	ATextStream& operator << (double d);

	/// Outputs a 64 bit int.
	ATextStream& operator << (INT64 i);

	/// Outputs a 64 bit unsigned int.
	ATextStream& operator << (UINT64 u);

	/// Outputs a String.
	ATextStream& operator << (const String& _str);

	ATextStream& operator << (const Chars& _str);

	ATextStream& Repeat( char c, int count );

public:

	// I overload the comma operator to reduce typing and avoid "Chevron Hell":
	// http://ithare.com/cppcon2017-day-1-hope-to-get-something-better-than-chevrone-hell/

	template< typename TYPE >
	mxFORCEINLINE ATextStream & operator , ( const TYPE& o )
	{
		return *this << o;
	}

protected:
	ATextStream() {}
	virtual ~ATextStream() {}
};

class StringStream: public ATextStream {
protected:
	String & 	m_string;
public:
	StringStream( String &_string );
	virtual ~StringStream();

	//=-- ATextStream
	virtual void VWrite( const char* text, int length ) override;

	////==- AWriter
	//virtual ERet Write( const void* buffer, size_t size ) override;

	const String& GetString() const;

	PREVENT_COPY(StringStream);
};

class StringWriter: public AWriter {
protected:
	String & 	m_string;
public:
	StringWriter( String &_string );
	virtual ~StringWriter();

	//==- AWriter
	virtual ERet Write( const void* buffer, size_t size ) override;

	const String& GetString() const;

	PREVENT_COPY(StringWriter);
};

/// Distributes log messages to their destination(s) (e.g. a text log or a byte stream).
/// Records formatting in a private buffer and flushes the resulting buffer in the destructor.
class LogStream: public StringStream {
public:
	String512 		m_buffer;
	const ELogLevel	m_level;
public:
	LogStream( ELogLevel level = LL_Info );
	virtual ~LogStream();

	void Flush();

	PREVENT_COPY(LogStream);
};

///
///	TextStream - provides formatted text output.
///
class TextStream: public ATextStream {
protected:
	AWriter &	mStream;
public:
	explicit TextStream( AWriter& stream );
	virtual ~TextStream();

	//=-- ATextStream
	virtual void VWrite( const char* text, int length ) override;

public_internal:
	AWriter & GetStream() { return mStream; }

	PREVENT_COPY(TextStream);
};

class StdOut_Log: public ALog
{
public:
	StdOut_Log();
	~StdOut_Log();

	virtual	void VWrite( ELogLevel _level, const char* _message, int _length ) override;
};


template< typename TYPE >
void DBG_PrintArray( const TYPE* _items, int _count, ATextStream &_log )
{
	for( int i = 0; i < _count; i++ )
	{
		_log << "[" << i << "]: " << _items[i] << '\n';
	}
}

//----------------------------------------------------------//
//				End Of File.									//
//----------------------------------------------------------//
