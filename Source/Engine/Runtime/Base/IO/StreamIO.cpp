/*
=============================================================================
	File:	StreamIO.cpp
	Desc:	
=============================================================================
*/

#include <Base/Base_PCH.h>
#pragma hdrstop
#include <Base/Base.h>

//#define DBGSTREAMS	(MX_DEBUG)
#define DBGSTREAMS	0

/*================================
	AReader
================================*/

AReader::~AReader()
{
#if DBGSTREAMS
	mxGetLog().PrintF(LL_Trace,"~ Stream Reader: '%s'",this->DbgGetName());
#endif // MX_DEBUG
}

/*================================
	AWriter
================================*/

AWriter::~AWriter()
{
#if DBGSTREAMS
	mxGetLog().PrintF(LL_Trace,"~ Stream Writer: '%s'",this->DbgGetName());
#endif // MX_DEBUG
}

/*================================
	mxStreamWriter_CountBytes
================================*/

mxStreamWriter_CountBytes::mxStreamWriter_CountBytes()
{
	this->DbgSetName( "mxStreamWriter_CountBytes" );
	m_bytesWritten = 0;
}

mxStreamWriter_CountBytes::~mxStreamWriter_CountBytes()
{

}

ERet mxStreamWriter_CountBytes::Write( const void* pBuffer, size_t numBytes )
{
	m_bytesWritten += numBytes;
	return ALL_OK;
}

/*================================
	mxStreamReader_CountBytes
================================*/

mxStreamReader_CountBytes::mxStreamReader_CountBytes( AReader & stream )
	: m_reader( stream )
{
	this->DbgSetName( "mxStreamReader_CountBytes" );
	m_bytesRead = 0;
}

mxStreamReader_CountBytes::~mxStreamReader_CountBytes()
{

}
size_t mxStreamReader_CountBytes::Tell() const
{
	return m_bytesRead;
}
ERet mxStreamReader_CountBytes::Read( void *pBuffer, size_t numBytes )
{
	const size_t num_bytes_read = m_reader.Read( pBuffer, numBytes );
	m_bytesRead += num_bytes_read;
	return ALL_OK;
}

//RedirectingStream::RedirectingStream( AReader& source, AWriter &destination )
//	: m_source( source ), m_destination( destination )
//{}
//
//size_t RedirectingStream::Read( void *pBuffer, size_t numBytes )
//{
//	return m_destination.Write( pBuffer, numBytes );
//}
//
//size_t RedirectingStream::GetSize() const
//{
//	return m_source.GetSize();
//}


/*================================
		MemoryReader
================================*/

MemoryReader::MemoryReader( const void* data, size_t dataSize )
	: mData( c_cast(const char*)data ), mDataSize( dataSize ), mReadOffset( 0 )
{
	this->DbgSetName( "MemoryReader" );
	mxASSERT_PTR(data);
	mxASSERT(dataSize > 0);
}

//MemoryReader::MemoryReader( const Blob& memBlob )
//	: mData(memBlob.raw()), mDataSize(memBlob.rawSize()), mReadOffset( 0 )
//{
//}

MemoryReader::~MemoryReader()
{
	//const UINT num_bytes_read = c_cast(UINT) mReadOffset;
	//DBGOUT("~MemoryReader: %u bytes read.\n",num_bytes_read);
}
size_t MemoryReader::Tell() const
{
	return mReadOffset;
}
size_t MemoryReader::Length() const
{
	return mDataSize;
}

ERet MemoryReader::Read( void *pDest, size_t numBytes )
{
	mxASSERT(numBytes > 0);
	mxENSURE(
		mReadOffset < mDataSize + numBytes,
		ERR_FAILED_TO_READ_FILE,
		""
		);
	size_t bytesToRead = Min<size_t>( mDataSize - mReadOffset, numBytes );
	memcpy( pDest, mData + mReadOffset, bytesToRead );
	mReadOffset += bytesToRead;
	return ALL_OK;
}

/*================================
		InPlaceMemoryWriter
================================*/

InPlaceMemoryWriter::InPlaceMemoryWriter( void *dstBuf, size_t bufSize )
	: mBuffer( c_cast(BYTE*)dstBuf ), mCurrPos( mBuffer ), mMaxSize( bufSize )
{
	this->DbgSetName( "InPlaceMemoryWriter" );
	mxASSERT_PTR(dstBuf);
	mxASSERT(bufSize > 0);
}

InPlaceMemoryWriter::~InPlaceMemoryWriter()
{
	//const UINT bytesWritten = c_cast(UINT) this->BytesWritten();
	//DBGOUT("~InPlaceMemoryWriter: %u bytes written.\n",bytesWritten);
}

