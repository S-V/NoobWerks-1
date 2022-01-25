/*
=============================================================================
	File:	FileSystem.cpp
	Desc:	
=============================================================================
*/
#include <Core/Core_PCH.h>
#pragma hdrstop

// for _getcwd()
#include <direct.h>

#include <Core/Core.h>
#include <Core/FileSystem.h>

#if 0
/*
--------------------------------------------------------------
	DiskFile
--------------------------------------------------------------
*/
DiskFile::DiskFile(OS::IO::AccessMode mode, const char* filename)
{
	m_fileHandle = OS::IO::Create_File(filename, mode);
}
DiskFile::~DiskFile()
{
	this->Close();
}
size_t DiskFile::Tell() const
{
	return OS::IO::Tell_File_Position(m_fileHandle);
}
size_t DiskFile::GetSize() const
{
	mxASSERT(this->IsOpen());
	return OS::IO::Get_File_Size(m_fileHandle);
}
ERet DiskFile::Read( void *buffer, size_t size )
{
	mxASSERT(this->IsOpen());
	const size_t read = OS::IO::Read_File( m_fileHandle, buffer, size );
	return (read == size) ? ALL_OK : ERR_FAILED_TO_READ_FILE;
}
ERet DiskFile::Write( const void* buffer, size_t size )
{
	mxASSERT(this->IsOpen());
	const size_t written = OS::IO::Write_File( m_fileHandle, buffer, size );
	return (written == size) ? ALL_OK : ERR_FAILED_TO_WRITE_FILE;
}
bool DiskFile::IsOpen() const
{
	return m_fileHandle != INVALID_HANDLE_VALUE;
}
void DiskFile::Close()
{
	if( m_fileHandle != INVALID_HANDLE_VALUE ) {
		OS::IO::Close_File( m_fileHandle );
		m_fileHandle = INVALID_HANDLE_VALUE;
	}	
}

/*
--------------------------------------------------------------
	DiskFilesystem
--------------------------------------------------------------
*/
DiskFilesystem::DiskFilesystem()
{
	char buffer[512];
	char* path = _getcwd( buffer, mxCOUNT_OF(buffer) );	// Gets the current working directory.
	this->SetRootFolder(path);
}
DiskFilesystem::DiskFilesystem(const char* folder)
{
	this->SetRootFolder(folder);
}
ERet DiskFilesystem::SetRootFolder(const char* folder)
{
	mxASSERT_PTR(folder);
	Str::CopyS(m_folder,folder);
	Str::NormalizePath(m_folder);
	chkRET_X_IF_NOT(OS::IO::PathExists(folder), ERR_FILE_OR_PATH_NOT_FOUND);
	return ALL_OK;
}
File* DiskFilesystem::open(const char* path, OS::IO::AccessMode mode)
{
	mxASSERT_PTR(path);
	String256 abs_path;
	get_absolute_path(path, abs_path);
	return MAKE_NEW(foundation::memory_globals::default_allocator(), DiskFile, mode, abs_path.c_str());
}
void DiskFilesystem::close(File* file)
{
	mxASSERT_PTR(file);
	MAKE_DELETE(foundation::memory_globals::default_allocator(), File, file);
}
bool DiskFilesystem::exists(const char* path)
{
	mxASSERT_PTR(path);
	String256 abs_path;
	get_absolute_path(path, abs_path);
	return OS::IO::FileExists(abs_path.c_str());
}
bool DiskFilesystem::is_directory(const char* path)
{
	mxASSERT_PTR(path);
	String256 abs_path;
	get_absolute_path(path, abs_path);
	return OS::IO::Is_Directory(abs_path.c_str());
}
bool DiskFilesystem::is_file(const char* path)
{
	mxASSERT_PTR(path);
	String256 abs_path;
	get_absolute_path(path, abs_path);
	return OS::IO::Is_File(abs_path.c_str());
}
void DiskFilesystem::create_directory(const char* path)
{
	mxASSERT_PTR(path);
	String256 abs_path;
	get_absolute_path(path, abs_path);
	if (!OS::IO::PathExists(abs_path.c_str()))
		OS::IO::MakeDirectory(abs_path.c_str());
}
void DiskFilesystem::delete_directory(const char* path)
{
	mxASSERT_PTR(path);
	String256 abs_path;
	get_absolute_path(path, abs_path);
	OS::IO::DeleteDirectory(abs_path.c_str());
}
void DiskFilesystem::create_file(const char* path)
{
	mxASSERT_PTR(path);
	String256 abs_path;
	get_absolute_path(path, abs_path);
	OS::IO::Create_File(abs_path.c_str(), OS::IO::ReadWriteAccess);
}
void DiskFilesystem::delete_file(const char* path)
{
	mxASSERT_PTR(path);
	String256 abs_path;
	get_absolute_path(path, abs_path);
	OS::IO::Delete_File(abs_path.c_str());
}
void DiskFilesystem::list_files(const char* path, StringListT& files)
{
	mxASSERT_PTR(path);
	String256 abs_path;
	get_absolute_path(path, abs_path);
	UNDONE;
	//OS::IO::list_files(abs_path.c_str(), files);
}
void DiskFilesystem::get_absolute_path(const char* path, String& os_path)
{
	if (OS::Path::IsAbsolute(path))
	{
		Str::CopyS( os_path, path );
		return;
	}
	Str::Append( os_path, m_folder );
	Str::AppendS( os_path, path );
}
#endif


AFile::AFile(OS::IO::AccessMode mode, const char* filename)
{
	m_fileHandle = OS::IO::Create_File(filename, mode);
}
AFile::~AFile()
{
	this->Close();
}
size_t AFile::Tell() const
{
	return OS::IO::Tell_File_Position(m_fileHandle);
}
size_t AFile::Length() const
{
	mxASSERT(this->IsOpen());
	return OS::IO::Get_File_Size(m_fileHandle);
}
ERet AFile::Read( void *buffer, size_t size )
{
	mxASSERT(this->IsOpen());
	const size_t read = OS::IO::Read_File( m_fileHandle, buffer, size );
	return (read == size) ? ALL_OK : ERR_FAILED_TO_READ_FILE;
}
ERet AFile::Write( const void* buffer, size_t size )
{
	mxASSERT(this->IsOpen());
	const size_t written = OS::IO::Write_File( m_fileHandle, buffer, size );
	return (written == size) ? ALL_OK : ERR_FAILED_TO_WRITE_FILE;
}
bool AFile::IsOpen() const
{
	return m_fileHandle != INVALID_HANDLE_VALUE;
}
void AFile::Close()
{
	if( m_fileHandle != INVALID_HANDLE_VALUE ) {
		OS::IO::Close_File( m_fileHandle );
		m_fileHandle = INVALID_HANDLE_VALUE;
	}	
}

#if 0
AFileSystem::AFileSystem()
{
}
AFileSystem::~AFileSystem()
{
}
ERet AFileSystem::Initialize( const char* folder )
{
}
void AFileSystem::Shutdown()
{
}
AFile* AFileSystem::Open( const char* path, OS::IO::AccessMode mode )
{
}
void AFileSystem::Close( AFile* file )
{
}
#endif

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
