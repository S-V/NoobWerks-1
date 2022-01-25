/*
=============================================================================
	File:	SpatialDatabase.h
	Desc:	Spatial structures for accelerating spatial queries on the scene
			(e.g. hierarchical view frustum culling).
=============================================================================
*/
#pragma once

#include <Rendering/BuildConfig.h>
#include <Rendering/ForwardDecls.h>
#include <Rendering/Public/Scene/CameraView.h>
#include <Rendering/Public/Scene/Entity.h>


namespace Rendering {

///
struct RenderEntityList
{
	/// the number of visible entities of the given type
	U32		count[RE_MAX];

	/// offset of the first visible entity of the given type in the array
	U32		offset[RE_MAX];

	/// array of pointers to visible objects;
	/// objects of the same type are stored contiguously
	DynamicArray< const RenderEntityBase* >	ptrs;

public:
	RenderEntityList( AllocatorI & storage )
		: ptrs( storage )
	{
		Clear();
	}

	void Clear()
	{
		mxZERO_OUT_ARRAY(count);
		mxZERO_OUT_ARRAY(offset);
		ptrs.RemoveAll();
	}

	mxFORCEINLINE const RenderEntityBase* getObjectAt(
		const ERenderEntity entity_type
		, const UINT index
		) const
	{
		return ptrs[ offset[entity_type] + index ];
	}

	template< typename RenderEntityType >	// where RenderEntityType: RenderEntityBase
	TSpan< const RenderEntityType* > getObjectsOfType(
		const ERenderEntity entity_type
		) const
	{
		const U32 number_of_objects = count[ entity_type ];
		const RenderEntityType** array_of_pointers =
			(const RenderEntityType**)( ptrs.raw() + offset[ entity_type ] )
			;
		return TSpan< const RenderEntityType* >( array_of_pointers, number_of_objects );
	}
};

///
struct SpatialDatabaseI: NonCopyable
{
public:
	virtual HRenderEntity AddEntity(
		const void* new_entity,
		ERenderEntity entity_type,
		const AABBd& aabb_in_world_space
		) = 0;

	virtual void RemoveEntity( const HRenderEntity entity_handle ) = 0;

	virtual bool ContainsEntity( const HRenderEntity entity_handle ) const = 0;

	virtual void UpdateEntityBounds(
		const HRenderEntity entity_handle,
		const AABBd& bounding_box_in_world_space
	) = 0;

	//virtual void OnRegionChanged( const AABBf& region_bounds_in_world_space ) = 0;

public:	// Query

	virtual ERet GetEntitiesIntersectingViewFrustum(
		RenderEntityList &visible_entities_
		, const ViewFrustum& view_frustum
	) const = 0;

	virtual ERet GetEntitiesIntersectingBox(
		RenderEntityList &visible_entities_
		, const AABBf& aabb
	) const = 0;


	struct Stats
	{
		U32	total_object_count;
	};
	virtual void GetStats(Stats &stats_) const = 0;

public:	// Debugging

	struct DebugDrawFlags
	{
		enum Enum
		{
			DrawBoundingBoxes = BIT(0),
		};
	};

	/// Draws bounding boxes of all models.
	virtual void DebugDraw( ADebugDraw & renderer
		, const NwView3D& main_camera_view
		, int flags = ~0
		) const
	{}

public:	// Convenience.


public:
	static SpatialDatabaseI* CreateSimpleDatabase(
		AllocatorI & storage
		//, const U32 max_object_count
		);
	static void Destroy( SpatialDatabaseI* spatial_database );

//protected:
	virtual ~SpatialDatabaseI() {}
};

}//namespace Rendering
