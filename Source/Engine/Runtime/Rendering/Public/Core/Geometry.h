/*
	Graphics mesh buffer management.
*/
#pragma once

#include <Base/Memory/FreeList/FreeList.h>	// FreeListElement
#include <Base/Template/SmartPtr/TReferenceCounted.h>

#include <GPU/Public/graphics_types.h>	// HBuffer
#include <Rendering/BuildConfig.h>


class NwClump;

namespace Rendering
{
/*
=====================================================================
	GEOMETRY
=====================================================================
*/

enum GeometryBufferType
{
	GeometryBufferType_Vertex,
	GeometryBufferType_Index,

	GeometryBufferType_COUNT
};
mxDECLARE_ENUM( GeometryBufferType, U8, GeometryBufferType8 );


/// Represents a dynamic vertex or index buffer.
struct NwGeometryBuffer
	: CStruct
	, TReferenceCounted< NwGeometryBuffer >
	, FreeListElement
{
	typedef TRefPtr< NwGeometryBuffer >	Ref;

public:
	U32					aligned_size;
	HBuffer				buffer_handle;
	GeometryBufferType8	buffer_type;

	// 20/24

public:
	mxDECLARE_CLASS( NwGeometryBuffer, CStruct );
	mxDECLARE_REFLECTION;
	NwGeometryBuffer();
	~NwGeometryBuffer();

public:	// Internal

	void create(
		const GeometryBufferType buffer_type
		, const U32 size_aligned_to_pow_of_two
		);

	void destroy();

	// TReferenceCounted
	static void onReferenceCountDroppedToZero( NwGeometryBuffer * buffer );
};

///
struct NwGeometryBufferPoolStats
{
	U32	total_buffer_bytes_allocated;
	U32	num_bytes_wasted_due_to_padding;

	U16	num_created_buffers_in_total;
	U16	num_buffers_used_right_now;
};

/*
-----------------------------------------------------------------------------
	NwGeometryBufferPool

	for pooling vertex and index buffers
-----------------------------------------------------------------------------
*/
class NwGeometryBufferPool: public TGlobal< NwGeometryBufferPool >
{
	static const U32 MIN_BUFFER_SIZE = 8;
	static const U32 MAX_BUFFER_SIZE = nwRENDERER_MAX_GEOMETRY_BUFFER_SIZE;

	enum
	{
		NUM_BUFFER_SLOTS = TLog2<MAX_BUFFER_SIZE>::value,
	};

	struct FreeListHead
	{
		NwGeometryBuffer *	next_free;
	};
	FreeListHead	_pools[GeometryBufferType_COUNT][NUM_BUFFER_SLOTS];

	//
	TPtr< NwClump >	_buffer_storage_clump;

	//
	NwGeometryBufferPoolStats		_stats;

public:
	NwGeometryBufferPool();
	~NwGeometryBufferPool();

	ERet Initialize( NwClump* buffer_storage_clump );
	void Shutdown();

public:	// API

	NwGeometryBuffer* allocateDynamicVertexBuffer(
		U32 size_in_bytes
		);
	NwGeometryBuffer* allocateDynamicIndexBuffer(
		U32 size_in_bytes
		);

	void releaseDynamicBuffer(
		NwGeometryBuffer* buffer
		);

	const NwGeometryBufferPoolStats& getStats() const { return _stats; }

private:
	NwGeometryBuffer* _allocateDynamicBuffer(
		GeometryBufferType buffer_type
		, U32 requested_buffer_size
		);
};

/*
-----------------------------------------------------------------------------
	NwMeshBuffers
-----------------------------------------------------------------------------
*/
struct NwMeshBuffers
{
	NwGeometryBuffer::Ref	VB;	//!< Vertex buffer handle.
	NwGeometryBuffer::Ref	IB;	//!< Index buffer handle.

public:
	ERet updateWithRawData(
		const void* _vertices,
		const U32 _vertex_bytes,
		const void* _indices,
		const U32 _index_bytes
		);

	ERet updateVertexBuffer(
		const NGpu::Memory* vertex_data_update_mem
		);
	ERet updateIndexBuffer(
		const NGpu::Memory* index_data_update_mem
		);

	void release();
};


/*
//
//	EHWBufferAlloc -
//		enumerates ways of creating hardware geometry buffers
//		for render models with dynamic geometry.
//
enum EHWBufferAlloc
{
	HWA_Fixed			= 0,	// Create a non-growing buffer with the size enough to hold initial geometry; the buffer can still be written into, but not resized.
	HWA_Reserve			= 1,	// Create a fixed size buffer with some space reserved; the user must specify exactly how much space should be reserved.
	HWA_Discardable		= 2,	// Discardable buffers can be created and destroyed just as needed to hold new geometry.
};

//
//	EHWBufferUpdate - strategy of updating hardware geometry buffers.
//
enum EHWBufferUpdate
{
	HWU_NeverUpdate		= 0,	// Don't change the buffer.
	HWU_WriteExisting	= 1,	// Write new geometry into existing buffer (discard the new data if the buffer has no space left).
	HWU_GrowAsNeeded	= 2,	// Create a new buffer with a size enough to hold new geometry and discard the old buffer, if needed.
	HWU_GrowByChunks	= 3,	// Create a new buffer with a size enough to hold new geometry and the size must be a multiple of the specified granularity, discard the old buffer, if needed.
	HWU_GrowFromPool	= 4,	// Allocate space to hold new data from special pre-allocated dynamic buffers, if needed.
};
*/

}//namespace Rendering
