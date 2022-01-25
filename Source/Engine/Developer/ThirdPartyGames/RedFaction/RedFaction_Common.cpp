#include <Base/Base.h>
#include <Developer/ThirdPartyGames/RedFaction/RedFaction_Common.h>


#define CONVERT_TO_MY_COORD_SYSTEM	(1)


namespace RF1
{

	// RF: Left-Handed:  Y - UP, Z - FORWARD, X - RIGHT.
	// My: Right-Handed: Z - UP, Y - FORWARD, X - RIGHT.

	const V3f	toMyVec3( const V3f& rf_vec )
	{

#if CONVERT_TO_MY_COORD_SYSTEM

		// map axis Y to Z and axis Z to Y
		return CV3f( rf_vec.x, rf_vec.z, rf_vec.y );

#else

		return rf_vec;

#endif

	}

	const Q4f	toMyQuat( const Q4f& rf_quat )
	{

#if CONVERT_TO_MY_COORD_SYSTEM

		// Swap the Y and Z axes *and* reverse the angle, because handedness changed.
		// We've changed the handedness of our coordinate system,
		// our angle takes the opposite sign (a +ve rotation in a right-handed coordinate system
		// is a -ve rotation about the corresponding axis in a left-handed coordinate system).
		// This leaves the real part (w) unchanged, since cos(theta) = cos(-theta),
		// and negates the imaginary part, since sin(theta) = -sin(-theta).
		// https://gamedev.stackexchange.com/a/141249/15235

		const Q4f	swizzled_q = Q4f::create(
			-rf_quat.x, -rf_quat.z, -rf_quat.y
			, rf_quat.w
			);
#if 0
		//
		const NwAxisAngle	orig_axis_angle = rf_quat.toAxisAngle();

		const NwAxisAngle	new_axis_angle = {
			toMyVec3( V3_Normalized( orig_axis_angle.axis ) ),
			-orig_axis_angle.angle
		};

		const Q4f	tmp_q = Q4f::FromAxisAngle( new_axis_angle );
		mxASSERT( tmp_q.equals( swizzled_q, 1e-5f ) );
#endif
		return swizzled_q;

#else

		return rf_quat;

#endif

	}

	const Q4f removeFlip( const Q4f& q )
	{
		if( q.lengthSquared() < 0 ){
			return q.negated();
		}
		return q;
	}

	const M44f buildMat4(
		const Q4f& q,
		const V3f& t
		)
	{
		const M33f	rotation_matrix = q.toMat3();

		M44f	result;
		result.r0 = Vector4_Set( rotation_matrix.r0, 0.0f );
		result.r1 = Vector4_Set( rotation_matrix.r1, 0.0f );
		result.r2 = Vector4_Set( rotation_matrix.r2, 0.0f );
		result.r3 = Vector4_Set( t, 1.0f );

		return result;
	}

	static const M33f extractRotationMatrixPart( const M44f& mat4x4 )
	{
		M33f	mat;
		mat.r0 = V4_As_V3( mat4x4.v0 );
		mat.r1 = V4_As_V3( mat4x4.v1 );
		mat.r2 = V4_As_V3( mat4x4.v2 );
		return mat;
	}

	V3f getRotatedPointFromBoneMat( const M44f& obj_space_bone_mat )
	{
		const V3f t = M44_getTranslation( obj_space_bone_mat );
		const M33f	rot_mat = extractRotationMatrixPart( obj_space_bone_mat );
		return M33_Transform( rot_mat, t );
	}

}//namespace RF1
