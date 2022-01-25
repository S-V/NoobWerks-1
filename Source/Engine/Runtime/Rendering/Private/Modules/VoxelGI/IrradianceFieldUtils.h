#pragma once


/// Returns 3D unit vectors uniformly distributed on the sphere.
const V3f GetRandomUnitVector(NwRandom & rng)
{
	V3f		random_vec;
	float	mag_squared;

	// Rejection sample
	do {
		random_vec.x = rng.GetRandomFloatMinus1Plus1();
		random_vec.y = rng.GetRandomFloatMinus1Plus1();
		random_vec.z = rng.GetRandomFloatMinus1Plus1();
		mag_squared = random_vec.lengthSquared();
	} while( mag_squared >= 1.0f );

	// Divide by magnitude to produce a unit vector
	random_vec *= mmInvSqrt(mag_squared);

	return random_vec;
}

const Q4f GetRandomQuaternion(NwRandom & rng)
{
	const V3f random_axis_vec = GetRandomUnitVector(rng);
	const float random_rotation_angle = rng.GetRandomFloatInRange(0, mxTWO_PI);
	return Q4f::FromAxisAngle(random_axis_vec, random_rotation_angle);
}

// Generate a spherical Fibonacci point.
// The points go from z = +1 down to z = -1 in a spiral.
// To generate samples on the +z hemisphere, just stop before sample_index > N/2.
// http://extremelearning.com.au/how-to-evenly-distribute-points-on-a-sphere-more-effectively-than-the-canonical-fibonacci-lattice/
//
const V3f SphericalFibonacci(
	const int sample_index
	, const float inverse_samples_count	// 1.f / samples_count
	)
{
	const float product = (float)sample_index * mxPHI_MINUS_1;
	const float frac_part = product - floorf(product);
    const float phi = mxTWO_PI * frac_part;

	const float cos_theta = 1.0f - (float)(2 * sample_index + 1) * inverse_samples_count;
    const float sin_theta = sqrtf(Saturate(1.0f - cos_theta * cos_theta));

    return CV3f(
		cos(phi) * sin_theta,
		sin(phi) * sin_theta,
		cos_theta
	);
}

const V3f RotateVectorByQuaternion( const V3f v, const Q4f& q )
{
	const V3f& b = q.v.asVec3();
	const float b2 = V3f::dot(b, b);
	return v * (q.w * q.w - b2)
		+ b * (V3f::dot(v, b) * 2.f)
		+ V3f::cross(b, v) * (q.w * 2.f)
		;
}
