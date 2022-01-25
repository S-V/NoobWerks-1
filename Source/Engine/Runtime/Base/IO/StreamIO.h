/*
=============================================================================
	File:	StreamIO.h
	Desc:	Basic I/O.
=============================================================================
*/
#pragma once

#include <Base/IO/AbstractReader.h>
#include <Base/IO/AbstractWriter.h>


//
//	mxStreamWriter_CountBytes
//
class mxStreamWriter_CountBytes: public AWriter
{
	size_t	m_bytesWritten;

public:
	mxStreamWriter_CountBytes();
	~mxStreamWriter_CountBytes();

	virtual ERet Write( const void* buffer, size_t numBytes ) override;

	inline size_t NumBytesWritten() const { return m_bytesWritten; }
};

//
//	mxStreamReader_CountBytes
//
class mxStreamReader_CountBytes: public AReader
{
	AReader &	m_reader;

	size_t	m_bytesRead;

public:
	mxStreamReader_CountBytes( AReader & stream );
	~mxStreamReader_CountBytes();

	virtual size_t Tell() const override;
	virtual ERet Read( void *buffer, size_t numBytes ) override;

	inline size_t NumBytesRead() const { return m_bytesRead; }
};

//class RedirectingStream: public AReader
//{
//	AReader &	m_source;
//	AWriter &	m_destination;
//
//public:
//	RedirectingStream( AReader& source, AWriter &destination );
//
//	virtual ERet Read( void *buffer, size_t numBytes ) override;
//	virtual size_t GetSize() const override;
//};

class MemoryWriter: public AWriter
{
	void *			m_destination;
	const UINT32	m_bufferSize;
	UINT32			m_bytesWritten;

public:
	MemoryWriter( void* destination, UINT32 bufferSize, UINT32 startOffset = 0 )
		: m_destination( destination )
		, m_bufferSize( bufferSize )
	{
		m_bytesWritten = startOffset;
	}
	virtual ERet Write( const void* buffer, size_t numBytes ) override
	{
		chkRET_X_IF_NOT(m_bytesWritten + numBytes <= m_bufferSize, ERR_BUFFER_TOO_SMALL);
		memcpy(m_destination, buffer, numBytes);
		m_destination = mxAddByteOffset(m_destination, numBytes);
		m_bytesWritten += numBytes;
		return ALL_OK;
	}
	inline size_t NumBytesWritten() const { return m_bytesWritten; }
};


//
//	MemoryReader
//
class MemoryReader: public AReader
{
public:
	MemoryReader( const void* data, size_t dataSize );
	//MemoryReader( const Blob& memBlob );
	~MemoryReader();

	virtual size_t Tell() const override;
	virtual size_t Length() const override;
	virtual ERet Read( void *pDest, size_t numBytes ) override;


	inline const char* bufferStart() const
	{
		return mData;
	}
	inline char* bufferStart()
	{
		return const_cast<char*>( mData );
	}

	inline const char* GetPtr() const
	{
		return mData + mReadOffset;
	}
	inline char* GetPtr()
	{
		return const_cast<char*>( mData + mReadOffset );
	}
	inline size_t BytesRead() const
	{
		return mReadOffset;
	}

	inline size_t Size() const
	{
		return mDataSize;
	}

	inline ERet Seek( const size_t absolute_offset )
	{
		if( absolute_offset <= mDataSize ) {
			mReadOffset = absolute_offset;
			return ALL_OK;
		}
		return ERR_FAILED_TO_SEEK_FILE;
	}

	inline ERet Skip( const size_t num_bytes_to_skip )
	{
		const size_t new_read_offset = mReadOffset + num_bytes_to_skip;
		return this->Seek( new_read_offset );
	}

private:
	const char* mData;
	size_t mReadOffset;
	const size_t mDataSize;
};

//
//	InPlaceMemoryWriter
//
class InPlaceMemoryWriter: public AWriter
{
public:
	InPlaceMemoryWriter( void *dstBuf, size_t bufSize );
	~InPlaceMemoryWriter();

	virtual ERet Write( const void *pSrc, size_t numBytes ) override;

	inline const BYTE* GetPtr() const
	{
		return mCurrPos;
	}
	inline size_t BytesWritten() const
	{
		return mCurrPos - mBuffer;
	}

private:
	BYTE	*mBuffer;
	BYTE	*mCurrPos;
	const size_t mMaxSize;
};

class NullWriter: public AWriter
{
public:
	size_t	bytes_written;

public:
	NullWriter()
	{
		bytes_written = 0;
	}

	virtual ERet Write( const void* buffer, size_t numBytes ) override
	{
		(void)buffer;
		bytes_written += numBytes;
		return ALL_OK;
	}
};



ERet Writer_AlignForward(
						 AWriter& writer
						 , U32 cursor
						 , U32 alignment
						 );

mxDEPRECATED
ERet Reader_Align_Forward( AReader& _stream, U32 alignment );

ERet Stream_Skip(
				 AReader& stream,
				 size_t bytes_to_skip,
				 void* temporary_buffer, size_t buffer_size
				 );

/// very inefficient
ERet Stream_Fill(
				 AWriter& stream,
				 size_t bytes_to_write,
				 UINT32 value_to_write = 0
				 );

ERet Stream_WriteZeros(
					   AWriter& stream,
					   size_t bytes_to_write
					   );

ERet Stream_Copy(
				 AReader& reader, AWriter &writer,
				 size_t bytes_to_copy,
				 void* temporary_buffer, size_t buffer_size
				 );

void EncodeString_Xor( const char* src, size_t size, UINT32 key, char *dest );
void DecodeString_Xor( const char* src, size_t size, UINT32 key, char *dest );