ERet InPlaceMemoryWriter::Write( const void *pSrc, size_t numBytes )
{
	const size_t writtenSoFar = this->BytesWritten();
	mxASSERT(writtenSoFar + numBytes < mMaxSize );
	size_t bytesToWrite = Min<size_t>( mMaxSize - writtenSoFar, numBytes );
	memcpy( mCurrPos, pSrc, bytesToWrite );
	mCurrPos += bytesToWrite;
	return ALL_OK;
}

ERet ByteWriter::Write( const void* data, size_t size )
{
	const UINT32 oldNum = m_data.num();
	const UINT32 newNum = oldNum + size;
	mxDO(m_data.setNum( newNum ));
	void* destination = mxAddByteOffset( m_data.raw(), oldNum );
	memcpy( destination, data, size );
	return ALL_OK;
}

ERet Writer_AlignForward(
						 AWriter& writer
						 , U32 cursor
						 , U32 alignment
						 )
{
	const U32 current_offset = cursor;
	const U32 alignedOffset = AlignUp( current_offset, alignment );
	const U32 sizeOfPadding = alignedOffset - current_offset;

	if( sizeOfPadding ) {
		void *	padding = _alloca( alignment );
		mxDO(writer.Write( padding, sizeOfPadding ));
	}

	return ALL_OK;
}

ERet Reader_Align_Forward( AReader& _stream, U32 alignment )
{
	const U32 current_offset = _stream.Tell();
	const U32 aligned_offset = AlignUp( current_offset, alignment );
	const U32 bytes_to_skip = aligned_offset - current_offset;
	if( bytes_to_skip ) {
		char	temporary_buffer[128];
		mxASSERT(alignment <= mxCOUNT_OF(temporary_buffer));
		mxDO(_stream.Read( temporary_buffer, bytes_to_skip ));
	}
	return ALL_OK;
}

ERet Stream_Fill(
				 AWriter& stream,
				 size_t bytes_to_write,
				 UINT32 value_to_write
				 )
{
	const void* source_buffer = &value_to_write;
	const size_t buffer_size = sizeof(value_to_write);

	const size_t num = bytes_to_write / buffer_size;
	const size_t rem = bytes_to_write - num * buffer_size;

	// write 'num' integers
	for( size_t i = 0; i < num; i++ )
	{
		mxDO(stream.Write( source_buffer, buffer_size ));
	}

	// write 'rem' bytes
	if( rem > 0 ) {
		mxDO(stream.Write( source_buffer, rem ));
	}

	return ALL_OK;
}


ERet Stream_WriteZeros(
					   AWriter& stream,
					   size_t bytes_to_write
					   )
{
	mxPREALIGN(16) BYTE source_buffer[256] = { 0 };

	const size_t buffer_size = sizeof(source_buffer);

	const size_t num = bytes_to_write / buffer_size;
	const size_t rem = bytes_to_write - num * buffer_size;

	for( size_t i = 0; i < num; i++ ) {
		mxDO(stream.Write( source_buffer, buffer_size ));
	}

	if( rem > 0 ) {
		mxDO(stream.Write( source_buffer, rem ));
	}

	return ALL_OK;
}

ERet Stream_Skip(
				 AReader& stream,
				 size_t bytes_to_skip,
				 void* temporary_buffer, size_t buffer_size
				 )
{
	const size_t num = bytes_to_skip / buffer_size;
	const size_t rem = bytes_to_skip - num * buffer_size;

	for( size_t i = 0; i < num; i++ )
	{
		mxDO(stream.Read( temporary_buffer, buffer_size ));
	}

	if( rem > 0 ) {
		mxDO(stream.Read( temporary_buffer, rem ));
	}

	return ALL_OK;
}

ERet Stream_Copy(
				 AReader& reader, AWriter &writer,
				 size_t bytes_to_copy,
				 void* temporary_buffer, size_t buffer_size
				)
{
	const size_t num = bytes_to_copy / buffer_size;
	const size_t rem = bytes_to_copy - num * buffer_size;

	for( UINT32 i = 0; i < num; i++ )
	{
		mxDO(reader.Read( temporary_buffer, buffer_size ));
		mxDO(writer.Write( temporary_buffer, buffer_size ));
	}

	if( rem > 0 ) {
		mxDO(reader.Read( temporary_buffer, rem ));
		mxDO(writer.Write( temporary_buffer, rem ));
	}

	return ALL_OK;
}



namespace AReader_
{
	ERet Align(
		AReader & reader,
		const U32 alignment
		)
	{
		return Reader_Align_Forward( reader, alignment );
	}

}//namespace
