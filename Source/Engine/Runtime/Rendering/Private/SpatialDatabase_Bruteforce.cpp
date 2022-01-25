// Visibility Culling
#include <bx/string.h>
#include <Base/Base.h>
#pragma hdrstop

#include <Base/Math/BoundingVolumes/ViewFrustum.h>
#include <Base/Template/HandleAllocator.h>

#include <Rendering/Public/Settings.h>
#include <Rendering/Public/Globals.h>
#include <Rendering/Public/Scene/Entity.h>
#include <Rendering/Public/Scene/SpatialDatabase.h>


#define DBG_LOG	(0)

namespace Rendering
{
namespace
{
union TaggedPointer
{
	const void *p;
	size_t		u;
};

static mxFORCEINLINE
TaggedPointer encodeTaggedPointer( const void* new_entity, ERenderEntity entity_type )
{
	TaggedPointer	result;
	result.p = new_entity;
	result.u |= entity_type;
	return result;
}

static mxFORCEINLINE
void decodeTaggedPointer( const TaggedPointer& tagged_pointer
						, const RenderEntityBase **entity_pointer_, ERenderEntity *entity_type_ )
{
	mxSTATIC_ASSERT(RE_MAX < 16);
	*entity_pointer_	= (RenderEntityBase*) (size_t(tagged_pointer.p) & ~0xF);	// zero out the 4 lowest bits
	*entity_type_		= ERenderEntity( tagged_pointer.u & 0xF );	// extract the 4 lowest bits
}

///
struct SpatialDatabase_Bruteforce: SpatialDatabaseI
{
	enum { MAX_OBJECTS = 8 * 1024 };

mxREMOVE_THIS
NwFloatingOrigin	_floating_origin;

	/// the total number of entities of the given type
	U32				_object_counts[RE_MAX];

	/// AABBs relative to the floating origin - these are used for frustum culling
	AABBf 			_relative_bounding_boxes[MAX_OBJECTS];

	///
	AABBd			_bounding_boxes_in_world_space[MAX_OBJECTS];

	///
	TaggedPointer	_object_pointers[MAX_OBJECTS];

	///
	bx::HandleAllocT<MAX_OBJECTS>	_handle_alloc;

	AllocatorI &			_storage;

	static const U16 BAD_INDEX = (U16)-1;

public:
	SpatialDatabase_Bruteforce( AllocatorI & storage )
		: _storage( storage )
	{
		mxZERO_OUT_ARRAY(_object_counts);
	}

	mxFORCEINLINE U16 getObjectIndexFromHandle( const U16 handle ) const
	{
		U16* dense  = _handle_alloc.getDensePtr();
		U16* sparse = _handle_alloc.getSparsePtr();

		U16 index = sparse[ handle ];
		return index;
	}

	bool isValidHandle( const HRenderEntity handle ) const
	{
		return _handle_alloc.IsValid( handle.id );
	}

	virtual bool ContainsEntity( const HRenderEntity entity_handle ) const override
	{
		return isValidHandle( entity_handle );
	}

	//
	virtual HRenderEntity AddEntity(
		const void* new_entity,
		ERenderEntity entity_type,
		const AABBd& aabb_in_world_space
		) override
	{
		mxASSERT2(IS_16_BYTE_ALIGNED(new_entity), "need space to pack entity type into lowest bits of pointer");
		mxASSERT2(_handle_alloc.getNumHandles() < MAX_OBJECTS, "max object count reached!");

		const U16 new_handle = _handle_alloc.alloc();

		const UINT new_object_index = getObjectIndexFromHandle( new_handle );

		_bounding_boxes_in_world_space[ new_object_index ] = aabb_in_world_space;
		_relative_bounding_boxes[ new_object_index ] = _floating_origin.getRelativeAABB( aabb_in_world_space );
		_object_pointers[ new_object_index ] = encodeTaggedPointer( new_entity, entity_type );


		++_object_counts[ entity_type ];

		//
		const HRenderEntity new_entity_handle ( new_handle );

#if DBG_LOG
		DEVOUT("\nADDED ENTITY(handle=%d) of type %d; curr count: %d (total: %d)",
			new_entity_handle, entity_type, _object_counts[ entity_type ], _handle_alloc.getNumHandles()
		);
#endif

		return new_entity_handle;
	}

