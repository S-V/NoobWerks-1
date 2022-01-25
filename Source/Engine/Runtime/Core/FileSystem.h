/*
=============================================================================
	File:	FileSystem.h
	Desc:	
=============================================================================
*/
#pragma once

#if 0
class PakFile: public AssetPackage, public AFileSystem, DependsOn_Core
{
	String256	m_path;	// normalized directory name (ends with '/')

	// for detecting file changes and hot-reloading changed assets
	MyFileWatcher	m_fileWatcher;

public:
	DevAssetFolder();
	~DevAssetFolder();

	ERet Initialize();
	void Shutdown();

	//@ AFileSystem
	virtual ERet Mount( const char* path ) override;
	virtual void Unmount();

	//@ AssetPackage

	virtual ERet Open(
		const AssetKey& _key,
		AssetReader &_stream,
		const U32 _subresource = 0
	) override;
};
#endif
#if 0
mxSTOLEN("Crown engine");
/*
* Copyright (c) 2012-2014 Daniele Bartolini and individual contributors.
* License: https://github.com/taylor001/crown/blob/master/LICENSE
*/

class Compressor;

/*
--------------------------------------------------------------
	File
--------------------------------------------------------------
*/
class File: public AReader, public AWriter
{
public:
	virtual ~File() {};

#if 0
	/// Opens the file with the given @a mode
	File(AccessMode mode) : _open_mode(mode) {}
	/// Sets the position indicator of the file to position.
	virtual void seek(size_t position) = 0;
	/// Sets the position indicator to the end of the file
	virtual void seek_to_end() = 0;
	/// Sets the position indicator to bytes after current position
	virtual void skip(size_t bytes) = 0;
	/// Reads a block of data from the file.
	virtual void read(void* buffer, size_t size) = 0;
	/// Writes a block of data to the file.
	virtual void write(const void* buffer, size_t size) = 0;
	/// Copies a chunk of 'size' bytes of data from this to another file.
	virtual bool copy_to(File& file, size_t size = 0) = 0;
	/// Forces the previous write operations to complete.
	/// Generally, when a File is attached to a file,
	/// write operations are not performed instantly, the output data
	/// may be stored to a temporary buffer before making its way to
	/// the file. This method forces all the pending output operations
	/// to be written to the file.
	virtual void flush() = 0;
	/// Returns whether the file is valid.
	/// A file is valid when the buffer where it operates
	/// exists. (i.e. a file descriptor is attached to the file,
	/// a memory area is attached to the file etc.)
	virtual bool is_valid() = 0;
	/// Returns whether the position is at end of file.
	virtual bool end_of_file() = 0;
	/// Returns the size of file in bytes.
	virtual size_t size() = 0;
	/// Returns the current position in file.
	/// Generally, for binary data, it means the number of bytes
	/// from the beginning of the file.
	virtual size_t position() = 0;
	/// Returns whether the file can be read.
	virtual bool can_read() const = 0;
	/// Returns whether the file can be wrote.
	virtual bool can_write() const = 0;
	/// Returns whether the file can be sought.
	virtual bool can_seek() const = 0;
protected:
	AccessMode _open_mode;
#endif
};

/*
--------------------------------------------------------------
	Filesystem
--------------------------------------------------------------
*/
class Filesystem
{
public:
	Filesystem() {};
	virtual ~Filesystem() {};
	/// Opens the file at the given @a path with the given @a mode.
	virtual File* open(const char* path, OS::IO::AccessMode mode) = 0;
	/// Closes the given @a file.
	virtual void close(File* file) = 0;
	/// Returns whether @a path exists.
	virtual bool exists(const char* path) = 0;
	/// Returns true if @a path is a directory.
	virtual bool is_directory(const char* path) = 0;
	/// Returns true if @a path is a regular file.
	virtual bool is_file(const char* path) = 0;
	/// Creates the directory at the given @a path.
	virtual void create_directory(const char* path) = 0;
	/// Deletes the directory at the given @a path.
	virtual void delete_directory(const char* path) = 0;
	/// Creates the file at the given @a path.
	virtual void create_file(const char* path) = 0;
	/// Deletes the file at the given @a path.
	virtual void delete_file(const char* path) = 0;
	/// Returns the relative file names in the given @a path.
	virtual void list_files(const char* path, StringListT& files) = 0;
	/// Returns the absolute path of the given @a path based on
	/// the root path of the file source. If @a path is absolute,
	/// the given path is returned.
	virtual void get_absolute_path(const char* path, String& os_path) = 0;
private:
	// Disable copying
	Filesystem(const Filesystem&);
	Filesystem& operator=(const Filesystem&);
};

