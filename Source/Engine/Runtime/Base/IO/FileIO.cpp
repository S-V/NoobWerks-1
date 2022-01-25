/*
=============================================================================
	File:	Files.cpp
	Desc:	Files.
	ToDo:	refactor
=============================================================================
*/
#include <Base/Base_PCH.h>
#pragma hdrstop
#include <Base/Base.h>

// _stati64
#include <sys/types.h>
#include <sys/stat.h>
// _access()
#include <io.h>

// StringCchPrintf*
#include <Strsafe.h>

//#define DBGFILE( X )	X
#define DBGFILE( X )

static const char* UNKNOWN_FILE_NAME = "?";

FileTimeT FileTimeT::CurrentTime()
{
	SYSTEMTIME	sysTime;
	::GetSystemTime( & sysTime );
	FILETIME	fileTime;
	BOOL ret = ::SystemTimeToFileTime( &sysTime, & fileTime );
	mxASSERT(ret != 0);
	return FileTimeT( fileTime );
}

using namespace OS::IO;

/*
-------------------------------------------------------------------------
	FileReader
-------------------------------------------------------------------------
*/
FileReader::FileReader()
{
	mHandle = INVALID_HANDLE_VALUE;
	mMappedData = nil;
}

FileReader::FileReader( const char* _filePath, FileReadFlags flags )
	: mHandle( nil ), mMappedData( nil )
{
	mxASSERT_PTR(_filePath);
	this->Open( _filePath, flags );
}

ERet FileReader::Open( const char* _filePath, FileReadFlags flags )
{
	mxASSERT_PTR( _filePath );

	mHandle = OS::IO::Create_File( _filePath, AccessMode::ReadAccess );

	if( mHandle == INVALID_HANDLE_VALUE )
	{
		if( flags & FileRead_NoOpenError ) {
			//ptWARN( "~ Failed to open file '%s' for reading\n", fileName );
		} else {
			ptWARN( "~ Failed to open file '%s' for reading\n", _filePath );
		}
		return ERR_FAILED_TO_OPEN_FILE;
	}

	if( this->IsOpen() && OS::IO::Get_File_Size(mHandle) == 0 )
	{
		if( !(flags & FileRead_NoReadError) )
		{
			ptERROR( "~ Failed to open file '%s' of zero size for reading\n", _filePath );
			this->Close();
			return ERR_FAILED_TO_OPEN_FILE;
		}
	}

	if( mHandle != INVALID_HANDLE_VALUE ) {
		DBGFILE(DBGOUT("~ Opened file '%s' (%u bytes) for reading\n",_filePath,(UINT)this->Length()));
		this->DbgSetName( _filePath );
		return ALL_OK;
	} else {
		DBGFILE(DBGOUT( "~ Couldn't open file '%s' for reading\n", _filePath ));
		return ERR_FAILED_TO_OPEN_FILE;
	}

	// pessimistic
	return ERR_UNKNOWN_ERROR;
}

FileReader::FileReader( FileHandleT hFile )
{
	mxASSERT( hFile != INVALID_HANDLE_VALUE );
	mHandle = hFile;
	mMappedData = nil;
}

FileReader::~FileReader()
{
	if( this->IsOpen() ) {
		this->Close();
	}
}

bool FileReader::IsOpen() const
{
	return mHandle != INVALID_HANDLE_VALUE;
}

ERet FileReader::Read( void* pBuffer, size_t size )
{
	mxASSERT_PTR(pBuffer);
	const size_t read = OS::IO::Read_File( mHandle, pBuffer, size );
//	mxASSERT(read > 0);
	return (read == size) ? ALL_OK : ERR_FAILED_TO_READ_FILE;
}

void FileReader::Close()
{
	if( this->IsMapped() ) {
		this->Unmap();
	}
	if( mHandle != INVALID_HANDLE_VALUE ) {
		OS::IO::Close_File( mHandle );
	}
	mHandle = INVALID_HANDLE_VALUE;
	//DBGOUT("Closed real-only file '%s'\n",mFileName);
}

const FileTimeT FileReader::GetTimeStamp() const
{
	mxASSERT(this->IsOpen());
	FileTimeT	result;
	CHK(OS::IO::GetFileTimeStamp(mHandle, result));
	return result;
}

void FileReader::Seek( size_t offset )
{
	mxASSERT(this->IsOpen());
	OS::IO::Seek_File( mHandle, offset, SeekOrigin::Begin );
}

void FileReader::Skip( size_t bytes )
{
	mxASSERT(this->IsOpen());
	size_t newPos = this->Tell() + bytes;
	this->Seek( newPos );
}

size_t FileReader::Length() const
{
	mxASSERT(this->IsOpen());

	const size_t fileSize = OS::IO::Get_File_Size( mHandle );
	mxASSERT( fileSize != INVALID_FILE_SIZE );

	return fileSize;
}

