#pragma once

#include <Base/Text/StringTools.h>
#include <Base/Text/String.h>

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
template< UINT NUM_CHARS >
inline
bool F_ExtractFileBase(
					   const char* inFilePath,
					   ANSICHAR (&outFileBase)[NUM_CHARS]
)
{
	chkRET_FALSE_IF_NIL(inFilePath);

	V_FileBase( inFilePath, outFileBase, mxCOUNT_OF(outFileBase) );

	return true;
}
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
template< UINT NUM_CHARS >
bool F_ComposeFilePath(
					   const char* inOldFilePath,
					   const char* inOldFileName,
					   const char* inNewExtension,
					   ANSICHAR (&outNewFilePath)[NUM_CHARS]
)
{
	chkRET_FALSE_IF_NIL(inOldFilePath);
	chkRET_FALSE_IF_NIL(inOldFileName);
	chkRET_FALSE_IF_NIL(inNewExtension);

	// Extract the file base.
	ANSICHAR	fileBase[ MAX_PATH ];
	V_FileBase( inOldFileName, fileBase, mxCOUNT_OF(fileBase) );

	// Extract the file path only.
	ANSICHAR	purePath[ MAX_PATH ];
	if( !V_ExtractFilePath( inOldFilePath, purePath, mxCOUNT_OF(purePath) ) )
	{
		mxStrCpyAnsi( purePath, inOldFilePath );
	}

	// Compose a new file path with the given extension.
	V_ComposeFileName( inOldFilePath, fileBase, outNewFilePath, NUM_CHARS );
	V_SetExtension( outNewFilePath, inNewExtension, NUM_CHARS );

	return true;
}
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
template< UINT NUM_CHARS >
bool F_ComposeFilePath(
					   const char* inOldFilePath,
					   const char* inNewExtension,
					   ANSICHAR (&outNewFilePath)[NUM_CHARS]
)
{
	chkRET_FALSE_IF_NIL(inOldFilePath);
	chkRET_FALSE_IF_NIL(inNewExtension);

	// Extract the file base.
	ANSICHAR	fileBase[ MAX_PATH ];
	V_FileBase( inOldFilePath, fileBase, mxCOUNT_OF(fileBase) );

	// Extract the file path only.
	ANSICHAR	purePath[ MAX_PATH ];
	if( !V_ExtractFilePath( inOldFilePath, purePath, mxCOUNT_OF(purePath) ) )
	{
		mxStrCpyAnsi( purePath, inOldFilePath );
	}

	// Compose a new file path with the given extension.
	V_ComposeFileName( purePath, fileBase, outNewFilePath, NUM_CHARS );
	V_SetExtension( outNewFilePath, inNewExtension, NUM_CHARS );

	return true;
}
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
template< UINT NUM_CHARS >
bool F_ExtractPathOnly(
					   const char* inFilePath,
					   ANSICHAR (&outPurePath)[NUM_CHARS]
)
{
	return V_ExtractFilePath( inFilePath, outPurePath, mxCOUNT_OF(outPurePath) );
}


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

/*
 *
 * DirectoryWalkerWin32.h
 * 
 * 
*/

//template< typename ARG_TYPE >
//struct DirWalkerBase
//{
//	// if returns true, then the given directory will be visited
//	virtual bool edEnterFolder( const char* folder, ARG_TYPE args )
//	{
//		DBGOUT("Entering folder: '%s'\n", folder);
//		return true;
//	}
//	virtual void edLeaveFolder( const char* folder, ARG_TYPE args )
//	{
//		DBGOUT("Leaving folder: '%s'\n", folder);
//	}
//	// accepts file name without path
//	virtual void edProcessFile( const char* fileName, ARG_TYPE args )
//	{
//		DBGOUT("Processing file: '%s'\n", fileName);
//	}
//};

struct SDirectoryInfo
{
	const char* fullPathName;	// fully-qualified path name
	const char* relativePath;	// name relative to the root folder, "" at the beginning
};
struct SFindFileInfo
{
//	String256	fileBaseOnly;
	String256	fileNameOnly;	// name of file without path (including extension)
	String256	fullPathName;	// full name of directory excluding file name
	String256	fullFileName;	// fully qualified file name (including path)
	String256	relativePath;	// name of folder relative to the root directory (including file name)
};

struct ADirectoryWalker abstract
{
	// return false to skip this folder
	virtual bool Enter_Folder( const SDirectoryInfo& dir )
	{ return true; }

	// return true to exit immediately
	virtual bool Found_File( const SFindFileInfo& file )
	{ return false; }

	virtual void Leave_Folder( const SDirectoryInfo& dir )
	{}

protected:
	virtual ~ADirectoryWalker() = 0;
};

void Win32_ProcessFilesInDirectory( const char* rootPath, ADirectoryWalker* callback, bool bRecurseSubfolders = true );

bool Win32_FindFileInDirectory( const char* _rootPath, const char* _fileName, SFindFileInfo &_fileInfo );

ERet Win32_GetAbsoluteFilePath( const char* _rootPath, const char* _fileName, String &_filePath );
