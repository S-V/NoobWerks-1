//
#include "stdafx.h"
#pragma hdrstop

#include "voxels/voxels.h"

#include <Rendering/Private/Modules/VoxelGI/vxgi_brickmap.h>


const VX5::ProcGen::UserDataSourceFloatT& MyVoxels::VX_GetDataSource_Float() const
{
	return *this;
}

void MyVoxels::VX_GetBoxContentsInfo(
								  VX5::ProcGen::BoxContentsInfo &box_contents_
								  , const CubeMLd& box_in_world_space
								  , const VX5::ProcGen::ChunkContext& context
								  ) const
{
#if 0|| GAME_CFG_RELEASE_BUILD
	// for faster generation
	box_contents_.is_homogeneous = true;
	box_contents_.homogeneous_material_id = VX5::EMPTY_SPACE;
#endif
}

VX5::ProcGen::ProvidedChunkData MyVoxels::VX_StartGeneratingChunk(
	VX5::ProcGen::ChunkConstants &chunk_constants_
	, const VX5::ProcGen::ChunkContext& context
	)
{
	//DEVOUT("VX_StartGeneratingChunk: %s",
	//	VX5::ToMyChunkID(context.chunk_id).ToString().c_str()
	//	);
	return VX5::ProcGen::ProvidedChunkData();
}



/*
// Box: correct distance to corners
float fBox(
		   vec3 p, vec3 b
		   )
{
vec3 d = abs(p) - b;
return length(
max(d, vec3(0))) + vmax(min(d, vec3(0))
);
}
*/

/*
float AABB_SDF(
			   const AABBd& aabb
			   , const V3d& p
			   )
{
	const V3d rel_pos = p - aabb.center();
	const V3d d = abs(rel_pos) - aabb.halfSize();
	return (
		V3d::min(d, CV3d(0))) + CV3d( (V3d::min(d, CV3d(0)).maxComponent() )
		).length();
}
*/

#if 0
/// BUG: this is a thin shell box (of zero thickness)!
double DistanceToBoxShell(
				const AabbCEd& aabb
				, const V3d& p
				)
{
	const V3d rel_pos = p - aabb.center;
	const V3d d = V3d::abs(rel_pos) - aabb.extent;
	const V3d v_zero = CV3d(0);
	return (
		V3d::max(d, v_zero) + CV3d( V3d::min(d, v_zero).maxComponent() )
	).length();
}
#endif

/// Box: correct distance to corners
double DistanceToBox_Correct(
				const AabbCEd& aabb
				, const V3d& p
				)
{
	const V3d rel_pos = p - aabb.center;
	const V3d d = V3d::abs(rel_pos) - aabb.extent;
	const V3d v_zero = CV3d(0);
	return V3d::max(d, v_zero).length()
		+ V3d::min(d, v_zero).maxComponent()
		;
}

double DistanceToBox( const AabbCEd& aabb
					 , const V3d& _position
					 )
{
	// position relative to the center of the cube
	const V3d relativePosition = _position - aabb.center;
	// Measure how far each coordinate is from the center of the cube along that coordinate axis.
	const V3d offsetFromCenter = V3d::abs( relativePosition );	// abs() accounts for symmetry/congruence
	// Outside the cube, each point has at least one coordinate of distance greater than 'half-size',
	// points on the edges have at least one coordinate at distance 'half-size',
	// and points on the interior have all coordinates within 'half-size' of the center.
	// NOTE: distances inside the object are negative, so the maximum of these distances yields the correct result.
	// The Box Approximation: the returned result may be less than the correct Euclidean distance.
	mxBIBREF("Interactive Modeling with Distance Fields, Tim-Christopher Reiner [2010], 3.4.4 Box Approximation, P.20");
	// This SDF returns a Chebyshev distance, which can be greater than the Euclidean distance: max( |x|-W/2, |y|-D/2, |z|-H/2 ).
	mxBIBREF("Interactive Modeling of Implicit Surfaces using a Direct Visualization Approach with Signed Distance Functions [2011], 3.3. Describing Primitives, P.3");
	return Max3(
		offsetFromCenter.x - aabb.extent.x,
		offsetFromCenter.y - aabb.extent.y,
		offsetFromCenter.z - aabb.extent.z
		);
}