void WriteEncodedString( AWriter & stream, const char* src, size_t size, UINT32 seed );
void ReadDecodedString( AReader & stream, char* dest, UINT32 seed );

//----------------------------------------------------------//
//	Basic I/O.
//----------------------------------------------------------//

class AObject;

/*
-------------------------------------------------------------------------
	mxArchive
	used for 'manual' binary serialization
-------------------------------------------------------------------------
*/
struct mxArchive
{
	// serialization of POD objects
	virtual void SerializeMemory( void* ptr, size_t size ) = 0;

	// serialization of (references to) non-POD objects
	virtual void SerializePointer( AObject *& o ) {Unimplemented;}

	// registers a top-level object
	virtual void InsertRootObject( AObject* o ) {Unimplemented;}

public:
	virtual AWriter* IsWriter() {return nil;}
	virtual AReader* IsReader() {return nil;}

	// IsSaving()
	inline bool IsWriting() { return this->IsWriter() != nil; }
	// IsLoading()
	inline bool IsReading() { return this->IsReader() != nil; }

protected:
	virtual ~mxArchive()
	{}
};




// (de-)serializes from/to archive via << and >> stream operators
//
template< typename T >
inline mxArchive& Serialize_ArcViaStreams( mxArchive& archive, T & o )
{
	if( AWriter* stream = archive.IsWriter() )
	{
		*stream << o;
	}
	if( AReader* stream = archive.IsReader() )
	{
		*stream >> o;
	}
	return archive;
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

template< typename T >
mxArchive& TSerializeArray( mxArchive & archive, T* a, UINT32 num )
{
	//NOTE: array elements must be processed one by one in case they are pointers
	for( UINT32 i = 0; i < num; i++ )
	{
		archive && a[i];
	}

	return archive;
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

template< class S, class T, const size_t size >
void TSerializeArray( S& s, T (&a)[size] )
{
	mxSTATIC_ASSERT( size > 0 );
	if( TypeTrait< T >::IsPlainOldDataType )
	{
		s.SerializeMemory( a, size * sizeof T );
	}
	else
	{
		for( UINT32 i = 0; i < size; i++ )
		{
			s & a[i];
		}
	}
	return s;
}

template< class ARRAY >
ERet AWriter_writeArrayAsRawBytes( const ARRAY& the_array, AWriter &stream )
{
	return stream.Write(
		the_array.raw()
		, the_array.rawSize()
		);
}




//-----------------------------------------------------------------------
//	Helper macros
//-----------------------------------------------------------------------



#define mxDECLARE_STREAMABLE( TYPE )\
	friend AWriter& operator << ( AWriter& stream, const TYPE& o );\
	friend AReader& operator >> ( AReader& stream, TYPE& o );

#define mxIMPLEMENT_FUNCTION_READ_SINGLE( TYPE, FUNCTION )	\
	mxFORCEINLINE TYPE FUNCTION( AReader & stream )	\
	{	\
		TYPE value;	\
		stream.Get(value);	\
		return value;	\
	}

#define mxIMPLEMENT_FUNCTION_WRITE_SINGLE( TYPE, FUNCTION )\
	mxFORCEINLINE ERet FUNCTION( AWriter & stream, const TYPE& value )\
	{\
		return stream.SerializeMemory( &value, sizeof(TYPE) );\
	}





mxIMPLEMENT_FUNCTION_READ_SINGLE(INT8,ReadInt8);
mxIMPLEMENT_FUNCTION_WRITE_SINGLE(INT8,WriteInt8);

mxIMPLEMENT_FUNCTION_READ_SINGLE(UINT8,ReadUInt8);
mxIMPLEMENT_FUNCTION_WRITE_SINGLE(UINT8,WriteUInt8);


mxIMPLEMENT_FUNCTION_READ_SINGLE(INT16,ReadInt16);
mxIMPLEMENT_FUNCTION_WRITE_SINGLE(INT16,WriteInt16);

mxIMPLEMENT_FUNCTION_READ_SINGLE(UINT16,ReadUInt16);
mxIMPLEMENT_FUNCTION_WRITE_SINGLE(UINT16,WriteUInt16);


mxIMPLEMENT_FUNCTION_READ_SINGLE(INT32,ReadInt32);
mxIMPLEMENT_FUNCTION_WRITE_SINGLE(INT32,WriteInt32);

mxIMPLEMENT_FUNCTION_READ_SINGLE(UINT32,ReadUInt32);
mxIMPLEMENT_FUNCTION_WRITE_SINGLE(UINT32,WriteUInt32);

mxIMPLEMENT_FUNCTION_READ_SINGLE(INT64,ReadInt64);
mxIMPLEMENT_FUNCTION_WRITE_SINGLE(INT64,WriteInt64);

mxIMPLEMENT_FUNCTION_READ_SINGLE(UINT64,ReadUInt64);
mxIMPLEMENT_FUNCTION_WRITE_SINGLE(UINT64,WriteUInt64);


mxIMPLEMENT_FUNCTION_READ_SINGLE(FLOAT32,ReadFloat32);
mxIMPLEMENT_FUNCTION_WRITE_SINGLE(FLOAT32,WriteFloat32);

mxIMPLEMENT_FUNCTION_READ_SINGLE(FLOAT64,ReadFloat64);
mxIMPLEMENT_FUNCTION_WRITE_SINGLE(FLOAT64,WriteFloat64);

//mxIMPLEMENT_FUNCTION_READ_SINGLE(SIMD_QUAD,ReadSimdQuad);
//mxIMPLEMENT_FUNCTION_WRITE_SINGLE(SIMD_QUAD,WriteSimdQuad);
