// utility functions for transforming vectors between different spaces
#ifndef __TRANSFORM_H__
#define __TRANSFORM_H__

#include <Shared/nw_shared_globals.h>

/// Transforms the position from local to homogeneous projection space.
inline float4 Pos_Local_To_Clip( in float4 local_position )
{
	return mul( g_world_view_projection_matrix, local_position );
}

///
inline float4 Transform_Position_World_To_Clip( in float4 world_position )
{
	return mul( g_view_projection_matrix, world_position );
}

/// Transforms the position from local to view space.
inline float4 Pos_Local_To_View( in float4 local_position )
{
	return mul( g_world_view_matrix, local_position );
}
/// Transforms the position from local to world space.
inline float4 Pos_Local_To_World( in float4 local_position )
{
	return mul( g_world_matrix, local_position );
}
/// Transforms the position from world to view space.
inline float4 Pos_World_To_View( float4 worldSpacePosition )
{
	return mul( g_view_matrix, worldSpacePosition );
}
/// Transforms the position from view to world space.
inline float4 Pos_View_To_World( float4 position_in_view_space )
{
	return mul( g_inverse_view_matrix, position_in_view_space );
}
/// Transforms the position from view to world space.
inline float4 Pos_View_To_Clip( float3 position_in_view_space )
{
	return mul( g_projection_matrix, float4( position_in_view_space, 1 ) );
}
///// Transforms the position from clip to world space.
//inline float4 Pos_Clip_To_World( float4 clipSpacePosition )
//{
//	return mul( g_inverse_view_projection_matrix, clipSpacePosition );
//}

/// Transforms the direction from object to world space.
inline float3 Dir_Local_To_World( in float3 objectSpaceDirection )
{
	return normalize(mul( (float3x3)g_world_matrix, objectSpaceDirection ));
}
/// Transforms the direction from object to view space.
inline float3 Dir_Local_To_View( float3 objectSpaceDirection )
{
	return normalize(mul( (float3x3)g_world_view_matrix, objectSpaceDirection ));
}
/// Transforms the direction from world to view space.
inline float3 Dir_World_To_View( in float3 worldSpaceDirection )
{
	return normalize(mul( (float3x3)g_view_matrix, worldSpaceDirection ));
}
/// Transforms the direction from view to world space.
inline float3 Dir_View_To_World( in float3 viewSpaceDirection )
{
	//return normalize(mul( (float3x3)g_inverseTransposeViewMatrix, viewSpaceDirection ));
	return normalize(mul( (float3x3)g_inverse_view_matrix, viewSpaceDirection ));
}

#endif // __TRANSFORM_H__