	virtual void RemoveEntity(
		const HRenderEntity entity_handle
		) override
	{
		mxASSERT2(_handle_alloc.getNumHandles() > 0, "removing from empty set!");
		mxASSERT(isValidHandle(entity_handle));

		//
		U16* dense  = _handle_alloc.getDensePtr();
		U16* sparse = _handle_alloc.getSparsePtr();

		U16 index_of_object_being_removed = sparse[ entity_handle.id ];


		//
		const RenderEntityBase *entity_pointer;
		ERenderEntity entity_type;
		decodeTaggedPointer( _object_pointers[ index_of_object_being_removed ], &entity_pointer, &entity_type );

#if DBG_LOG
		DEVOUT("\nREMOVING ENTITY(handle=%d) of type %d; curr count: %d (total: %d)",
			entity_handle.id, entity_type, _object_counts[ entity_type ], _handle_alloc.getNumHandles()
		);
#endif

		--_object_counts[ entity_type ];


		//
		U16 temp = dense[ _handle_alloc.getNumHandles() - 1 ];
		const UINT index_of_last_created_object = sparse[ temp ];

		_relative_bounding_boxes[ index_of_object_being_removed ]		= _relative_bounding_boxes[ index_of_last_created_object ];
		_bounding_boxes_in_world_space[ index_of_object_being_removed ]	= _bounding_boxes_in_world_space[ index_of_last_created_object ];
		_object_pointers[ index_of_object_being_removed ]				= _object_pointers[ index_of_last_created_object ];


		//
		_handle_alloc.free( entity_handle.id );
	}

	virtual void UpdateEntityBounds(
		const HRenderEntity entity_handle,
		const AABBd& bounding_box_in_world_space
	) override
	{
		mxASSERT(isValidHandle(entity_handle));

		const UINT object_index = getObjectIndexFromHandle( entity_handle.id );

		_bounding_boxes_in_world_space[ object_index ] = bounding_box_in_world_space;

		_relative_bounding_boxes[ object_index ] = _floating_origin.getRelativeAABB(bounding_box_in_world_space);
	}

	//virtual void OnRegionChanged(
	//	const AABBf& region_bounds_in_world_space
	//	) override
	//{
	//}

	virtual ERet GetEntitiesIntersectingViewFrustum(
		RenderEntityList &visible_entities_
		, const ViewFrustum& view_frustum
	) const override
	{
		const UINT num_objects_to_test = _handle_alloc.getNumHandles();
		//const UINT num_objects_to_test = Calculate_Sum_1D( _object_counts );


		const AABBf* bounding_boxes_to_test = _relative_bounding_boxes;
		const TaggedPointer* object_pointers = _object_pointers;

		// reserve space for max number of objects
		mxDO(visible_entities_.ptrs.setNum( num_objects_to_test ));

		const RenderEntityBase** dst_buffer = visible_entities_.ptrs.raw();

		// Figure out write write_offsets for counting/bucket sort.
		U32		write_offsets[RE_MAX];	//!< offset of the first entity of the given type
		Build_Offset_Table_1D( _object_counts, write_offsets );

		U32		written_count[RE_MAX] = {0};	//!< the number of written entities of the given type

		//
		for( UINT i = 0; i < num_objects_to_test; i++ )
		{
			const AABBf& bounding_box = bounding_boxes_to_test[ i ];

			if( view_frustum.IntersectsAABB( bounding_box ) )
			{
				const TaggedPointer& tagged_pointer = object_pointers[ i ];

				const RenderEntityBase *entity_pointer;
				ERenderEntity entity_type;
				decodeTaggedPointer( tagged_pointer, &entity_pointer, &entity_type );

				const UINT dest_index = write_offsets[ entity_type ] + written_count[ entity_type ]++;
				dst_buffer[ dest_index ] = entity_pointer;
			}
		}

		TCopyStaticArray( visible_entities_.count, written_count );
		TCopyStaticArray( visible_entities_.offset, write_offsets );

		//DEVOUT("\n[REND] DUMPING STATS:\n");
		//int accum_count = 0;
		//for (int obj_type = 0; obj_type < RE_MAX; obj_type++ )
		//{
		//	if(visible_entities_.count[obj_type])
		//	{
		//		DEVOUT("obj_type=%d: %d objects at offset %d",
		//			obj_type, visible_entities_.count[obj_type], visible_entities_.offset[obj_type]
		//		);
		//		mxASSERT(
		//			visible_entities_.offset[obj_type] >= accum_count
		//		);
		//	}
		//
		//	accum_count += visible_entities_.count[obj_type];
		//}
	
		return ALL_OK;
	}

