/*
=============================================================================

=============================================================================
*/
#pragma once


namespace Cubemap_Util
{

extern const float s_faceUvVectors[6][3][3];

/// _u and _v should be center addressing and in [-1.0+invSize..1.0-invSize] range.
static inline V3f texelCoordToVec( float _u, float _v, int _faceId )
{
	V3f	direction;

    // out = u * s_faceUv[0] + v * s_faceUv[1] + s_faceUv[2].
    direction.x = s_faceUvVectors[_faceId][0][0] * _u + s_faceUvVectors[_faceId][1][0] * _v + s_faceUvVectors[_faceId][2][0];
    direction.y = s_faceUvVectors[_faceId][0][1] * _u + s_faceUvVectors[_faceId][1][1] * _v + s_faceUvVectors[_faceId][2][1];
    direction.z = s_faceUvVectors[_faceId][0][2] * _u + s_faceUvVectors[_faceId][1][2] * _v + s_faceUvVectors[_faceId][2][2];

	direction.normalizeNonZeroLength();

	return direction;
}

}//namespace Cubemap_Util