size_t FileReader::Tell() const
{
	mxASSERT(this->IsOpen());
	return OS::IO::Tell_File_Position( mHandle );
}

bool FileReader::AtEnd() const
{
	mxASSERT(this->IsOpen());
	return OS::IO::Is_End_of_File( mHandle ) != 0;
}

bool FileReader::CanBeMapped() const
{
	return true;
}

void * FileReader::Map()
{
	UNDONE;
	return nil;
	//mxASSERT(nil == mMappedData);
	//size_t size = this->Length();
	//mxASSERT( size > 0 );
	//void * data = g_heaps[HeapStreaming]->Allocate( size, 16 );
	//Seek( 0 );
	//size_t readSize = this->Read( data, size );
	//mxASSERT( readSize == size );
	//(void)readSize;
	//mMappedData = data;
	//return mMappedData;
}

void FileReader::Unmap()
{
	UNDONE;
	//mxASSERT(nil != mMappedData);
	//g_heaps[HeapStreaming]->Deallocate( mMappedData/*, this->Length()*/ );
	//mMappedData = nil;
}

bool FileReader::IsMapped() const
{
	return (mMappedData != nil);
}

/*
-------------------------------------------------------------------------
	FileWriter
-------------------------------------------------------------------------
*/
FileWriter::FileWriter()
{
	mHandle = INVALID_HANDLE_VALUE;
}

FileWriter::FileWriter( const char* fileName, FileWriteFlags flags )
{
	this->Open( fileName, flags );
}

FileWriter::FileWriter( FileHandleT hFile )
{
	mxASSERT( hFile != INVALID_HANDLE_VALUE );
	mHandle = hFile;
}

FileWriter::~FileWriter()
{
	if( this->IsOpen() ) {
		this->Close();
	}
}

ERet FileWriter::Open( const char* fileName, FileWriteFlags flags )
{
	mxASSERT_PTR( fileName );
	this->DbgSetName( fileName );

	AccessMode	accessMode = AccessMode::WriteAccess;

	if( flags & FileWrite_Append ) {
		accessMode = AccessMode::AppendAccess;
	}

	mHandle = OS::IO::Create_File( fileName, accessMode );

	if( mHandle != INVALID_HANDLE_VALUE ) {
		DBGFILE(DBGOUT("~ Opened file '%s' for writing\n",fileName));
	} else {
		DBGFILE(DBGOUT( "~ Couldn't open file '%s' for writing\n", fileName ));
	}

	if( mHandle == INVALID_HANDLE_VALUE )
	{
		if( flags & FileWrite_NoErrors ) {
			//ptWARN( "~ Failed to open file '%s' for writing\n", fileName );
		} else {
			ptERROR( "~ Failed to open file '%s' for writing\n", fileName );
		}
		return ERR_FAILED_TO_CREATE_FILE;
	}

	return ALL_OK;
}

bool FileWriter::IsOpen() const
{
	return (mHandle != INVALID_HANDLE_VALUE);
}

void FileWriter::Close()
{
	if( mHandle != INVALID_HANDLE_VALUE ) {
		OS::IO::Close_File( mHandle );
	}
	mHandle = INVALID_HANDLE_VALUE;
}

ERet FileWriter::Write( const void* pBuffer, size_t size )
{
	mxASSERT_PTR(pBuffer);
	mxASSERT(mHandle != INVALID_HANDLE_VALUE);
	const size_t written = OS::IO::Write_File( mHandle, pBuffer, size );
	return (written == size) ? ALL_OK : ERR_FAILED_TO_WRITE_FILE;
}

size_t FileWriter::Tell() const
{
	mxASSERT(this->IsOpen());
	return OS::IO::Tell_File_Position( mHandle );
}

void FileWriter::Seek( size_t offset )
{
	mxASSERT(this->IsOpen());
	OS::IO::Seek_File( mHandle, offset, SeekOrigin::Begin );
}

/*
-------------------------------------------------------------------------
	IOStreamFILE
-------------------------------------------------------------------------
*/
IOStreamFILE::IOStreamFILE()
{
	m_file = NULL;
}

IOStreamFILE::~IOStreamFILE()
{
	mxASSERT2(m_file == NULL, "Did you forget to close the file?");
}


ERet IOStreamFILE::openForReadingOnly(
	const char* filename
	)
{
	m_file = ::fopen(
		filename
		// In order to open a file as a binary file, a "b" character has to be included in the mode string.
		, "rb"
		);
	if( !m_file )
	{
		ptPRINT("Failed to open file '%s'\n", filename);
		return ERR_FAILED_TO_OPEN_FILE;
	}
	return ALL_OK;
}