/*
--------------------------------------------------------------
	DiskFile
--------------------------------------------------------------
*/
class DiskFile: public File
{
	FileHandleT	m_fileHandle;

public:
	DiskFile(OS::IO::AccessMode mode, const char* filename);
	virtual ~DiskFile();

	//-- AReader
	virtual size_t Tell() const override;
	virtual size_t GetSize() const override;
	virtual ERet Read( void *buffer, size_t size ) override;

	//-- AWriter
	virtual ERet Write( const void* buffer, size_t size ) override;

	bool IsOpen() const;

	void Close();

#if 0
	/// Opens @a filename with specified @a mode
	DiskFile(AccessMode mode, const char* filename);
	virtual ~DiskFile();
	/// @copydoc File::seek()
	void seek(size_t position);
	/// @copydoc File::seek_to_end()
	void seek_to_end();
	/// @copydoc File::skip()
	void skip(size_t bytes);
	/// @copydoc File::read()
	void read(void* buffer, size_t size);
	/// @copydoc File::write()
	void write(const void* buffer, size_t size);
	/// @copydoc File::copy_to()
	bool copy_to(File& file, size_t size = 0);
	/// @copydoc File::flush()
	void flush();
	/// @copydoc File::end_of_file()
	bool end_of_file();
	/// @copydoc File::is_valid()
	bool is_valid();
	/// @copydoc File::size()
	size_t size();
	/// @copydoc File::position()
	size_t position();
	/// @copydoc File::can_read()
	bool can_read() const;
	/// @copydoc File::can_write()
	bool can_write() const;
	/// @copydoc File::can_seek()
	bool can_seek() const;
protected:
	OsFile _file;
	bool _last_was_read;
protected:
	inline void check_valid() const
	{
		CE_ASSERT(_file.is_open(), "File is not open");
	}
#endif
};

/*
--------------------------------------------------------------
	DiskFilesystem
--------------------------------------------------------------
*/
class DiskFilesystem: public Filesystem
{
public:
	/// Sets the root path to the current working directory of
	/// the engine executable.
	DiskFilesystem();
	/// Sets the root path to the given @a prefix.
	/// @note
	/// The @a prefix must be absolute.
	DiskFilesystem(const char* folder);

	ERet SetRootFolder(const char* folder);

	/// Opens the file at the given @a path with the given @a mode.
	File* open(const char* path, OS::IO::AccessMode mode);
	/// Closes the given @a file.
	void close(File* file);
	/// Returns whether @a path exists.
	bool exists(const char* path);
	/// Returns true if @a path is a directory.
	bool is_directory(const char* path);
	/// Returns true if @a path is a regular file.
	bool is_file(const char* path);
	/// Creates the directory at the given @a path.
	void create_directory(const char* path);
	/// Deletes the directory at the given @a path.
	void delete_directory(const char* path);
	/// Creates the file at the given @a path.
	void create_file(const char* path);
	/// Deletes the file at the given @a path.
	void delete_file(const char* path);
	/// Returns the relative file names in the given @a path.
	void list_files(const char* path, StringListT& files);
	/// Returns the absolute path of the given @a path based on
	/// the root path of the file source. If @a path is absolute,
	/// the given path is returned.
	void get_absolute_path(const char* path, String& os_path);
private:
	String256 m_folder;	// 'prefix'
};
#endif

	//struct AFileSystem
	//{
	//public:
	//	AFileSystem();
	//	~AFileSystem();

	//	ERet Initialize( const char* folder );
	//	void Shutdown();

	//	AFile* Open( const char* path, OS::IO::AccessMode mode );
	//	void Close( AFile* file );
	//};

struct AFile: public AReader, public AWriter
{
	FileHandleT	m_fileHandle;
public:
	AFile(OS::IO::AccessMode mode, const char* filename);
	virtual ~AFile();

	//@ AReader
	virtual size_t Tell() const override;
	virtual size_t Length() const override;
	virtual ERet Read( void *buffer, size_t size ) override;

	//@ AWriter
	virtual ERet Write( const void* buffer, size_t size ) override;

	bool IsOpen() const;

	void Close();
};

struct AFileSystem
{
	virtual ERet Mount( const char* path ) = 0;
	virtual void Unmount() = 0;
	//virtual ERet GetAbsolutePath( const char* _fileName, String &_filePath ) = 0;
protected:
	virtual ~AFileSystem() {}
};

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
