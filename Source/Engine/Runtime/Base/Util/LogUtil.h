/*
=============================================================================
	File:	LogUtil.h
	Desc:	
=============================================================================
*/
#pragma once

//----------------------------------------------------------//
//	Logging.
//----------------------------------------------------------//

void F_Util_ComposeLogFileName( String256 & log_file_name );

// writes current date and time, system info, etc.
void mxUtil_StartLogging( ALog* logger );

// writes current date and time, performance stats, etc.
void mxUtil_EndLogging( ALog* logger );

//----------------------------------------------------------//

//
//	mxLogger_FILE - is an ANSI file logger.
//
class mxLogger_FILE: public ALog
{
	FILE *	mFile;

public:
	mxLogger_FILE()
		: mFile( nil )
	{}
	mxLogger_FILE( const char* fileName )
		: mFile( nil )
	{
		this->Open( fileName );
	}
	~mxLogger_FILE()
	{
		this->Close();
	}
	bool Open( const char* filename, bool eraseExisting = true )
	{
		mxASSERT_PTR(filename);
		mxASSERT(!this->IsOpen());
		mFile = ::fopen( filename, eraseExisting ? "w" : "a" );
		if( !mFile ) {
			ptERROR("Failed to open log file '%s' for writing", filename);
			return false;
		}
		return true;
	}
	bool IsOpen() const {
		return (mFile != nil);
	}
	virtual void VWrite( ELogLevel _level, const char* _message, int _length ) override
	{
		chkRET_IF_NOT(IsOpen());
		if( mFile )
		{
			::fwrite( _message, sizeof(_message[0]), _length, mFile );
			::fwrite( "\n", sizeof(char), 1, mFile );
			::fflush( mFile );
		}
	}
	virtual void Flush() override
	{
		if( mFile )
		{
			::fflush( mFile );
		}
	}
	virtual void Close() override
	{
		if( this->IsOpen() )
		{
			::fflush( mFile );
			::fclose( mFile );
			mFile = nil;
		}
		mxASSERT(!this->IsOpen());
	}
};

class FileLogUtil : DependsOn_Base
{
	mxLogger_FILE	m_fileLog;

public:
	FileLogUtil()
	{}
	FileLogUtil(EInitDefault)
	{
		String256	log_file_name;
		F_Util_ComposeLogFileName( log_file_name );
		this->OpenLog( log_file_name.raw() );
	}
	explicit FileLogUtil( const char* log_file_name )
	{
		this->OpenLog( log_file_name );
	}
	~FileLogUtil()
	{
		this->CloseLog();
	}

	void OpenLog( const char* log_file_name )
	{
		mxASSERT(!m_fileLog.IsOpen());
		bool bOK = m_fileLog.Open( log_file_name );
		if( !bOK ) {
			ptWARN( "Failed to create log file '%s' for writing.\n", log_file_name );
			return;
		}

		ALogManager& logMgr = mxGetLog();
		logMgr.Attach( &m_fileLog );

		mxUtil_StartLogging( &logMgr );
	}
	void CloseLog()
	{
		mxASSERT(m_fileLog.IsOpen());
		ALogManager& logMgr = mxGetLog();

		mxUtil_EndLogging( &logMgr );

		logMgr.Detach( &m_fileLog );
	}
	PREVENT_COPY(FileLogUtil);
};

//----------------------------------------------------------//
//				End Of File.									//
//----------------------------------------------------------//