ERet IOStreamFILE::openForReadingAndWriting(
	const char* filename
	, bool *created_new_file_ /*= nil*/
	)
{
	m_file = ::fopen(
		filename
		// "r+": read/update: Open a file for update (both for input and output). The file must exist.
		// In order to open a file as a binary file, a "b" character has to be included in the mode string.
		, "rb+"
		);

	bool	created_new_file = false;

	// If the file doesn't exist...
	if( !m_file )
	{
		// ...create it.
		m_file = ::fopen(
			filename
			// "w+": write/update: Create an empty file and open it for update (both for input and output). If a file with the same name already exists its contents are discarded and the file is treated as a new empty file.
			, "w+"
			);

		created_new_file = true;
	}

	if( !m_file )
	{
		ptPRINT("Failed to open file '%s'\n", filename);
		return ERR_FAILED_TO_OPEN_FILE;
	}

	if( created_new_file_ ) {
		*created_new_file_ = created_new_file;
	}

	return ALL_OK;
}

void IOStreamFILE::Close()
{
	if( m_file != NULL ) {
		::fflush( m_file );
		::fclose( m_file );
		m_file = NULL;
	}
}

bool IOStreamFILE::IsOpen() const
{
	return (m_file != NULL);
}

ERet IOStreamFILE::Seek( U32 offset ) const
{
	const int ret = ::fseek(m_file, offset, SEEK_SET);
	return (ret == 0) ? ALL_OK : ERR_FAILED_TO_SEEK_FILE;
}

ERet IOStreamFILE::seekToBeginning() const
{
	::fseek( m_file, 0, SEEK_SET );
	return ALL_OK;
}

ERet IOStreamFILE::SeekToEnd( size_t *file_length_ /*= nil*/ ) const
{
	const int ret = ::fseek( m_file, 0, SEEK_END );
	mxENSURE( ret == 0, ERR_FAILED_TO_SEEK_FILE, "" );
	if( file_length_ ) {
		*file_length_ = ::ftell(m_file);
	}
	return ALL_OK;
}

ERet IOStreamFILE::flushToOutput()
{
	// Returns zero on success. Otherwise EOF is returned and the error indicator of the file stream is set. 
	const int ret = ::fflush(m_file);
	return ret == 0 ? ALL_OK : ERR_FAILED_TO_WRITE_FILE;
}

size_t IOStreamFILE::Tell() const
{
	long ret = ::ftell(m_file);
	return ret != -1L ? ret : 0;
}

size_t IOStreamFILE::Length() const
{
	const size_t currPos = this->Tell();
	const size_t size = this->SeekToEnd();
	this->Seek(currPos);
	return size;
}

ERet IOStreamFILE::Read( void *buffer, size_t toRead )
{
	const size_t numRead = ::fread( buffer, 1, toRead, m_file );
	return (numRead != toRead) ? ERR_FAILED_TO_READ_FILE : ALL_OK;
}

ERet IOStreamFILE::Write( const void* buffer, size_t toWrite )
{
	const size_t numWritten = ::fwrite( buffer, 1, toWrite, m_file );
	return (numWritten != toWrite) ? ERR_FAILED_TO_WRITE_FILE : ALL_OK;
}

ERet Util_SaveDataToFile( const void* data, UINT dataSize, const char* fileName )
{
	chkRET_X_IF_NIL( fileName, ERR_NULL_POINTER_PASSED );
	chkRET_X_IF_NIL( data, ERR_NULL_POINTER_PASSED );
	chkRET_X_IF_NOT( dataSize > 0, ERR_INVALID_PARAMETER );
	FileWriter	file;
	mxDO(file.Open( fileName, FileWrite_NoErrors ));
	mxDO(file.Write( data, dataSize ));
	return ALL_OK;
}

#if 0//(mxPLATFORM == mxPLATFORM_WINDOWS)

//--------------------------------------------------------------------------

enum EOSFileType {
	FILETYPE_NONE,
	FILETYPE_FILE,
	FILETYPE_DIRECTORY
};
EOSFileType GetFileType( const char *filename )
{
	DWORD result = ::GetFileAttributesA(filename);

	if(result == INVALID_FILE_ATTRIBUTES)
		return FILETYPE_NONE;

	if(result & FILE_ATTRIBUTE_DIRECTORY)
		return FILETYPE_DIRECTORY;

	return FILETYPE_FILE;
}

//AReader & operator >> ( AReader& archive, FileTimeT& o )
//{
//	return archive.Unpack( o );
//}
//AWriter & operator << ( AWriter& archive, const FileTimeT& o )
//{
//	return archive.Pack( o );
//}

mxSTOLEN("http://winmerge.svn.sourceforge.net/svnroot/winmerge/trunk/Src/files.h");

