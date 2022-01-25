// Skeletal animation system
#pragma once

//
#include <Base/Template/Containers/Dictionary/TSortedMap.h>
//
#include <Core/Assets/AssetReference.h>	// TResPtr<>
#include <Core/Tasking/TaskSchedulerInterface.h>
//
#include <Rendering/ForwardDecls.h>
#include <Rendering/Public/Scene/MeshInstance.h>


namespace Rendering
{
class NwDecalsSystem: public TGlobal< NwDecalsSystem >
{
	enum
	{
		/// e.g. plasma scorch marks
		MAX_DECALS = 1024,
	};

	//
	struct ScorchMark
	{
		V3f			center;
		float		radius;	// light flash radius in world space
		V3f			normal;
		float01_t	life;	// 0..1, dead if > 1
	};
	ASSERT_SIZEOF(ScorchMark, 32);

	TFixedArray< ScorchMark, MAX_DECALS >	scorch_marks;


public:
	NwDecalsSystem();
	~NwDecalsSystem();

	ERet Initialize();
	void Shutdown();

	void AdvanceDecalsOncePerFrame(
		const NwTime& game_time
		);

	ERet RenderDecalsIfAny(
		SpatialDatabaseI* spatial_database
		);

public:
	void AddScorchMark(
		const V3f& pos_world
		, const V3f& normal
		);
};


/// Point -> OBB expansion in Geometry Shader
struct RrDecalVertex: CStruct
{
	// size - radius in world space
	V4f		position_and_size;

	// color - used by shader
	// life: [0..1] is used for alpha
	V4f		color_and_life;

	V4f		decal_axis_x;
	V4f		decal_normal;

public:
	tbDECLARE_VERTEX_FORMAT(RrDecalVertex);
	mxDECLARE_REFLECTION;
	static void buildVertexDescription( NwVertexDescription *description_ );
};

}//namespace Rendering
