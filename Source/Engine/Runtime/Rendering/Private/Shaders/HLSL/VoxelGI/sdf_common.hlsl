// Tracing SDF bricks
#ifndef DISTANCE_FIELD_COMMON_HLSL
#define DISTANCE_FIELD_COMMON_HLSL

#include <RayTracing/ray_intersection.hlsl>
#include <Common/constants.hlsli>


static const int MAX_SDF_TRACE_STEPS = 128;

/// Each brick is surrounded by 1 voxel thick border
/// so that we can use hardware trilinear filtering
/// for sampling the brick's interior voxels.
static const int SDF_BRICK_MARGIN_VOXELS = 1;	// aka 'apron' in GVDB


#endif // DISTANCE_FIELD_COMMON_HLSL