#define LogErrorString(x,...)

/**
* @brief Open file as memory-mapped file.
* @param [in,out] fileData Memory-mapped file's info.
* @return TRUE if opening succeeded, FALSE otherwise.
*/
BOOL OpenFileMapped(MAPPEDFILEDATA *fileData)
{
	DWORD dwProtectFlag = 0;
	DWORD dwMapAccess = 0;
	DWORD dwOpenAccess = 0;
	DWORD dwFileSizeHigh = 0;
	DWORD dwSharedMode = FILE_SHARE_READ;
	HANDLE hTemplateFile = NULL; // for creating new file
	BOOL bSuccess = TRUE;

	if (fileData->bWritable)
	{
		dwProtectFlag = PAGE_READWRITE;
		dwMapAccess = FILE_MAP_ALL_ACCESS;
		dwOpenAccess = GENERIC_READ | GENERIC_WRITE;
	}
	else
	{
		dwProtectFlag = PAGE_READONLY;
		dwMapAccess = FILE_MAP_READ;
		dwOpenAccess = GENERIC_READ;
	}

	fileData->hFile = CreateFile(fileData->fileName,
		dwOpenAccess, dwSharedMode, NULL, fileData->dwOpenFlags,
		FILE_ATTRIBUTE_NORMAL, hTemplateFile);

	if (fileData->hFile == INVALID_HANDLE_VALUE)
	{
		bSuccess = FALSE;
		LogErrorString(Fmt(_T("CreateFile(%s) failed in OpenFileMapped: %s")
			, fileData->fileName, GetSysError(GetLastError()).c_str()));
	}
	else
	{
		if (fileData->dwSize == 0)
		{
			fileData->dwSize = GetFileSize(fileData->hFile,
				&dwFileSizeHigh);
			if (fileData->dwSize == 0xFFFFFFFF || dwFileSizeHigh)
			{
				fileData->dwSize = 0;
				bSuccess = FALSE;
			}
		}
	}

	if (bSuccess)
	{
		if (fileData->dwSize == 0 && dwFileSizeHigh == 0)
			// empty file (but should be accepted anyway)
			return bSuccess;

		fileData->hMapping = CreateFileMapping(fileData->hFile,
			NULL, dwProtectFlag, 0, fileData->dwSize, NULL);
		if (!fileData->hMapping)
		{
			bSuccess = FALSE;
			LogErrorString(Fmt(_T("CreateFileMapping(%s) failed: %s")
				, fileData->fileName, GetSysError(GetLastError()).c_str()));
		}
		else
		{
			fileData->pMapBase = MapViewOfFile(fileData->hMapping,
				dwMapAccess, 0, 0, 0);
			if (!fileData->pMapBase)
			{
				bSuccess = FALSE;
				LogErrorString(Fmt(_T("MapViewOfFile(%s) failed: %s")
					, fileData->fileName, GetSysError(GetLastError()).c_str()));
			}
		}
	}

	if (!bSuccess)
	{
		UnmapViewOfFile(fileData->pMapBase);
		fileData->pMapBase = NULL;
		CloseHandle(fileData->hMapping);
		fileData->hMapping = NULL;
		CloseHandle(fileData->hFile);
		fileData->hFile = NULL;
	}
	return bSuccess;
}

/**
* @brief Close memory-mapped file.
* @param [in, out] fileData Memory-mapped file's info.
* @param [in] newSize New size for the file.
* @param [in] flush Flush buffers before closing the file.
* @return TRUE if closing succeeded without errors, FALSE otherwise.
*/
BOOL CloseFileMapped(MAPPEDFILEDATA *fileData, DWORD newSize, BOOL flush)
{
	BOOL bSuccess = TRUE;

	if (fileData->pMapBase)
	{
		UnmapViewOfFile(fileData->pMapBase);
		fileData->pMapBase = NULL;
	}

	if (fileData->hMapping)
	{
		CloseHandle( fileData->hMapping );
		fileData->hMapping = NULL;
	}

	if (newSize != 0xFFFFFFFF)
	{
		SetFilePointer(fileData->hFile, newSize, NULL, FILE_BEGIN);
		SetEndOfFile(fileData->hFile);
	}

	if (flush)
		FlushFileBuffers(fileData->hFile);

	if (fileData->hFile)
	{
		CloseHandle(fileData->hFile);
		fileData->hFile = NULL;
	}
	return bSuccess;
}

