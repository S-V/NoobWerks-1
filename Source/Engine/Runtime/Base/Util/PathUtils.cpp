/*
=============================================================================
	File:	Misc.cpp
	Desc:	
=============================================================================
*/
#include <Base/Base_PCH.h>
#pragma hdrstop
#include <Base/Base.h>

#include <Base/Text/StringTools.h>
#include "PathUtils.h"

void Win32_ProcessFilesInDirectory_Internal( const SDirectoryInfo& dir, ADirectoryWalker* callback, bool bRecurseSubfolders )
{
	if( !callback->Enter_Folder( dir ) ) {
		return;
	}

	ANSICHAR	tmp[ MAX_PATH ];
	tMy_sprintfA( tmp, "%s*.*", dir.fullPathName );

	WIN32_FIND_DATAA	findFileData;
	const HANDLE fh = ::FindFirstFileA( tmp, &findFileData );

	if( fh == INVALID_HANDLE_VALUE ) {
		// the folder is empty or it doesn't exist
		return;
	}

	do
	{
		// skip self
		if(!strcmp( findFileData.cFileName, (".") ))
		{
			continue;
		}
		if(!strcmp( findFileData.cFileName, ("..") ))
		{
			continue;
		}

		// skip hidden file
		if( findFileData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN )
		{
			continue;
		}

		// if this is directory
		if( findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
		{
			if( bRecurseSubfolders )
			{
				ANSICHAR	relativePath[ MAX_PATH ];
				strcpy(relativePath, dir.relativePath);
				strcat(relativePath, findFileData.cFileName);
				V_AppendSlash(relativePath, mxCOUNT_OF(relativePath));

				ANSICHAR	fullChildPath[ MAX_PATH ];
				strcpy(fullChildPath, dir.fullPathName);
				strcat(fullChildPath, findFileData.cFileName);
				V_AppendSlash(fullChildPath, mxCOUNT_OF(fullChildPath));

				SDirectoryInfo	subfolder;
				subfolder.fullPathName = fullChildPath;
				subfolder.relativePath = relativePath;
				Win32_ProcessFilesInDirectory_Internal( subfolder, callback, bRecurseSubfolders );
			}
		}
		else
		{
			SFindFileInfo	fileInfo;

			Str::CopyS(fileInfo.fileNameOnly, findFileData.cFileName);
			Str::CopyS(fileInfo.fullPathName, dir.fullPathName);
			Str::CopyS(fileInfo.relativePath, dir.relativePath);

			ANSICHAR	fullFileName[ MAX_PATH ];
			strcpy(fullFileName, fileInfo.fullPathName.c_str());
			strcat(fullFileName, fileInfo.fileNameOnly.c_str());
			Str::CopyS(fileInfo.fullFileName, fullFileName);

			if( callback->Found_File( fileInfo ) ) {
				return;
			}
		}
	}
	while( ::FindNextFileA( fh , &findFileData ) != 0 );

	callback->Leave_Folder( dir );
}

void Win32_ProcessFilesInDirectory( const char* rootPath, ADirectoryWalker* callback, bool bRecurseSubfolders )
{
	mxASSERT_PTR(rootPath);

	SDirectoryInfo	dirInfo;
	dirInfo.fullPathName = rootPath;
	dirInfo.relativePath = "";

	Win32_ProcessFilesInDirectory_Internal( dirInfo, callback, bRecurseSubfolders );
}

ADirectoryWalker::~ADirectoryWalker()
{
}

bool Win32_FindFileInDirectory( const char* rootPath, const char* fileName, SFindFileInfo &fileInfo )
{
	fileInfo.fileNameOnly.empty();
	fileInfo.fullPathName.empty();
	fileInfo.fullFileName.empty();
	fileInfo.relativePath.empty();

	struct FileSearcher : ADirectoryWalker {
		const char* fileName;
		SFindFileInfo &fileInfo;
	public:
		FileSearcher( const char* _fileName, SFindFileInfo &_fileInfo )
			: fileName(_fileName), fileInfo(_fileInfo)
		{}
		virtual bool Found_File( const SFindFileInfo& found ) override
		{
			if( stricmp( fileName, found.fileNameOnly.c_str() ) == 0 )
			{
				Str::Copy( fileInfo.fileNameOnly, found.fileNameOnly );
				Str::Copy( fileInfo.fullPathName, found.fullPathName );
				Str::Copy( fileInfo.fullFileName, found.fullFileName );
				Str::Copy( fileInfo.relativePath, found.relativePath );
				//LogStream(LL_Debug) << "found '" << found.fileNameOnly << "' in '" << found.relativePath << "'";
				return true;
			}
			return false;
		}
	};

	FileSearcher	callback( fileName, fileInfo );
	Win32_ProcessFilesInDirectory( rootPath, &callback );

	return !fileInfo.fileNameOnly.IsEmpty();
}

ERet Win32_GetAbsoluteFilePath( const char* _rootPath, const char* _fileName, String &_filePath )
{
	SFindFileInfo	fileInfo;
	if( Win32_FindFileInDirectory( _rootPath, _fileName, fileInfo ) )
	{
		Str::Copy( _filePath, fileInfo.fullFileName );
		return ALL_OK;
	}
	return ERR_OBJECT_NOT_FOUND;
}

//----------------------------------------------------------//
//				End Of File.									//
//----------------------------------------------------------//
