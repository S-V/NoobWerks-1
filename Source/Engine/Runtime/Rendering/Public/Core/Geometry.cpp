
/*
=============================================================================
	Graphics model used for rendering.
=============================================================================
*/
#include <Base/Base.h>
#pragma hdrstop
#include <Core/ObjectModel/Clump.h>
#include <GPU/Public/graphics_device.h>
#include <Rendering/Public/Core/Geometry.h>


namespace Rendering
{
mxBEGIN_REFLECT_ENUM( GeometryBufferType8 )
	mxREFLECT_ENUM_ITEM( Vertex, GeometryBufferType_Vertex ),
	mxREFLECT_ENUM_ITEM( Index, GeometryBufferType_Index ),
mxEND_REFLECT_ENUM

#if MX_DEVELOPER

static
const char* GeometryBufferType_name( GeometryBufferType type )
{
	return type == GeometryBufferType_Vertex ? "Vertex" : "Index";
}

#endif // MX_DEVELOPER

/*
-----------------------------------------------------------------------------
	NwGeometryBuffer
-----------------------------------------------------------------------------
*/
mxDEFINE_CLASS( NwGeometryBuffer );
mxBEGIN_REFLECTION( NwGeometryBuffer )
	mxMEMBER_FIELD( aligned_size ),
	mxMEMBER_FIELD( buffer_type ),
mxEND_REFLECTION

NwGeometryBuffer::NwGeometryBuffer()
{
	__nextfree = nil;

	aligned_size = 0;
	buffer_handle.SetNil();
	buffer_type = GeometryBufferType_Vertex;
}

NwGeometryBuffer::~NwGeometryBuffer()
{
	mxASSERT2(getReferenceCount() == 0, "The buffer is still referenced! You forgot to release a reference to the buffer!");
	mxASSERT2(buffer_handle.IsNil(), "The buffer handle is null. Did you forget to destroy the buffer?");
}

void NwGeometryBuffer::create(
							  const GeometryBufferType buffer_type
							  , const U32 size_aligned_to_pow_of_two
							  )
{
	NwBufferDescription	buffer_description(_InitDefault);
	{
		buffer_description.type = (buffer_type == GeometryBufferType_Vertex)
			? Buffer_Vertex
			: Buffer_Index
			;
		buffer_description.size = size_aligned_to_pow_of_two;
		buffer_description.flags = NwBufferFlags::Dynamic;
	}

	//
	this->buffer_handle = NGpu::CreateBuffer( buffer_description );
	this->aligned_size = size_aligned_to_pow_of_two;
	this->buffer_type = buffer_type;

	//DBGOUT("Create buffer %u: %u bytes", buffer_type, _size);
}

void NwGeometryBuffer::destroy()
{
//	DBGOUT("Destroy buffer %u: %u bytes", buffer_type, aligned_size);
	if( buffer_handle.IsValid() )
	{
		NGpu::DeleteBuffer(buffer_handle);
		buffer_handle.SetNil();
	}

	aligned_size = 0;
	buffer_type = GeometryBufferType_Vertex;
}

void NwGeometryBuffer::onReferenceCountDroppedToZero( NwGeometryBuffer * buffer )
{
	NwGeometryBufferPool::Get().releaseDynamicBuffer( buffer );
}

/*
-----------------------------------------------------------------------------
	NwGeometryBufferPool
-----------------------------------------------------------------------------
*/
NwGeometryBufferPool::NwGeometryBufferPool()
{
	mxZERO_OUT(_stats);
}

NwGeometryBufferPool::~NwGeometryBufferPool()
{
}

ERet NwGeometryBufferPool::Initialize( NwClump* buffer_storage_clump )
{
	_buffer_storage_clump = buffer_storage_clump;

	for( UINT iBufferType = 0; iBufferType < GeometryBufferType_COUNT; iBufferType++ )
	{
		for( UINT iSizeSlot = 0; iSizeSlot < NUM_BUFFER_SLOTS; iSizeSlot++ )
		{
			_pools[ iBufferType ][ iSizeSlot ].next_free = nil;
		}
	}

	mxZERO_OUT(_stats);

	return ALL_OK;
}

void NwGeometryBufferPool::Shutdown()
{
	mxTODO("destroy NwGeometryBuffer's");

	U32	num_buffers_destroyed = 0;

	//
	NwClump::Iterator< NwGeometryBuffer >	buffer_iter( *_buffer_storage_clump );
	while( buffer_iter.IsValid() )
	{
		NwGeometryBuffer & buffer = buffer_iter.Value();

		buffer.destroy();
		_buffer_storage_clump->Free( &buffer );

		++num_buffers_destroyed;

		buffer_iter.MoveToNext();
	}

	DBGOUT("[Renderer] Destroyed %u geometry buffers.", num_buffers_destroyed);

	_buffer_storage_clump = nil;
}

NwGeometryBuffer* NwGeometryBufferPool::_allocateDynamicBuffer(
	GeometryBufferType buffer_type
	, U32 requested_buffer_size
	)
{
	mxASSERT(requested_buffer_size >= MIN_BUFFER_SIZE && requested_buffer_size <= MAX_BUFFER_SIZE);

	// round the size up to a power of two
	const U32 aligned_size = CeilPowerOfTwo( requested_buffer_size );

	// get the pool index; all buffers in the pool have the same size
	const U32 slot_index = Log2OfPowerOfTwo( aligned_size );
	mxASSERT(slot_index < NUM_BUFFER_SLOTS);

	//
	// allocate a new buffer
	FreeListHead & free_list_head = _pools[ buffer_type ][ slot_index ];

	//
	NwGeometryBuffer* free_buffer = free_list_head.next_free;

	//
	if( free_buffer )
	{
		//DBGOUT("[Renderer] Found free %s buffer: size = %u, slot_index = %u",
		//	GeometryBufferType_name(buffer_type), requested_buffer_size, slot_index
		//	);

		//
		mxASSERT(
			free_buffer->buffer_handle.IsValid()
			&&
			free_buffer->aligned_size == aligned_size
			);

		//
		free_list_head.next_free = (NwGeometryBuffer*) free_buffer->__nextfree;

		++_stats.num_buffers_used_right_now;

		return free_buffer;
	}
	else
	{
		//DBGOUT("[Renderer] Allocating a new %s buffer: size = %u, slot_index = %u",
		//	GeometryBufferType_name(buffer_type), requested_buffer_size, slot_index
		//	);

		//
		NwGeometryBuffer *	new_buffer;
		mxENSURE(
			_buffer_storage_clump->New( new_buffer ) == ALL_OK,
			nil,
			"Failed to allocate new buffer of type: %d", buffer_type
			);

		// create a new dynamic buffer

		new_buffer->create(
			buffer_type
			, aligned_size
			);

		// update stats
		_stats.total_buffer_bytes_allocated += aligned_size;
		_stats.num_bytes_wasted_due_to_padding += (aligned_size - requested_buffer_size);
		++_stats.num_created_buffers_in_total;
		++_stats.num_buffers_used_right_now;

		return new_buffer;
	}
}

void NwGeometryBufferPool::releaseDynamicBuffer( NwGeometryBuffer* buffer )
{
	// don't destroy the buffer, simply add it to the free list for later reuse

	const U32 slot_index = Log2OfPowerOfTwo( buffer->aligned_size );

	FreeListHead & free_list_head = _pools[ buffer->buffer_type ][ slot_index ];

	buffer->__nextfree = free_list_head.next_free;

	free_list_head.next_free = buffer;

	//
	--_stats.num_buffers_used_right_now;

	// (all buffers will be destroyed on Shutdown)
}

NwGeometryBuffer* NwGeometryBufferPool::allocateDynamicVertexBuffer( U32 size_in_bytes )
{
	return this->_allocateDynamicBuffer(
		GeometryBufferType_Vertex
		, size_in_bytes
		);
}

NwGeometryBuffer* NwGeometryBufferPool::allocateDynamicIndexBuffer( U32 size_in_bytes )
{
	return this->_allocateDynamicBuffer(
		GeometryBufferType_Index
		, size_in_bytes
		);
}

/*
-----------------------------------------------------------------------------
	NwMeshBuffers
-----------------------------------------------------------------------------
*/
ERet NwMeshBuffers::updateWithRawData(
						 const void* _vertices,
						 const U32 _vertex_bytes,
						 const void* _indices,
						 const U32 _index_bytes
						 )
{
	NwGeometryBuffer::Ref new_VB = NwGeometryBufferPool::Get()
		.allocateDynamicVertexBuffer( _vertex_bytes );

	NwGeometryBuffer::Ref new_IB = NwGeometryBufferPool::Get()
		.allocateDynamicIndexBuffer( _index_bytes );

	if(mxUNLIKELY( new_VB.IsNull() || new_IB.IsNull() )) {
		return ERR_OUT_OF_MEMORY;
	}

	this->VB = new_VB;
	this->IB = new_IB;

	//
	NGpu::updateBuffer( new_VB->buffer_handle, NGpu::copy( _vertices, _vertex_bytes ) );
	NGpu::updateBuffer( new_IB->buffer_handle, NGpu::copy( _indices, _index_bytes ) );

	return ALL_OK;
}

ERet NwMeshBuffers::updateVertexBuffer(
	const NGpu::Memory* vertex_data_update_mem
	)
{
	this->VB = NwGeometryBufferPool::Get()
		.allocateDynamicVertexBuffer( vertex_data_update_mem->size );
	mxENSURE( this->VB.IsValid(), ERR_OUT_OF_MEMORY, "" );

	NGpu::updateBuffer( this->VB->buffer_handle, vertex_data_update_mem );

	return ALL_OK;
}

ERet NwMeshBuffers::updateIndexBuffer(
	const NGpu::Memory* index_data_update_mem
	)
{
	this->IB = NwGeometryBufferPool::Get().allocateDynamicIndexBuffer( index_data_update_mem->size );
	mxENSURE( this->IB.IsValid(), ERR_OUT_OF_MEMORY, "" );

	NGpu::updateBuffer( this->IB->buffer_handle, index_data_update_mem );

	return ALL_OK;
}

void NwMeshBuffers::release()
{
	this->VB = nil;
	this->IB = nil;
}

}//namespace Rendering