///**
// * @brief Checks if file is read-only on disk.
// * Optionally returns also if file exists.
// * @param [in] file Full path to file to check.
// * @param [in, out] fileExists If non-NULL, function returns if file exists.
// * @return TRUE if file is read-only, FALSE otherwise.
// */
//BOOL IsFileReadOnly(const TCHAR* file, BOOL *fileExists /*=NULL*/)
//{
//	struct _stati64 fstats = {0};
//	BOOL bReadOnly = FALSE;
//	BOOL bExists = FALSE;
//
//	if (_tstati64(file, &fstats) == 0)
//	{
//		bExists = TRUE;
//
//		if ((fstats.st_mode & _S_IWRITE) == 0)
//			bReadOnly = TRUE;
//	}
//	else
//		bExists = FALSE;
//
//	if (fileExists != NULL)
//		*fileExists = bExists;
//
//	return bReadOnly;
//}

/**
* @brief Update file's modification time.
* @param [in] info contains filename, path and file times to update.
*/
//void UpdateFileTime(const DiffFileInfo & info)
//{
//        String path = paths_ConcatPath(info.path, info.filename);
//        _utimbuf times = {0};
//
//        times.modtime = info.mtime;
//        _tutime(path.c_str(), &times);
//}

#endif // (mxPLATFORM == mxPLATFORM_WINDOWS)

namespace OS
{
	namespace IO
	{

	FileHandleT Create_File(
		const char* path,
		AccessMode accessMode,
		AccessPattern accessPattern )
	{
		const char * nativePath = path;

		DWORD access = 0;
		DWORD disposition = 0;
		DWORD shareMode = 0;
		DWORD flagsAndAttributes = 0;

		switch (accessMode)
		{
		case AccessMode::ReadAccess:
			access = GENERIC_READ;            
			disposition = OPEN_EXISTING;
			shareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
			break;

		case AccessMode::WriteAccess:
			access = GENERIC_WRITE;
			disposition = CREATE_ALWAYS;
			shareMode = FILE_SHARE_READ;
			break;

		case AccessMode::ReadWriteAccess:
		case AccessMode::AppendAccess:
			access = GENERIC_READ | GENERIC_WRITE;
			disposition = OPEN_ALWAYS;
			shareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
			break;
		}

		switch (accessPattern)
		{
		case AccessPattern::Random:
			flagsAndAttributes |= FILE_FLAG_RANDOM_ACCESS;
			break;

		case AccessPattern::Sequential:
			flagsAndAttributes |= FILE_FLAG_SEQUENTIAL_SCAN;
			break;
		}

		// open/create the file
		FileHandleT handle = ::CreateFileA(
			nativePath,              // lpFileName
			access,                  // dwDesiredAccess
			shareMode,               // dwShareMode
			0,                       // lpSecurityAttributes
			disposition,             // dwCreationDisposition,
			flagsAndAttributes,      // dwFlagsAndAttributes
			NULL                     // hTemplateFile
			);
		if( handle != INVALID_HANDLE_VALUE )
		{
			// in append mode, we need to seek to the end of the file
			if (AccessMode::AppendAccess == accessMode)
			{
				::SetFilePointer( handle, 0, NULL, FILE_END );
			}
			return handle;
		}
		else
		{
			return INVALID_HANDLE_VALUE;
		}
	}

	//--------------------------------------------------------------------------

	void Close_File( FileHandleT handle )
	{
		mxASSERT(INVALID_HANDLE_VALUE != handle);
		::CloseHandle( handle );
	}

	//--------------------------------------------------------------------------

	size_t Write_File( FileHandleT handle, const void* buf, size_t size )
	{
		mxASSERT(INVALID_HANDLE_VALUE != handle);
		mxASSERT(buf != 0);
		mxASSERT(size > 0);
		DWORD bytesWritten = 0;
		BOOL result = ::WriteFile(handle, buf, size, &bytesWritten, NULL);
		if ((0 == result) || ((DWORD)size != bytesWritten))
		{
			ptERROR("WriteFile() failed!");
		}
		return bytesWritten;
	}

	//--------------------------------------------------------------------------
	/**
	Read data from a file, returns number of bytes read.
	*/

	size_t Read_File( FileHandleT handle, void* buf, size_t size )
	{
		mxASSERT(INVALID_HANDLE_VALUE != handle);
		mxASSERT(buf != 0);
		mxASSERT(size > 0);
		DWORD bytesRead = 0;
		BOOL result = ::ReadFile( handle, buf, size, &bytesRead, NULL );
		//	mxASSERT(bytesRead > 0);
		if (0 == result || (bytesRead != (DWORD)size))
		{
			// If the return value is nonzero and the number of bytes read is zero,
			// the file pointer was beyond the current end of the file at the time of the read operation.
			ptERROR("ReadFile() failed! %u bytes to read, %u bytes read (offset=%u)",
				size, bytesRead, OS::IO::Tell_File_Position( handle )
				);
		}

		return bytesRead;
	}

	//--------------------------------------------------------------------------
	/**
	Seek in a file.
	*/

