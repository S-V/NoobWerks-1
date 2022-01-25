#include <Base/Base_PCH.h>
#pragma hdrstop
#include <Base/Base.h>
#include <Base/Template/Containers/Blob.h>


NwBlobWriter::NwBlobWriter( NwBlob & buffer, Mode mode )
	: _buffer( buffer )
{
	this->DbgSetName( "NwBlobWriter" );
	_write_cursor = (mode == APPEND) ? buffer.rawSize() : 0;
}

NwBlobWriter::NwBlobWriter( NwBlob & buffer, U32 startOffset )
	: _buffer( buffer )
{
	this->DbgSetName( "NwBlobWriter" );
	_write_cursor = startOffset;
}

ERet NwBlobWriter::Write( const void* source, size_t numBytes )
{
	const size_t totalBytes =  _write_cursor + numBytes;
	mxDO(_buffer.setNum( totalBytes ));
	memcpy( _buffer.raw() + _write_cursor, source, numBytes );
	_write_cursor = totalBytes;
	return ALL_OK;
}

ERet NwBlobWriter::allocateAndAdvanceWritePointer(
						  const size_t num_bytes_to_allocate
						  , void **_memory
						  , const Arrays::ResizeBehavior resize_behaviour
						  )
{
	mxASSERT(num_bytes_to_allocate);
	const size_t old_offset = _write_cursor;
	mxDO(this->reserveMoreBufferSpace( num_bytes_to_allocate, resize_behaviour ));
	_write_cursor = _buffer.num();
	*_memory = &(_buffer[ old_offset ]);
	return ALL_OK;
}

ERet NwBlobWriter::reserveMoreBufferSpace(
										const size_t num_bytes_to_reserve
										, const Arrays::ResizeBehavior resize_behaviour
										)
{
	mxASSERT(num_bytes_to_reserve);
	const size_t current_offset = _write_cursor;
	const size_t new_buffer_size = current_offset + num_bytes_to_reserve;

	if( resize_behaviour == Arrays::Preallocate ) {
		mxDO(_buffer.setNum( new_buffer_size ));
	} else {
		mxDO(_buffer.setCountExactly( new_buffer_size ));
	}

	return ALL_OK;
}

ERet NwBlobWriter::alignBy( const U32 alignment )
{
	return Writer_AlignForward(
		*this
		, this->Tell()
		, alignment
		);
}

namespace NwBlob_
{
	ERet loadBlobFromStream( AReader& stream, NwBlob &data )
	{
		const size_t fileSize = stream.Length();
		mxASSERT(fileSize > 0);
		if( fileSize  == 0 ) {
			DBGOUT("File '%s' has zero size.\n", stream.DbgGetName());
			return ERR_FILE_HAS_ZERO_SIZE;
		}

		const size_t curr_size = data.num();
		mxDO(data.setNum(curr_size + fileSize));
		mxDO(stream.Read(
			mxAddByteOffset(data.raw(), curr_size),
			fileSize
			));

		return ALL_OK;
	}

	ERet loadBlobFromFile(
		NwBlob &data_
		, const char* filepath
		, FileReadFlags flags
		)
	{
		FileReader	file;
		mxTRY(file.Open( filepath, FileRead_NoErrors ));

		return NwBlob_::loadBlobFromStream( file, data_ );
	}

	ERet saveBlobToFile( const NwBlob& data, const char* fileName, FileWriteFlags flags )
	{
		FileWriter	file;
		mxTRY(file.Open( fileName, FileWrite_NoErrors ));
		mxTRY(file.Write( data.raw(), data.rawSize() ));
		return ALL_OK;
	}
}//namespace NwBlob_
