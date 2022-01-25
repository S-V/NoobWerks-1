/*
=============================================================================

	Based on Nebula 3's Blob.
=============================================================================
*/
#pragma once

#include <Base/Memory/MemoryBase.h>
#include <Base/IO/StreamIO.h>

///
///	Dynamically-sized raw memory blob
///
///	Memory blobs are always 16-byte aligned.
///
typedef DynamicArray< char, 16 >	NwBlob;

///
class NwBlobWriter: public AWriter
{
	NwBlob &	_buffer;	//TODO: {size, mem}, not {alloced_size, alloced_mem, used_size}
	size_t	_write_cursor;

public:
	enum Mode { APPEND, OVERWRITE };

	NwBlobWriter( NwBlob & buffer, Mode mode = APPEND );
	NwBlobWriter( NwBlob & buffer, U32 startOffset );

	virtual ERet Write( const void* source, size_t numBytes ) override;

	inline const char* raw() const
	{
		return _buffer.raw();
	}
	inline char* raw()
	{
		return _buffer.raw();
	}
	/// Returns the total size of stored elements, in bytes.
	inline size_t rawSize() const
	{
		return _buffer.rawSize();
	}
	inline size_t Tell() const
	{
		return _write_cursor;
	}
	inline const char* end() const
	{
		return _buffer.raw() + _write_cursor;
	}
	inline void Rewind( size_t _newOffset = 0 )
	{
		mxASSERT(_newOffset < _write_cursor);
		_write_cursor = _newOffset;
	}

	/// Use it only if you know what you're doing!
	inline void seekForward( size_t new_offset )
	{
		mxASSERT(new_offset > _write_cursor);
		_write_cursor = new_offset;
	}

	ERet allocateAndAdvanceWritePointer(
		const size_t num_bytes_to_allocate
		, void **_memory
		, const Arrays::ResizeBehavior resize_behaviour = Arrays::Preallocate
		);

	ERet reserveMoreBufferSpace(
		const size_t num_bytes_to_reserve
		, const Arrays::ResizeBehavior resize_behaviour = Arrays::Preallocate
		);

	//inline ERet Seek( size_t absOffset )
	//{
	//	mxDO(_buffer.reserve( absOffset ));
	//	_write_cursor = absOffset;
	//}

	mxFORCEINLINE ERet writeBlob( const NwBlob& blob )
	{
		mxASSERT(!blob.IsEmpty());
		return this->Write(
			blob.raw(),
			blob.rawSize()
			);
	}

	ERet alignBy( const U32 alignment );
};

inline
ERet Writer_Align16( NwBlobWriter & writer )
{
	const U32 current_offset = writer.Tell();
	const U32 alignedOffset = AlignUp( current_offset, 16 );
	const U32 sizeOfPadding = alignedOffset - current_offset;
	if( sizeOfPadding ) {
		char	padding[16] = {0};
		mxDO(writer.Write( padding, sizeOfPadding ));
	}
	return ALL_OK;
}

inline
ERet Writer_Align16( NwBlob & _blob )
{
	const U32 current_offset = _blob.num();
	const U32 alignedOffset = AlignUp( current_offset, 16 );
	const U32 sizeOfPadding = alignedOffset - current_offset;
	if( sizeOfPadding ) {
		return _blob.AllocateUninitialized( sizeOfPadding )
			? ALL_OK
			: ERR_OUT_OF_MEMORY;
	}
	return ALL_OK;
}

template< class HEADER >
ERet Writer_Alloc16_Header_And_Payload( NwBlob & _blob, HEADER *&_header, U32 _payloadSize = 0 )
{
	mxSTATIC_ASSERT(IS_16_BYTE_ALIGNED(sizeof(HEADER)));

	mxDO(Writer_Align16( _blob ));

	const U32 alignedOffset = _blob.num();
	const U32 newSize = alignedOffset + sizeof(HEADER) + _payloadSize;
	mxDO(_blob.setNum( newSize ));

	_header = (HEADER*) &(_blob[ alignedOffset ]);
	mxASSERT(IS_16_BYTE_ALIGNED(_header));

	mxZERO_OUT(*_header);
	return ALL_OK;
}



namespace NwBlob_
{
	/// NOTE: appends data to the blob!
	ERet loadBlobFromStream( AReader& stream, NwBlob &data );

	ERet loadBlobFromFile(
		NwBlob &data_
		, const char* filepath
		, FileReadFlags flags = FileRead_DefaultFlags
		);

	ERet saveBlobToFile( const NwBlob& data, const char* fileName, FileWriteFlags flags = FileWrite_DefaultFlags );
}//namespace NwBlob_







//
//	MemoryReader
//
class NwBlobReader: public MemoryReader
{
public:
	NwBlobReader( const NwBlob& blob )
		: MemoryReader( blob.raw(), blob.rawSize() )
	{}
};