	bool Seek_File( FileHandleT handle, size_t offset, SeekOrigin orig )
	{
		mxASSERT(INVALID_HANDLE_VALUE != handle);
		DWORD moveMethod;
		switch (orig)
		{
		case SeekOrigin::Begin:
			moveMethod = FILE_BEGIN;
			break;
		case SeekOrigin::Current:
			moveMethod = FILE_CURRENT;
			break;
		case SeekOrigin::End:
			moveMethod = FILE_END;
			break;
		default:
			// can't happen
			mxUNREACHABLE;
			moveMethod = FILE_BEGIN;
			break;
		}
		LARGE_INTEGER	distanceToMove;
		distanceToMove.LowPart = offset;
		distanceToMove.HighPart = 0;
		return ::SetFilePointerEx( handle, distanceToMove, NULL, moveMethod ) == TRUE;
	}

	//--------------------------------------------------------------------------
	/**
	Get current position in file.
	*/

	size_t Tell_File_Position( FileHandleT handle )
	{
		mxASSERT(INVALID_HANDLE_VALUE != handle);
		return ::SetFilePointer( handle, 0, NULL, FILE_CURRENT );
	}

	void Flush_File( FileHandleT handle )
	{
		mxASSERT(INVALID_HANDLE_VALUE != handle);
		::FlushFileBuffers( handle );
	}

	//--------------------------------------------------------------------------
	/**
	Returns true if current position is at end of file.
	*/

	bool Is_End_of_File( FileHandleT handle )
	{
		mxASSERT(INVALID_HANDLE_VALUE != handle);
		DWORD fpos = ::SetFilePointer( handle, 0, NULL, FILE_CURRENT );
		DWORD size = ::GetFileSize( handle, NULL );
		// NOTE: THE '>=' IS NOT A BUG!!!
		return fpos >= size;
	}

	//--------------------------------------------------------------------------
	/**
	Returns the size of a file in bytes.
	*/

	size_t Get_File_Size( FileHandleT handle )
	{
		mxASSERT(INVALID_HANDLE_VALUE != handle);
		return ::GetFileSize( handle, NULL );
	}

	/**
	Return true if a file exists.
	*/

	bool
		FileExists( const char* path )
	{
#if 1
		const char * nativePath = path;

		const DWORD fileAttrs = ::GetFileAttributesA( nativePath );
		if( (-1 != fileAttrs)
			&& (0 == (FILE_ATTRIBUTE_DIRECTORY & fileAttrs)) )
		{
			return true;
		}
		else
		{
			return false;
		}
#else
		// simply check for file existence
		return ( ::_access( path, 0 ) == 0 );
#endif
	}

	bool PathExists( const char* path )
	{
#if 0
		// GetFileAttributes() works with both files and directories.
		return ( ::GetFileAttributesA( path ) != INVALID_FILE_ATTRIBUTES );
#elif 0
		// for some reason, GetFileAttributesA() didn't worked,
		// this works ok:
		struct stat st;
		return stat( path, &st ) == 0;
#else
		return _access(path, 0) != -1;
#endif
	}

	bool FileOrPathExists( const char* path )
	{
		// GetFileAttributes() works with both files and directories.
		return ( ::GetFileAttributesA( path ) != INVALID_FILE_ATTRIBUTES );
	}

	bool Is_File(const char* path)
	{
		DWORD fattr = GetFileAttributesA(path);
		return (fattr != INVALID_FILE_ATTRIBUTES && (fattr & FILE_ATTRIBUTE_DIRECTORY) == 0);
	}
	bool Is_Directory(const char* path)
	{
		DWORD fattr = GetFileAttributesA(path);
		return (fattr != INVALID_FILE_ATTRIBUTES && (fattr & FILE_ATTRIBUTE_DIRECTORY) != 0);
	}

	bool GetFileTimeStamp( FileHandleT handle, FileTimeT &outTimeStamp )
	{
		FILETIME	ftWrite;
		RET_FALSE_IF_NOT( ::GetFileTime( handle, NULL, NULL, &ftWrite ) );

		outTimeStamp = ftWrite;

		return true;
	}

	bool GetFileCreationTime( FileHandleT handle, FileTimeT &outCreationTime )
	{
		FILETIME	ftCreation;
		RET_FALSE_IF_NOT( ::GetFileTime( handle, &ftCreation, NULL, NULL ) );

		outCreationTime = ftCreation;

		return true;
	}