//double MyVoxels::VX_GetSignedDistanceAtPoint(
//	const V3d& position_in_world_space
//	, const VX5::ProcGen::ChunkContext& context
//	, const VX5::ProcGen::ChunkConstants& scratchpad
//	)
//{
//	//UNDONE;
//	//const double box_sdf = DistanceToBoxShell(_test_box, position_in_world_space);//-
//	//const double box_sdf = DistanceToBox(_test_box, position_in_world_space);
//	const double box_sdf = DistanceToBox_Correct(_test_box, position_in_world_space);
//
//	const double plane_sdf = _test_plane.distanceToPoint(position_in_world_space);//-
//	const double sphere_sdf = _test_sphere.signedDistanceToPoint(position_in_world_space);
//	return Min3(box_sdf, plane_sdf, sphere_sdf);//union
//}

float MyVoxels::VX_GetSignedDistanceAtPoint(
	const Tuple3<float>& position_in_world_space
	, const VX5::ProcGen::ChunkContext& chunk_context
	, const VX5::ProcGen::ChunkConstants& chunk_constants
	) const
{
//return position_in_world_space.y - position_in_world_space.x * 0.15f;

	const V3d position_in_world_space_d = V3d::fromXYZ(position_in_world_space);

	const double box_sdf = DistanceToBox_Correct(_test_box, position_in_world_space_d);

	const double plane_sdf = _test_plane.distanceToPoint(position_in_world_space_d);//-
	const double sphere_sdf = _test_sphere.signedDistanceToPoint(position_in_world_space_d);

return plane_sdf;

	return Min3(box_sdf, plane_sdf, sphere_sdf);//union
}

//float01_t MyVoxels::GetStrength(const V3f& pos)
//{
//	// good values for voxel_size = 0.25
//	nwTWEAKABLE_CONST(float, CRATER_NOISE_SCALE, 2);
//	nwTWEAKABLE_CONST(float, CRATER_NOISE_OCTAVES, 1);
//
//	return _test_terrain_noise.evaluate2D(
//		pos.x * CRATER_NOISE_SCALE, pos.y * CRATER_NOISE_SCALE,
//		_perlin_noise,
//		CRATER_NOISE_OCTAVES
//		);
//}

const VX5::RequestedChunkData MyVoxels::GetRequestedChunkData_AnyThread(
	const VX5::ChunkID& chunk_id
	, VX_WorldUserData* user_world_data
	) const
{
	VX5::RequestedChunkData	requested_chunk_data;

	//requested_chunk_data.sdf.resolution_log2 = 3;


#ifdef VXGI_DISTANCE_FIELD_FORMAT_USED
	#if VXGI_DISTANCE_FIELD_FORMAT_USED == VXGI_DISTANCE_FIELD_FORMAT_FLOAT
		requested_chunk_data.sdf.format = VX5::Distance_Float32;
	#elif VXGI_DISTANCE_FIELD_FORMAT_USED == VXGI_DISTANCE_FIELD_FORMAT_UNORM16
		requested_chunk_data.sdf.format = VX5::Distance_UNorm16;
	#elif VXGI_DISTANCE_FIELD_FORMAT_USED == VXGI_DISTANCE_FIELD_FORMAT_UNORM8
		requested_chunk_data.sdf.format = VX5::Distance_UNorm8;
	#else
	#	error Unknown SDF format!
	#endif
#else
	#	error VXGI_DISTANCE_FIELD_FORMAT_USED is undefined!
#endif

	return requested_chunk_data;
}
