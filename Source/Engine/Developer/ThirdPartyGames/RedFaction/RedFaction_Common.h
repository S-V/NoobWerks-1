#pragma once

#include <Base/Math/Quaternion.h>


namespace RF1
{
	// RF: Left-Handed:  Y - UP, Z - FORWARD, X - RIGHT.
	// My: Right-Handed: Z - UP, Y - FORWARD, X - RIGHT.

	const V3f	toMyVec3( const V3f& rf_vec );

	const Q4f	toMyQuat( const Q4f& rf_quat );

	const Q4f	removeFlip( const Q4f& q );

	const M33f quat_to_mat3x3(
		const Q4f& q
		);

	const M44f buildMat4(
		const Q4f& q,
		const V3f& t
		);

	V3f getRotatedPointFromBoneMat( const M44f& obj_space_bone_mat );

enum
{
	MAX_LOD_DISTANCES = 3,
	MAX_SUBMESH_MATERIALS = 32,
	MAX_SUBMESH_BATCHES = 8,

	MAX_SUBMESH_VERTICES = 1024,
	MAX_SUBMESH_TRIANGLES = 1024,

	MAX_UNKNOWN_DATA_SIZE = 2048,

	/// According to RF Bone
	MAX_BONES = 50,

	/// bone attachments
	MAX_SOCKETS = 32,
};

}//namespace RF1