	BOOL GetFileDateTime( FileHandleT handle, PtDateTime &outDateTime )
	{
		FILETIME	ftCreate, ftAccess, ftWrite,
			ftLocal;

		RET_FALSE_IF_NOT( ::GetFileTime( handle, &ftCreate, &ftAccess, &ftWrite ) );
		RET_FALSE_IF_NOT( ::FileTimeToLocalFileTime( &ftWrite, &ftLocal ) );

		SYSTEMTIME fileSysTime;
		RET_FALSE_IF_NOT( ::FileTimeToSystemTime( &ftLocal, &fileSysTime ) );

		outDateTime.year = fileSysTime.wYear - 1;
		outDateTime.month = fileSysTime.wMonth - 1;
		outDateTime.day = fileSysTime.wDay - 1;
		outDateTime.hour = fileSysTime.wHour;
		outDateTime.minute = fileSysTime.wMinute;
		outDateTime.second = fileSysTime.wSecond;

		return TRUE;
	}

	mxSTOLEN("MSDN");
	BOOL GetLastWriteTimeString( FileHandleT hFile, LPTSTR lpszString, DWORD dwSize )
	{
		FILETIME ftCreate, ftAccess, ftWrite;
		SYSTEMTIME stUTC, stLocal;
		DWORD dwRet;

		// Retrieve the file times for the file.
		if (!GetFileTime(hFile, &ftCreate, &ftAccess, &ftWrite))
		{
			ptBREAK;
			return FALSE;
		}

		// Convert the last-write time to local time.
		FileTimeToSystemTime(&ftWrite, &stUTC);
		SystemTimeToTzSpecificLocalTime(NULL, &stUTC, &stLocal);

		// Build a string showing the date and time.
		dwRet = ::StringCchPrintf(
			lpszString, dwSize, 
			TEXT("%02d/%02d/%d  %02d:%02d"),
			stLocal.wMonth, stLocal.wDay, stLocal.wYear,
			stLocal.wHour, stLocal.wMinute
			);

		return ( S_OK == dwRet ) ? TRUE : FALSE;
	}

	BOOL Win32_FileTimeStampToDateTimeString( const FILETIME& fileTime, char *outString, UINT numChars )
	{
		DWORD dwRet;

		FILETIME	ftLocal;
		RET_FALSE_IF_NOT( ::FileTimeToLocalFileTime( &fileTime, &ftLocal ) );

		SYSTEMTIME fileSysTime;
		RET_FALSE_IF_NOT( ::FileTimeToSystemTime( &ftLocal, &fileSysTime ) );

		// Build a string showing the date and time.
		dwRet = ::StringCchPrintfA(
			outString, numChars, 
			("%02d.%02d.%d  %02d:%02d"),
			fileSysTime.wMonth, fileSysTime.wDay, fileSysTime.wYear,
			fileSysTime.wHour, fileSysTime.wMinute
			);
		if( S_OK == dwRet )
			return TRUE;
		else return FALSE;
	}

	UINT64 GetDriveFreeSpace( const char* path )
	{
		LARGE_INTEGER freeBytesAvailableToCaller;// bytes on disk available to caller
		LARGE_INTEGER totalNumberOfBytes;	// bytes on disk
		LARGE_INTEGER totalNumberOfFreeBytes;	// free bytes on disk

		::GetDiskFreeSpaceExA(
			path,
			(PULARGE_INTEGER )&freeBytesAvailableToCaller,
			(PULARGE_INTEGER )&totalNumberOfBytes,
			(PULARGE_INTEGER )&totalNumberOfFreeBytes
			);

		//return *(size_t*)&totalNumberOfFreeBytes;
		return always_cast< UINT64, LARGE_INTEGER >( totalNumberOfFreeBytes );
	}

	bool AisNewerThanB( const FileTimeT& a, const FileTimeT& b )
	{
		return a > b;
	}

