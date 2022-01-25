#ifndef INTERSECT_HLSL
#define INTERSECT_HLSL

#include <base.hlsli>

inline
float3 GetInverseRayDirectionSafe(in float3 raydir)
{
	// Fix ray directions that are too close to 0.
    const float ooeps = VERY_SMALL_NUMBER; // to avoid div by zero
	const float3 dir_abs = abs(raydir);

    float3 invdir;
    invdir.x = 1.0f / ((dir_abs.x > ooeps) ? raydir.x : CopySign(ooeps, raydir.x));
    invdir.y = 1.0f / ((dir_abs.y > ooeps) ? raydir.y : CopySign(ooeps, raydir.y));
    invdir.z = 1.0f / ((dir_abs.z > ooeps) ? raydir.z : CopySign(ooeps, raydir.z));
    return invdir;
}



struct PrecomputedRay
{
	float3	origin;

	float3	direction;
	float	max_t;	// max_ray_distance

	// precomputed values for faster intersection testing:

	float3	invdir;	// 1 / dir

	/// for faster ray-aabb intersection testing
	float3	neg_org_x_invdir;	// -(orig / dir)


	void SetFromRay(
		in float3 ray_origin
		, in float3 ray_direction
	)
	{
		origin = ray_origin;
		direction = ray_direction;
		max_t = 1e10;
		//
		invdir = GetInverseRayDirectionSafe(ray_direction);
		neg_org_x_invdir = -ray_origin * invdir;
	}
};



// Intersect rays vs bbox and return intersection span. 
// Intersection criteria is ret.x <= ret.y
inline
float2 IntersectRayAgainstAABB(
	in PrecomputedRay precomp_ray
	, in float3 aabb_min
	, in float3 aabb_max
	)
{
    // float3 n = (aabb_min - ray_origin) / ray_direction;
    // float3 f = (aabb_max - ray_origin) / ray_direction;
	const float3 n = MAD(aabb_min, precomp_ray.invdir, precomp_ray.neg_org_x_invdir);
    const float3 f = MAD(aabb_max, precomp_ray.invdir, precomp_ray.neg_org_x_invdir);
    
	const float3 tmin = min(f, n);
    const float3 tmax = max(f, n);
    
	const float t0 = max(max3(tmin), 0.f);	// don't allow intersections 'behind' the ray
    const float t1 = min(min3(tmax), precomp_ray.max_t);
	
    return float2(t0, t1);
}

///
#define RAY_DID_HIT_AABB(hit_times)	(hit_times.x <= hit_times.y)



/*
==========================================================
	RAY-SPHERE INTERSECTION
==========================================================
*/
bool IntersectRaySphere(
						float3 rO, float3 rD,
						float3 sO, float sR,
						out float tnear, out float tfar
						)
{
    float3 delta = rO - sO;
    
    float A = dot( rD, rD );
    float B = 2*dot( delta, rD );
    float C = dot( delta, delta ) - sR*sR;
    
    float disc = B*B - 4.0*A*C;
	
	const float DIST_BIAS = 0.001f;
    if( disc < DIST_BIAS )
    {
		// error X4000: Use of potentially uninitialized variable
		tnear = 0;
		tfar = 0;
        return false;
    }
    else
    {
        float sqrtDisc = sqrt( disc );
        tnear = (-B - sqrtDisc ) / (2*A);
        tfar = (-B + sqrtDisc ) / (2*A);
        return true;
    }
}

#endif // INTERSECT_HLSL
