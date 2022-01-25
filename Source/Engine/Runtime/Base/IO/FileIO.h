/*
=============================================================================
	File:	Files.h
	Desc:	
=============================================================================
*/
#pragma once

#if mxPLATFORM == mxPLATFORM_WINDOWS
	typedef HANDLE	FileHandleT;

	struct FileTimeT
	{
		FILETIME time;
	public:
		inline FileTimeT()
		{
			time.dwLowDateTime = 0;
			time.dwHighDateTime = 0;
		}
		inline FileTimeT( DWORD lo, DWORD hi )
		{
			time.dwLowDateTime = lo;
			time.dwHighDateTime = hi;
		}
		inline FileTimeT( const FILETIME& ft )
		{
			time = ft;
		}
		/*
			values returned by CompareFileTime() and their meaning:
			-1 - First file time is earlier than second file time.
			0 - First file time is equal to second file time.
			1 - First file time is later than second file time.
		*/
		inline friend bool operator == ( const FileTimeT& a, const FileTimeT& b )
		{
			return (0 == ::CompareFileTime( &(a.time), &(b.time)) );
		}
		inline friend bool operator != ( const FileTimeT& a, const FileTimeT& b )
		{
			return (0 != ::CompareFileTime( &(a.time), &(b.time)) );
		}
		inline friend bool operator > ( const FileTimeT& a, const FileTimeT& b )
		{
			return (1 == ::CompareFileTime( &(a.time), &(b.time)) );
		}
		inline friend bool operator < ( const FileTimeT& a, const FileTimeT& b )
		{
			return (-1 == ::CompareFileTime( &(a.time), &(b.time)) );
		}

		inline void operator = ( const FILETIME& platformTimeStamp )
		{
			this->time = platformTimeStamp;
		}
	public:
		static FileTimeT CurrentTime();
	};
	mxDECLARE_POD_TYPE(FileTimeT);

#else
#	error "Unknown platform!"
#endif

enum EFileReadFlags
{
	FileRead_NoOpenError	= BIT(0),	// don't cause an error if the file could not be opened
	FileRead_NoReadError	= BIT(1),	// don't cause an error if the file has zero size

	FileRead_NoErrors		= FileRead_NoOpenError|FileRead_NoReadError,

	FileRead_DefaultFlags	= 0
};
typedef TBits< EFileReadFlags, U32 >	FileReadFlags;

enum EFileWriteFlags
{
	FileWrite_NoErrors	= BIT(0),	// erases existing contents
	FileWrite_Append	= BIT(1),	// appends new data to existing file

	FileWrite_DefaultFlags	= 0
};
typedef TBits< EFileWriteFlags, U32 >	FileWriteFlags;

/*
-------------------------------------------------------------------------
	FileReader
-------------------------------------------------------------------------
*/
class FileReader: public AReader
{
public:
				FileReader();
	explicit	FileReader( const char* _filePath, FileReadFlags flags = FileRead_DefaultFlags );
				~FileReader();

	ERet		Open( const char* _filePath, FileReadFlags flags = FileRead_DefaultFlags );

	bool		IsOpen() const;

	virtual ERet	Read( void* pBuffer, size_t size ) override;

	// Set the file pointer to the given offset (in bytes) from the beginning.
	void		Seek( size_t offset );

	void		Skip( size_t size );

	// Return the size of data (length of the file if this is a file), in bytes.
	virtual size_t	Length() const override;

	virtual size_t	Tell() const override;

	// Returns true if the end has been reached.
	bool		AtEnd() const;

	// Returns true if the stream provides direct memory access.
	bool		CanBeMapped() const;

	// Map stream to memory.
	void *		Map();
	void		Unmap();
	bool		IsMapped() const;

	void		Close();

	const FileTimeT GetTimeStamp() const;

public_internal:
	// initializes from the supplied file handle
	explicit FileReader( FileHandleT hFile );

	FileHandleT GetFileHandle() const
	{
		return mHandle;
	}

private:
	FileHandleT mHandle;
	void *	mMappedData;
};

/*
-------------------------------------------------------------------------
	FileWriter
-------------------------------------------------------------------------
*/
class FileWriter: public AWriter
{
public:
	FileWriter();
	explicit FileWriter( const char* fileName, FileWriteFlags flags = FileWrite_DefaultFlags );
	~FileWriter();

	ERet	Open( const char* fileName, FileWriteFlags flags = FileWrite_DefaultFlags );

	bool	IsOpen() const;
	void	Close();

	virtual ERet Write( const void* pBuffer, size_t size ) override;


	size_t	Tell() const;

	// Set the file pointer to the given offset (in bytes) from the beginning.
	//
	void	Seek( size_t offset );

public_internal:
	explicit FileWriter( FileHandleT hFile );

	FileHandleT GetFileHandle() const
	{
		return mHandle;
	}

private:
	FileHandleT mHandle;
};

/*
-------------------------------------------------------------------------
	IOStreamFILE
-------------------------------------------------------------------------
*/
class IOStreamFILE: public AReader, public AWriter
{
	FILE *	m_file;

public:
	IOStreamFILE();
	~IOStreamFILE();

	ERet openForReadingOnly(
		const char* filename
		);

	ERet openForReadingAndWriting(
		const char* filename
		, bool *created_new_file_ = nil
		);

	void Close();

	bool IsOpen() const;

	ERet Seek( U32 offset ) const;
	ERet seekToBeginning() const;
	ERet SeekToEnd( size_t *file_length_ = nil ) const;

	ERet flushToOutput();