	bool IsFileNewer( const char* fileNameA, const FileTimeT& lastTimeWhenModified )
	{
		HANDLE hFile;

		hFile = ::CreateFileA( fileNameA, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
		if( INVALID_HANDLE_VALUE == hFile ) {
			ptBREAK;
			return true;
		}
#if 0
		BY_HANDLE_FILE_INFORMATION fi;
		::GetFileInformationByHandle( hFile, &fi );
		::CloseHandle( hFile );

		LONG result = ::CompareFileTime( &fi.ftLastWriteTime, &lastTimeWhenModified );
#else
		FILETIME	ftWrite;
		::GetFileTime( hFile, NULL, NULL, &ftWrite );
		::CloseHandle( hFile );
		/*
		values returned by CompareFileTime() and their meaning:
		-1 - First file time is earlier than second file time.
		0 - First file time is equal to second file time.
		1 - First file time is later than second file time.
		*/
		LONG result = ::CompareFileTime( &ftWrite, &lastTimeWhenModified.time );
#endif
		return (result == 1);
	}

	bool IsFileANewerThanFileB( const char* fileNameA, const char* fileNameB )
	{
		HANDLE hFile;
		BY_HANDLE_FILE_INFORMATION fi1, fi2;

		hFile = ::CreateFileA( fileNameA, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
		if( INVALID_HANDLE_VALUE == hFile ) {
			ptBREAK;
			return true;
		}
		::GetFileInformationByHandle( hFile, &fi1 );
		::CloseHandle( hFile );

		hFile = ::CreateFileA( fileNameB, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
		if( INVALID_HANDLE_VALUE == hFile ) {
			ptBREAK;
			return true;
		}
		::GetFileInformationByHandle( hFile, &fi2 );
		::CloseHandle( hFile );

		LONG result = ::CompareFileTime( &fi1.ftLastWriteTime, &fi2.ftLastWriteTime );
		return (result == 1);
	}

	bool Delete_File( const char* path )
	{
		DBGOUT("Deleting file '%s'\n",path);
		return ::DeleteFileA( path ) == TRUE;
	}

	bool MakeDirectory( const char* path )
	{
		DBGOUT("Creating directory '%s'\n",path);
		return ::CreateDirectoryA( path, NULL ) == TRUE;
		//	if (::GetLastError() != ERROR_ALREADY_EXISTS) return false;
	}
	bool DeleteDirectory( const char* path )
	{
		return ::RemoveDirectoryA(path) != 0;
	}

	bool MakeDirectoryIfDoesntExist( const char* path )
	{
		if( PathExists( path ) ) {
			return true;
		}
		return MakeDirectory( path );
	}

	ERet MakeDirectoryIfDoesntExist2( const char* path )
	{
		mxENSURE(
			OS::IO::MakeDirectoryIfDoesntExist( path ),
			ERR_FAILED_TO_CREATE_FILE,
			"Couldn't create directory '%s'", path
			);
		return ALL_OK;
	}

	bool SetCurrentDirectory( const char* dir )
	{
		return TRUE == ::SetCurrentDirectoryA( dir );
	}

	U32 GetCurrentFolder( char *buffer, U32 numChars )
	{
		mxASSERT_PTR(buffer);
		mxASSERT(numChars > 0);
		// If the function succeeds, the return value specifies the number of characters that are written to the buffer, not including the terminating null character.
		// If the function fails, the return value is zero.
		return (U32)::GetCurrentDirectoryA( numChars, buffer );
	}

	// fills the buffer with strings that specify valid drives in the system
	// e.g. buffer = "CDEF" and numDrives = 4
	bool GetDriveLetters( char *buffer, int bufferSize, UINT &numDrives )
	{
		mxASSERT(bufferSize > 0);
		numDrives = 0;

		char	tmp[256];
		MemZero(tmp, sizeof (tmp));

		// GetLogicalDriveStringsA() fills the buffer with strings
		// that specify valid drives in the system
		// e.g. "C:\" "D:\" "E:\"
		if( GetLogicalDriveStringsA( sizeof(tmp), tmp ) )
		{
			char* p = tmp;
			while( *p && (p - tmp < bufferSize) )
			{
				buffer[ numDrives++ ] = *p;
				p += 4; // sizeof("C:\\ ") is 4
			}
			return true;
		}
		return false;
	}

#if 0
	void list_files(const char* path, Vector<DynamicString>& files)
	{
		HANDLE file = INVALID_HANDLE_VALUE;
		WIN32_FIND_DATA ffd;
		char cur_path[1024];
		strncpy(cur_path, path, strlen(path) + 1);
		strncat(cur_path, "\\*", 2);
		file = FindFirstFile(cur_path, &ffd);
		do
		{
			CE_ASSERT(file != INVALID_HANDLE_VALUE, "Unable to list files. errono %d", GetLastError());
			if ((strcmp(ffd.cFileName, ".") == 0) || (strcmp(ffd.cFileName, "..") == 0))
			{
				continue;
			}
			DynamicString filename(default_allocator());
			filename = ffd.cFileName;
			vector::push_back(files, filename);
		}
		while (FindNextFile(file, &ffd) != 0);
		FindClose(file);
	}
#endif

	}//namespace IO

	namespace Path
	{
		bool IsAbsolute( const char* path )
		{
			mxASSERT_PTR(path);
		#if mxPLATFORM == mxPLATFORM_WINDOWS
			return strlen(path) > 2 && isalpha(path[0]) && path[1] == ':' && path[2] == '\\';
		#else
			return strlen(path) > 0 && path[0] == '/';
		#endif
		}
		bool IsRootPath( const char* path )
		{
		#if mxPLATFORM == mxPLATFORM_WINDOWS
			return IsAbsolute(path) && strlen(path) == 3;
		#else
			return IsAbsolute(path) && strlen(path) == 1;
		#endif
		}
	}//namespace Path
}//namespace OS

//----------------------------------------------------------//
//				End Of File.									//
//----------------------------------------------------------//
