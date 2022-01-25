/*
=============================================================================
	File:	Quaternion.cpp
	Desc:	Quaternion.
=============================================================================
*/

#include <Base/Base_PCH.h>
#pragma hdrstop
#include <Base/Base.h>
#include <Base/Math/Quaternion.h>



#if MM_ENABLE_REFLECTION

	mxBEGIN_STRUCT(Q4f)
		mxMEMBER_FIELD(x),
		mxMEMBER_FIELD(y),
		mxMEMBER_FIELD(z),
		mxMEMBER_FIELD(w),
	mxEND_REFLECTION

#endif



ATextStream & operator << ( ATextStream & log, const NwQuaternion& q )
{
	log.PrintF("(%.3f, %.3f, %.3f, %.3f)",q.x,q.y,q.z,q.w);
	return log;
}

String128 NwQuaternion::humanReadableString() const
{
	const NwAxisAngle	axis_angle = this->toAxisAngle();
	//
	String128	result;
	axis_angle.toString( &result );
	return result;
}

#if 0
		//  this is adapted from Computer Graphics (Hearn and Bker 2nd ed.) p. 420

		const float a = q.x;
		const float b = q.y;
		const float c = q.z;
		const float s = q.w;

		// 1st ROW vector
		mat.r0.x = 1.0f - 2.0f*b*b - 2.0f*c*c;
		mat.r0.y = 2.0f*a*b + 2.0f*s*c;
		mat.r0.z = 2.0f*a*c - 2.0f*s*b;

		// 2nd ROW vector
		mat.r1.x = 2.0f*a*b - 2.0f*s*c;
		mat.r1.y = 1.0f - 2.0f*a*a - 2.0f*c*c;
		mat.r1.z = 2.0f*b*c + 2.0f*s*a;

		// 3rd ROW vector
		mat.r2.x = 2.0f*a*c + 2.0f*s*b;
		mat.r2.y = 2.0f*b*c - 2.0f*s*a;
		mat.r2.z = 1.0f - 2.0f*a*a - 2.0f*b*b;
#endif


void QuaternionSlerp( const float* p, float* q, float t, float *qt )
{
	int i;
	float omega, cosom, sinom, sclp, sclq;

	// decide if one of the quaternions is backwards
	float a = 0;
	float b = 0;
	for (i = 0; i < 4; i++) {
		a += (p[i]-q[i])*(p[i]-q[i]);
		b += (p[i]+q[i])*(p[i]+q[i]);
	}
	if (a > b) {
		for (i = 0; i < 4; i++) {
			q[i] = -q[i];
		}
	}

	cosom = p[0]*q[0] + p[1]*q[1] + p[2]*q[2] + p[3]*q[3];

	if ((1.0 + cosom) > 0.00000001) {
		if ((1.0 - cosom) > 0.00000001) {
			omega = acos( cosom );
			sinom = sin( omega );
			sclp = sin( (1.0 - t)*omega) / sinom;
			sclq = sin( t*omega ) / sinom;
		}
		else {
			sclp = 1.0 - t;
			sclq = t;
		}
		for (i = 0; i < 4; i++) {
			qt[i] = sclp * p[i] + sclq * q[i];
		}
	}
	else {
		qt[0] = -p[1];
		qt[1] = p[0];
		qt[2] = -p[3];
		qt[3] = p[2];
		sclp = sin( (1.0 - t) * 0.5 * mxPI);
		sclq = sin( t * 0.5 * mxPI);
		for (i = 0; i < 3; i++) {
			qt[i] = sclp * p[i] + sclq * qt[i];
		}
	}
}


#if MX_DEVELOPER

void UnitTest_Quaternions()
{
	const float	comparison_epsilon = 1e-4f;
	//
	const V3f		point_on_X_axis = { 1, 0, 0 };

	//
	const Q4f	rotate_by_90_deg_around_Z = Q4f::FromAxisAngle(
		CV3f(0,0,1), DEG2RAD(90)
		);

	const Q4f	rotate_by_90_deg_around_Y = Q4f::FromAxisAngle(
		CV3f(0,1,0), DEG2RAD(90)
		);

	const Q4f	rotate_by_90_deg_around_X = Q4f::FromAxisAngle(
		CV3f(1,0,0), DEG2RAD(90)
		);

	//const Q4f	rotate_by_90_deg_around_Z_and_then_by_90_around_X = Q4f::FromAxisAngle(
	//	CV3f(1,0,0), DEG2RAD(90)
	//	);

	//
	const V3f		rotated_point1 = rotate_by_90_deg_around_Z.rotateVector( point_on_X_axis );
	mxASSERT(rotated_point1.equals(CV3f(0,1,0), comparison_epsilon));
	const V3f		rotated_point1_mat = rotate_by_90_deg_around_Z.toMat3()
		.transformVector(point_on_X_axis);
	mxASSERT(rotated_point1_mat.equals(CV3f(0,1,0), comparison_epsilon));

	//
	const Q4f	combined_rotation = 
		rotate_by_90_deg_around_X <<= rotate_by_90_deg_around_Y <<= rotate_by_90_deg_around_X
		;

	const V3f		rotated_point2 = combined_rotation.rotateVector( point_on_X_axis );
	mxASSERT(rotated_point2.equals(CV3f(0,1,0), comparison_epsilon));
	const V3f		rotated_point2_mat = combined_rotation.toMat3()
		.transformVector(point_on_X_axis);
	mxASSERT(rotated_point2_mat.equals(CV3f(0,1,0), comparison_epsilon));
}

#endif // MX_DEVELOPER

/*
https://en.wikipedia.org/wiki/Quaternions_and_spatial_rotation

Good APIs:
https://docs.unity3d.com/ScriptReference/Quaternion.html
*/