	///
	virtual ERet GetEntitiesIntersectingBox(
		RenderEntityList &visible_entities_
		, const AABBf& aabb
	) const override
	{
		const UINT num_objects_to_test = _handle_alloc.getNumHandles();
		const AABBf* bounding_boxes_to_test = _relative_bounding_boxes;
		const TaggedPointer* object_pointers = _object_pointers;

		// reserve space for max number of objects
		visible_entities_.ptrs.setNum( num_objects_to_test );
		const RenderEntityBase** dst_buffer = visible_entities_.ptrs.raw();

		// Figure out write write_offsets for counting/bucket sort.
		U32		write_offsets[RE_MAX];	//!< offset of the first entity of the given type
		Build_Offset_Table_1D( _object_counts, write_offsets );

		U32		written_count[RE_MAX] = {0};	//!< the number of written entities of the given type

		//
		for( UINT i = 0; i < num_objects_to_test; i++ )
		{
			const AABBf& bounding_box = bounding_boxes_to_test[ i ];

			if( aabb.intersects( bounding_box ) )
			{
				const TaggedPointer& tagged_pointer = object_pointers[ i ];

				const RenderEntityBase *entity_pointer;
				ERenderEntity entity_type;
				decodeTaggedPointer( tagged_pointer, &entity_pointer, &entity_type );

				const UINT dest_index = write_offsets[ entity_type ] + written_count[ entity_type ]++;
				dst_buffer[ dest_index ] = entity_pointer;
			}
		}

		TCopyStaticArray( visible_entities_.count, written_count );
		TCopyStaticArray( visible_entities_.offset, write_offsets );

		return ALL_OK;
	}

/*
	JobID findPotentiallyVisibleObjects(
		NwJobSchedulerI & task_scheduler
		)
	{
		/// Prepares the chunk's mesh in the background thread.
		/// Uploads the resulting mesh for rendering on the main thread.
		struct Task_FrustumCulling
		{
			/// each task writes to a local copy
			struct Outputs
			{
				DynamicArray< void* >	visible_object_pointers;
				U32						visible_object_counts[RE_MAX];	//!< the number of visible entities of the given type
			};

			//const ChunkID	_chunkId;	//!< id of the chunk to create; its LoD is always > 0
			//const ChunkInfo	_chunkInfo;
			//const SeamInfo	_seamInfo;		//!< for stitching
			//const bool		_modified;
			//OctreeWorld &	_world;
			////const U32		m_time;	//!< time (frame number) when this job was created
		public:
			Task_FrustumCulling(
				//const ChunkID& chunkId	//!< The ID of the chunk to remesh.
				//, const ChunkInfo& chunkInfo
				//, const SeamInfo& seamInfo
				//, const bool modified
				//, OctreeWorld & world
				)
				//: _chunkId( chunkId ), _chunkInfo( chunkInfo )
				//, _seamInfo( seamInfo ), _modified( modified )
				//, _world( world )
				////, m_time( world._currentFrameId )
			{
			}

			ERet Run( const NwThreadContext& context )
			{
				rmt_ScopedCPUSample(Task_FrustumCulling, RMTSF_None);
				PROFILE;

				AllocatorI &	local_scratchpad = *context.heap;	// thread-local allocator
				AllocatorI &	shared_scratchpad = VX::getJobAllocator();	// global thread-safe allocator

				RemeshResult* remeshResult;
				remeshResult = mxNEW( shared_scratchpad, RemeshResult, shared_scratchpad );
				mxENSURE( remeshResult, ERR_OUT_OF_MEMORY, "" );

				remeshResult->chunkId = _chunkId;
				remeshResult->chunkInfo = _chunkInfo;
				if( _chunkInfo.hasMesh() )
				{
					// mesh data is loaded in a background thread and passed to the main thread
					_world.prepareChunkMesh_AnyThread( _chunkId, _seamInfo, remeshResult->chunkMesh, local_scratchpad, _modified );
				}

				SpinWait::Lock	scopedLock( _world._remeshResultsCS );
				remeshResult->_next = _world._remeshResults;
				_world._remeshResults = remeshResult;

				return ALL_OK;
			}
		};//Task_FrustumCulling

		JobID task_id = task_scheduler.add(
			);

		return task_id;
	}
	*/

