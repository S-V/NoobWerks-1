#pragma once

#include <Base/Template/Containers/Array/DynamicArray.h>
#include <Base/Template/Containers/Blob.h>

#define	VERIFY_FORMAT_STRING

typedef int ID_TIME_T;

/*
==============================================================

  File Streams.

==============================================================
*/

// mode parm for Seek
typedef enum {
	FS_SEEK_CUR,
	FS_SEEK_END,
	FS_SEEK_SET
} fsOrigin_t;

class idFileSystemLocal;


class idFile {
public:
	virtual					~idFile() {};

							// Get the name of the file.
	virtual const char *	GetName() const
	{
		return "?";
	}
							// Get the full file path.
	virtual const char *	GetFullPath() const
	{
		return "?";
	}

							// Read data from the file to the buffer.
	virtual int				Read( void *buffer, int len )
	{
		UNDONE;
		return 0;
	}
							// Write data from the buffer to the file.
	virtual int				Write( const void *buffer, int len )
	{
		UNDONE;
		return 0;
	}
							// Returns the length of the file.
	virtual int				Length() const
	{
		UNDONE;
		return 0;
	}

							// Return a time value for reload operations.
	virtual ID_TIME_T		Timestamp() const
	{
		UNDONE;
		return 0;
	}
							// Returns offset in file.
	virtual int				Tell() const
	{
		UNDONE;
		return 0;
	}
	//						// Forces flush on files being writting to.
	virtual void			ForceFlush()
	{
	}
	//						// Causes any buffered data to be written to the file.
	//virtual void			Flush();
	//						// Seek on a file.
	//virtual int				Seek( long offset, fsOrigin_t origin );
	//						// Go back to the beginning of the file.
	//virtual void			Rewind();
};


class idFileWriter : public idFile {
	NwBlobWriter	m_writer;
public:
	idFileWriter( NwBlob & _blob ) : m_writer(_blob) {}

						// Get the name of the file.
	virtual const char *	GetName() const
	{
		return "?";
	}
							// Get the full file path.
	virtual const char *	GetFullPath() const
	{
		return "?";
	}

							// Read data from the file to the buffer.
	virtual int				Read( void *buffer, int len )
	{
		UNDONE;
		return 0;
	}
							// Write data from the buffer to the file.
	virtual int				Write( const void *buffer, int len )
	{
		m_writer.Write( buffer, len );
		return len;
	}
							// Returns the length of the file.
	virtual int				Length() const
	{
		return m_writer.Tell();
	}

							// Return a time value for reload operations.
	virtual ID_TIME_T		Timestamp() const
	{
		return 0;
	}
							// Returns offset in file.
	virtual int				Tell() const
	{
		return m_writer.Tell();
	}
};


class idFileReader : public idFile {
	NwBlob & m_blob;
	size_t m_readOffset;
public:
	idFileReader( NwBlob & _blob ) : m_blob(_blob), m_readOffset(0) {}

							// Get the name of the file.
	virtual const char *	GetName() const
	{
		return "?";
	}
							// Get the full file path.
	virtual const char *	GetFullPath() const
	{
		return "?";
	}

							// Read data from the file to the buffer.
	virtual int				Read( void *buffer, int len )
	{
		int bytesToRead = smallest( len, m_blob.rawSize() - m_readOffset );
		void* mem = mxAddByteOffset(m_blob.raw(), m_readOffset);
		memcpy(buffer, mem, bytesToRead);
		m_readOffset += bytesToRead;
		return bytesToRead;
	}
							// Write data from the buffer to the file.
	virtual int				Write( const void *buffer, int len )
	{
		UNDONE;
		return len;
	}
							// Returns the length of the file.
	virtual int				Length() const
	{
		return m_blob.rawSize();
	}

							// Return a time value for reload operations.
	virtual ID_TIME_T		Timestamp() const
	{
		return 0;
	}
							// Returns offset in file.
	virtual int				Tell() const
	{
		return m_readOffset;
	}
	//						// Forces flush on files being writting to.
	virtual void			ForceFlush()
	{
	}
	//						// Causes any buffered data to be written to the file.
	//virtual void			Flush();
	//						// Seek on a file.
	//virtual int				Seek( long offset, fsOrigin_t origin );
	//						// Go back to the beginning of the file.
	//virtual void			Rewind();
};

/*
===============================================================================

	idCompressor is a layer ontop of idFile which provides lossless data
	compression. The compressor can be used as a regular file and multiple
	compressors can be stacked ontop of each other.

===============================================================================
*/

class idCompressor /*: public idFile*/ {
public:
							// compressor allocation
	static idCompressor *	AllocNoCompression();
	static idCompressor *	AllocBitStream();
	static idCompressor *	AllocRunLength();
	static idCompressor *	AllocRunLength_ZeroBased();
	static idCompressor *	AllocHuffman();
	static idCompressor *	AllocArithmetic();
	static idCompressor *	AllocLZSS();
	static idCompressor *	AllocLZSS_WordAligned();
	static idCompressor *	AllocLZW();

							// initialization
	virtual void			Init( idFile *f, bool compress, int wordLength ) = 0;
	virtual void			FinishCompress() = 0;
	virtual float			GetCompressionRatio() const = 0;

							// common idFile interface
	virtual const char *	GetName() = 0;
	virtual const char *	GetFullPath() = 0;
	virtual int				Read( void *outData, int outLength ) = 0;
	virtual int				Write( const void *inData, int inLength ) = 0;
	virtual int				Length() = 0;
	virtual ID_TIME_T		Timestamp() = 0;
	virtual int				Tell() = 0;
	virtual void			ForceFlush() = 0;
	virtual void			Flush() = 0;
	virtual int				Seek( long offset, fsOrigin_t origin ) = 0;
};