	// AReader
	virtual size_t	Tell() const override;
	///NOTE: very slow! seeks to end to find file size!
	virtual size_t Length() const override;
	virtual ERet Read( void *buffer, size_t toRead ) override;

	// AWriter
	virtual ERet Write( const void* buffer, size_t toWrite ) override;
};

ERet Util_SaveDataToFile( const void* data, UINT dataSize, const char* fileName );

/*
=============================================================================
	Low-level file operations
=============================================================================
*/


#if 0 // mxPLATFORM == mxPLATFORM_WINDOWS

mxSTOLEN("http://winmerge.svn.sourceforge.net/svnroot/winmerge/trunk/Src/files.h");
/**
 * @brief Memory-mapped file information
 * When memory-mapped file is created, related information is
 * stored to this structure.
 */
struct MAPPEDFILEDATA
{
	TCHAR fileName[MAX_PATH];
	BOOL bWritable;
	DWORD dwOpenFlags;              // CreateFile()'s dwCreationDisposition
	DWORD dwSize;
	HANDLE hFile;
	HANDLE hMapping;
	LPVOID pMapBase;
};

BOOL OpenFileMapped(MAPPEDFILEDATA *fileData);
BOOL CloseFileMapped(MAPPEDFILEDATA *fileData, DWORD newSize, BOOL flush);
BOOL IsFileReadOnly(const TCHAR* file, BOOL *fileExists = NULL);

#endif // (mxPLATFORM == mxPLATFORM_WINDOWS)

// NOTE: underscores to avoid conflicts with Windows
namespace OS
{
	namespace IO
	{
		enum AccessMode
		{
			ReadAccess,		// opens a file only if it exists
			WriteAccess,	// overwrites existing file (always creates a new file)
			AppendAccess,	// appends to existing file
			ReadWriteAccess,// always creates a new file ('re-write')
		};
		enum AccessPattern
		{
			Random,
			Sequential,
		};
		enum SeekOrigin
		{
			Begin,
			Current,
			End,
		};

		// returns INVALID_HANDLE_VALUE on failure
		FileHandleT Create_File(
			const char* path,
			AccessMode accessMode,
			AccessPattern accessPattern = AccessPattern::Sequential
		);

		void Close_File( FileHandleT handle );

		// Writes data to the specified file.
		size_t Write_File( FileHandleT handle, const void* buf, size_t size );

		// Reads data from the specified file.
		size_t Read_File( FileHandleT handle, void* buf, size_t size );

		// Moves the file pointer of the specified file.
		bool Seek_File( FileHandleT handle, size_t offset, SeekOrigin orig );

		size_t Tell_File_Position( FileHandleT handle );

		// Flush unwritten data to disk.
		void Flush_File( FileHandleT handle );

		bool Is_End_of_File( FileHandleT handle );

		// Retrieves the size of the specified file, in bytes.
		size_t Get_File_Size( FileHandleT handle );

		bool FileExists( const char* file );
		bool PathExists( const char* path );//DirectoryExists
		bool FileOrPathExists( const char* path );

		bool Is_File(const char* path);
		bool Is_Directory(const char* path);

		// retrieves the time stamp of last modification
		bool GetFileTimeStamp( FileHandleT handle, FileTimeT &outTimeStamp );

		bool GetFileCreationTime( FileHandleT handle, FileTimeT &outCreationTime );

		// returns 'false' in case of failure
		BOOL GetFileDateTime( FileHandleT handle, PtDateTime &outDateTime );

		// GetLastWriteTime - Retrieves the last-write time and converts
		//                    the time to a string
		//
		// Return value - TRUE if successful, FALSE otherwise
		// hFile      - Valid file handle
		// lpszString - Pointer to buffer to receive string
		// dwSize     - Number of elements in the buffer
		BOOL GetLastWriteTimeString( FileHandleT hFile, LPTSTR lpszString, DWORD dwSize );

		BOOL Win32_FileTimeStampToDateTimeString( const FILETIME& fileTime, char *outString, UINT numChars );

		// Returns the amount of free drive space in the specified path, in bytes.
		UINT64 GetDriveFreeSpace( const char* path );

		// useful for reloading modified files
		bool AisNewerThanB( const FileTimeT& a, const FileTimeT& b );
		bool IsFileNewer( const char* fileNameA, const FileTimeT& lastTimeWhenModified );
		bool IsFileANewerThanFileB( const char* fileNameA, const char* fileNameB );

		bool Delete_File( const char* path );
		bool MakeDirectory( const char* path );
		bool DeleteDirectory( const char* path );

		bool MakeDirectoryIfDoesntExist( const char* path );
		ERet MakeDirectoryIfDoesntExist2( const char* path );

		// Changes the current directory for the current process.
		bool SetCurrentDirectory( const char* dir );

		// 'dir' - a pointer to the buffer that receives the current directory string. This null-terminated string specifies the absolute path to the current directory.
		// 'numChars' - size of the output buffer in characters. The buffer length must include room for a terminating null character.
		// If the function succeeds, the return value specifies the number of characters that are written to the buffer, not including the terminating null character.
		// If the function fails, the return value is zero.
		U32 GetCurrentFolder( char *buffer, U32 numChars );

		// fills the buffer with strings that specify valid drives in the system
		// e.g. buffer = "CDEF" and numDrives = 4
		bool GetDriveLetters( char *buffer, int bufferSize, UINT &numDrives );

	}//namespace IO

	namespace Path
	{
		bool IsAbsolute( const char* path );
		bool IsRootPath( const char* path );
	}//namespace Path
}//namespace OS