	virtual void GetStats(Stats &stats_) const override
	{
		stats_.total_object_count = _handle_alloc.getNumHandles();
	}

	virtual void DebugDraw( ADebugDraw & renderer
		, const NwView3D& main_camera_view
		, int flags /*= ~0*/
		) const override
	{
		tbPROFILE_FUNCTION;

		renderer.PushState();

		// enable depth testing
		NwRenderState * renderState = nil;
		Resources::Load(renderState, MakeAssetID("default"));
		if( renderState ) {
			renderer.SetStates( *renderState );
		}

		_DebugDrawBoundingBoxes( renderer );

		renderer.PopState();
	}

	void _DebugDrawBoundingBoxes( ADebugDraw & renderer ) const
	{
		for( UINT i = 0; i < _handle_alloc.getNumHandles(); i++ )
		{
			const AABBf& bounding_box = _relative_bounding_boxes[ i ];
			renderer.DrawAABB( bounding_box, RGBAf::GREEN );
		}
	}

};
}//namespace

SpatialDatabaseI* SpatialDatabaseI::CreateSimpleDatabase( AllocatorI & storage )
{
	return mxNEW( storage, SpatialDatabase_Bruteforce, storage );
}

void SpatialDatabaseI::Destroy( SpatialDatabaseI * o )
{
	SpatialDatabase_Bruteforce* culler = static_cast< SpatialDatabase_Bruteforce* >( o );
	mxDELETE( o, culler->_storage );
}

}//namespace Rendering

/*
References:

handle management / slot map / packed array:
http://bitsquid.blogspot.com/2011/09/managing-decoupling-part-4-id-lookup.html
https://gamedev.stackexchange.com/questions/33888/what-is-the-most-efficient-container-to-store-dynamic-game-objects-in
https://www.gamedev.net/articles/programming/general-and-gameplay-programming/game-engine-containers-handle_map-r4495/


Large Worlds & Floating-Point Precision:

http://scottbilas.com/files/2003/gdc_san_jose/continuous_world_paper.pdf

World Composition User Guide
System for managing large worlds including origin shifting technology.
https://docs.unrealengine.com/en-US/Engine/LevelStreaming/WorldBrowser/index.html

Shifting The Scene Origin
https://docs.nvidia.com/gameworks/content/gameworkslibrary/physx/guide/Manual/OriginShift.html

Unite 2013 - GIS Terrain & Unity
https://www.youtube.com/watch?v=VKWvAuTGVrQ



https://floatingorigin.com/

Harmony
Umbrella project for improving accuracy in applications and reducing positional variance.
https://gitlab.com/cosmochristo/harmony

Healing Cracks in Cyberspace: towards best practice [December 2019]
https://www.researchgate.net/publication/337759338_Healing_Cracks_in_Cyberspace_towards_best_practice
*/
