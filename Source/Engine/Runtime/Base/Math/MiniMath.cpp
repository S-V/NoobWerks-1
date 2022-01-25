/*
=============================================================================
Useful references:

How To Write A Maths Library In 2016 February 19th, 2016 
http://www.codersnotes.com/notes/maths-lib-2016/

Practical Cross Platform SIMD Math
http://www.gamedev.net/page/resources/_/technical/general-programming/practical-cross-platform-simd-math-r3068

SIMDíifying multi-platform math
http://molecularmusings.wordpress.com/2011/10/18/simdifying-multi-platform-math/

http://seanmiddleditch.com/journal/2012/08/matrices-handedness-pre-and-post-multiplication-row-vs-column-major-and-notations/

Rotation About an Arbitrary Axis in 3 Dimensions, Glenn Murray, June 6, 2013
http://inside.mines.edu/~gmurray/ArbitraryAxisRotation/

avoiding trigonometry
http://www.iquilezles.org/www/articles/noacos/noacos.htm

Quaternions: How
http://physicsforgames.blogspot.ru/2010/02/quaternions.html
Quaternions: Why
http://physicsforgames.blogspot.ru/2010/02/quaternions-why.html

Notes on Quaternions:
http://www.lce.hut.fi/~ssarkka/pub/quat.pdf

Useful math derivations:
http://www.euclideanspace.com/maths/

Perspective projections in LH and RH systems:
http://www.gamedev.net/page/resources/_/technical/graphics-programming-and-theory/perspective-projections-in-lh-and-rh-systems-r3598

—ËÒÚÂÏ‡  ÓÓ‰ËÌ‡Ú LHvsRH Yup vs Zup:
http://www.gamedev.ru/code/forum/?id=185904

@todo:
	The behaviour of Vector4_Normalize() is incorrect.
=============================================================================
*/
#include <Base/Base_PCH.h>
#pragma hdrstop
#include <Base/Base.h>

//#include <xnamath.h>

#include "MiniMath.h"



void Vector4_SinCos( Vec4Arg0 v, Vector4 *s, Vector4 *c )
{
	const V4f& rv = (const V4f&) v;
	V4f* ps = (V4f*) s;
	V4f* pc = (V4f*) c;
	mmSinCos( rv.x, ps->x, pc->x );
	mmSinCos( rv.y, ps->y, pc->y );
	mmSinCos( rv.z, ps->z, pc->z );
	mmSinCos( rv.w, ps->w, pc->w );
}

//const Vector4 Quaternion_Inverse( Vec4Arg0 q )
//{
//	return Vector4_Multiply( Quaternion_Conjugate( q ), Vector4_ReciprocalLengthV( q ) );
//}
/// Spherical linear interpolation between two unit (normalized) quaternions (aka SLERP).
/// NOTE:
/// Interpolates along the shortest path between orientations.
/// Does not clamp t between 0 and 1.
const Vector4 Quaternion_Slerp( Vec4Arg0 q0, Vec4Arg0 q1, float t )
{
    Vector4 start;
	// Compute "cosine of angle between quaternions" using dot product
	float cosTheta = Vector4_Get_X( Vector4_Dot( q0, q1 ) );
	// If cosTheta < 0, the interpolation will take the long way around the sphere. 
	// To fix this, one quat must be negated.
    if( cosTheta < 0.0f ) {
		// Two quaternions q and -q represent the same rotation, but may produce different slerp.
		// We chose q or -q to rotate using the acute angle.
		cosTheta = -cosTheta;
        start = Vector4_Negate( q0 );
    } else {
        start = q0;
    }
	// Interpolation = A*[sin((1-Elapsed) * Angle)/sin(Angle)] + B * [sin (Elapsed * Angle) / sin(Angle)]
	float scale0, scale1;
    if( cosTheta < mxSLERP_DELTA ) {
		// Essential Mathematics, page 467
        float angle = acosf( cosTheta );
		// Compute inverse of denominator, so we only have to divide once
		float recipSinAngle = ( 1.0f / sinf( angle ) );
		// Compute interpolation parameters
        scale0 = sinf( ( ( 1.0f - t ) * angle ) ) * recipSinAngle;
        scale1 = sinf( ( t * angle ) ) * recipSinAngle;
    } else {
		// Perform a linear interpolation when cosTheta is close to 1
		// to avoid side effect of sin(angle) becoming a zero denominator.
        scale0 = 1.0f - t;
        scale1 = t;
    }
	// Interpolate and return new quaternion
    return Vector4_Add( Vector4_Scale( start, scale0 ), Vector4_Scale( q1, scale1 ) );
}
// Constructs a quaternion from the Euler angles.
const Vector4 Quaternion_RotationPitchRollYaw( float pitch, float roll, float yaw )
{
#if 0
	V3f s, c;	// the sine and the cosine of the half angle
	mmSinCos( pitch * 0.5f, s.x, c.x );
	mmSinCos( roll * 0.5f, s.y, c.y );
	mmSinCos( yaw * 0.5f, s.z, c.z );

	Vector4 result;
	result.w = c.x * c.y * c.z + s.x * s.y * s.z;
	result.x = s.x * c.y * c.z - c.x * s.y * s.z;
	result.y = c.x * s.y * c.z + s.x * c.y * s.z;
	result.z = c.x * c.y * s.z - s.x * s.y * c.z;
	return result;
#else
	double heading = pitch, attitude = roll, bank = yaw;
    // Assuming the angles are in radians.
    double c1 = cos(heading/2);
    double s1 = sin(heading/2);
    double c2 = cos(attitude/2);
    double s2 = sin(attitude/2);
    double c3 = cos(bank/2);
    double s3 = sin(bank/2);
    double c1c2 = c1*c2;
    double s1s2 = s1*s2;
	Vector4 result;
    ((V4f&)result).w =c1c2*c3 - s1s2*s3;
  	((V4f&)result).x =c1c2*s3 + s1s2*c3;
	((V4f&)result).y =s1*c2*c3 + c1*s2*s3;
	((V4f&)result).z =c1*s2*c3 - s1*c2*s3;
	return result;
#endif
}
// Constructs a quaternion from the (normalized) rotation axis and the rotation angle.
// 'normalAxis' - normalized axis of rotation
// 'angle' - rotation angle, in radians
const Vector4 Quaternion_RotationNormal( const V3f& normalAxis, float angle )
{
	// See: http://lolengine.net/blog/2013/09/18/beautiful-maths-quaternion-from-vectors
	float s, c;	// the sine and the cosine of the half angle
	mmSinCos( angle * 0.5f, s, c );

	Vector4 result;
	((V4f&)result).x = normalAxis.x * s;
	((V4f&)result).y = normalAxis.y * s;
	((V4f&)result).z = normalAxis.z * s;
	((V4f&)result).w = c;
	return result;
}


EPlaneType CalculatePlaneType( const V4f& plane )
{
	const V3f& N = Plane_GetNormal(plane);

	if ( N[0] == 0.0f ) {
		if ( N[1] == 0.0f ) {
			return N[2] > 0.0f ? EPlaneType::PLANETYPE_Z : EPlaneType::PLANETYPE_NEGZ;
		} else if ( N[2] == 0.0f ) {
			return N[1] > 0.0f ? EPlaneType::PLANETYPE_Y : EPlaneType::PLANETYPE_NEGY;
		} else {
			return EPlaneType::PLANETYPE_ZEROX;
		}
	}
	else if ( N[1] == 0.0f ) {
		if ( N[2] == 0.0f ) {
			return N[0] > 0.0f ? EPlaneType::PLANETYPE_X : EPlaneType::PLANETYPE_NEGX;
		} else {
			return EPlaneType::PLANETYPE_ZEROY;
		}
	}
	else if ( N[2] == 0.0f ) {
		return EPlaneType::PLANETYPE_ZEROZ;
	}
	else {
		return EPlaneType::PLANETYPE_NONAXIAL;
	}
}

void buildOrthonormalBasis( const V3f& look, V3f &right_, V3f &up_ )
{
	V3f vUp = V3_UP;

	V3f vRight = V3_Cross( look, vUp );
	float len = V3_Length( vRight );
	if( len < 1e-3f ) {
		vRight = V3_RIGHT;	// light direction is too close to the UP vector
	} else {
		vRight /= len;
	}

	right_ = vRight;
	up_ = V3_Cross( vRight, look );
}

const V3f AABBf_GetNormalizedPosition( const AABBf& aabb, const V3f& positionInAabb )
{
	mxASSERT(AABBf_ContainsPoint( aabb, positionInAabb ));
	const V3f aabbSize = AABBf_FullSize(aabb);
	const V3f relativePosition = positionInAabb - aabb.min_corner;	// [0..aabb_size]
	mxASSERT(V3_AllGreaterOrEqual( relativePosition, 0.0f ));
	return V3_Multiply( relativePosition, V3_Reciprocal(aabbSize) );// [0..1]
}
const V3f AABBf_GetOriginalPosition( const AABBf& aabb, const V3f& normalizedPosition )
{
	const V3f aabbSize = AABBf_FullSize(aabb);
	const V3f relativePosition = V3_Multiply( normalizedPosition, aabbSize );	// [0..aabb_size]
	return aabb.min_corner + relativePosition;
}

void AABBf_EncodePosition( const AABBf& aabb, const V3f& point, UINT8 output[3] )
{
	const V3f rescaledPosition = AABBf_GetNormalizedPosition( aabb, point );// [0..1]
	const V3f quantizedPosition = rescaledPosition * 255.0f;	// [0..255]
	output[0] = quantizedPosition.x;
	output[1] = quantizedPosition.y;
	output[2] = quantizedPosition.z;
}
void AABBf_DecodePosition( const AABBf& aabb, const UINT8 input[3], V3f *point )
{
	const V3f quantizedPosition = V3_Set(input[0],input[1],input[2]);	// [0..255]
	const V3f rescaledPosition = quantizedPosition * (1.0f/255.0f);	// [0..1]
	*point = AABBf_GetOriginalPosition( aabb, rescaledPosition );
}
const V3f AABBf_GetNormal( const AABBf& aabb, const V3f& point )
{
	V3f normal;
	float minDistance = MAX_FLOAT32;

	const V3f p = point - AABBf_Center( aabb );
	const V3f aabbExtent = AABBf_Extent( aabb );	// half size

	for( int i = 0; i < 3; ++i )
	{
		float distance = fabs( aabbExtent[i] - fabs(p[i]) );

		if( distance < minDistance ) {
			minDistance = distance;
			normal = Vector4_As_V3(g_CardinalAxes[i]) * Sign(point[i]);
		}
	}
	return normal;
}
float AABBf_SurfaceArea( const AABBf& aabb )
{
	const V3f size = AABBf_FullSize(aabb);
	return ( size.x * size.z + size.x * size.y + size.y * size.z ) * 2.0f;
}

bool AABBf_Equal( const AABBf& a, const AABBf& b, float epsilon )
{
	return V3_Equal( a.min_corner, b.min_corner, epsilon ) && V3_Equal( a.max_corner, b.max_corner, epsilon );
}

const AABBf AABBf_From_Points_Aligned( UINT _count, const V3f* _points, UINT _stride )
{
	mxASSERT( _count > 0 );
	mxASSERT( _points && IS_16_BYTE_ALIGNED(_points) );
	mxASSERT( _stride >= sizeof(Vector4) && (_stride % 16 == 0) );

	// find the minimum and maximum x, y, and z
	Vector4 vMin, vMax;

	vMin = vMax = Vector4_Load( (float*)_points );

	for( UINT i = 1; i < _count; i++ )
	{
		Vector4 Point = Vector4_Load( ( float* )( ( BYTE* )_points + i * _stride ) );
		vMin = Vector4_Min( vMin, Point );
		vMax = Vector4_Max( vMax, Point );
	}

	AABBf	result;
	result.min_corner = V4_As_V3((V4f&)vMin);
	result.max_corner = V4_As_V3((V4f&)vMax);
	// Store center and extents.
	//XMStoreFloat3( &pOut->Center, ( vMin + vMax ) * 0.5f );
	//XMStoreFloat3( &pOut->Extents, ( vMax - vMin ) * 0.5f );
	return result;
}

const AABBf AABBf_From_Points_Unaligned( UINT _count, const V3f* _points, UINT _stride )
{
	mxASSERT( _count > 0 );
	mxASSERT( _points );
	mxASSERT( _stride > 0 );

	// find the minimum and maximum x, y, and z
	Vector4 vMin, vMax;

	vMin = vMax = Vector4_LoadFloat3_Unaligned( (float*)_points );

	for( UINT i = 1; i < _count; i++ )
	{
		Vector4 Point = Vector4_LoadFloat3_Unaligned( ( float* )( ( BYTE* )_points + i * _stride ) );
		vMin = Vector4_Min( vMin, Point );
		vMax = Vector4_Max( vMax, Point );
	}

	AABBf	result;
	result.min_corner = V4_As_V3((V4f&)vMin);
	result.max_corner = V4_As_V3((V4f&)vMax);
	return result;
}

M44f M44_getTransform( const AABBf& from, const AABBf& to )
{
	const V3f source_size = from.size();
	const M44f center_around_origin = M44_buildTranslationMatrix( -from.min_corner - source_size * 0.5f );

	const V3f destination_size = to.size();
	const M44f scale_to_destination_size = M44_scaling( V3f::div( destination_size, source_size ) );

	const M44f translate_from_origin_to_destination = M44_buildTranslationMatrix( to.center() );

	const M44f combined_transform = center_around_origin * scale_to_destination_size * translate_from_origin_to_destination;

	return combined_transform;
}

// We can detect the intersection between the ray and the box
// by detecting the intersections between the ray and the three pairs of slabs.
// see "Ray - Box Intersection", "slab" method by Kay and Kayjia:
// http://www.siggraph.org/education/materials/HyperGraph/raytrace/rtinter3.htm
// TODO: https://tavianator.com/fast-branchless-raybounding-box-intersections/
int intersectRayAABB(
					 const V3f& _minimum, const V3f& _maximum,
					 const V3f& _ro, const V3f& _rd,
					 float& _tnear, float& _tfar
					 )
{
	// Refactor
	int ret=-1;

	_tnear = -MAX_FLOAT32;
	_tfar = MAX_FLOAT32;
// PT: why did we change the initial epsilon value?
//#define LOCAL_EPSILON PX_EPS_F32
#define LOCAL_EPSILON 0.0001f

	for(unsigned int a=0; a<3; a++)
	{
		if( _rd.a[a] > -LOCAL_EPSILON && _rd.a[a] < LOCAL_EPSILON )
		{
			// ray parallel to planes in this direction
			if( _ro.a[a] < _minimum.a[a] || _ro.a[a] > _maximum.a[a] ) {
				return -1;// parallel AND outside box : no intersection possible
			}
			// the ray origin is between the slabs...
		}	
		else
		{
			// ray not parallel to planes in this direction

			// Compute intersection t value of ray with near and far plane of slab
			const float OneOverDir = 1.0f / _rd.a[a];
			float t1 = (_minimum.a[a] - _ro.a[a]) * OneOverDir;	// entry time
			float t2 = (_maximum.a[a] - _ro.a[a]) * OneOverDir;	// exit time

			unsigned int b = a;
			// Make (t1) be intersection with near plane, (t2) with far plane
			if(t1>t2)
			{
				// we want t1 to hold values for intersection with near plane
				TSwap(t1,t2);
				b += 3;
			}

			// Compute the intersection of slab intersection intervals
			if(t1>_tnear)
			{
				_tnear = t1;	// want largest Tnear, keep the maximal tnear
				ret = (int)b;
			}
			if(t2<_tfar) {
				_tfar=t2;	// want smallest Tfar - keep the minimal tfar
			}

			// Exit with no collision as soon as slab intersection becomes empty
			// If the maximal tnear is greater than the minimal tfar on the ray, the ray misses the box.
			if(_tnear>_tfar	// the ray misses the box
				||
				_tfar<LOCAL_EPSILON	// the box is behind the ray
				)
			{
				return -1;
			}
		}
	}

	if(_tnear>_tfar || _tfar<LOCAL_EPSILON) {
		return -1;
	}

#undef LOCAL_EPSILON

	// Ray intersects all 3 slabs
	return ret;
}

void Rayf::RecalcDerivedValues()
{
	inv_dir.x = (mmAbs(direction.x) > 1e-4f) ? mmRcp(direction.x) : 1.0f;
	inv_dir.y = (mmAbs(direction.y) > 1e-4f) ? mmRcp(direction.y) : 1.0f;
	inv_dir.z = (mmAbs(direction.z) > 1e-4f) ? mmRcp(direction.z) : 1.0f;
}

int IntersectRayAABB(
					 const Rayf& ray,
					 const V3f& aabb_min, const V3f& aabb_max,
					 float &tnear_, float &tfar_
					 )
{
	// Refactor
	int ret=-1;

	tnear_ = -MAX_FLOAT32;
	tfar_ = MAX_FLOAT32;
// PT: why did we change the initial epsilon value?
//#define LOCAL_EPSILON PX_EPS_F32
#define LOCAL_EPSILON 0.0001f

	for(unsigned int a=0; a<3; a++)
	{
		if( ray.direction.a[a] > -LOCAL_EPSILON && ray.direction.a[a] < LOCAL_EPSILON )
		{
			// ray parallel to planes in this direction
			if( ray.origin.a[a] < aabb_min.a[a] || ray.origin.a[a] > aabb_max.a[a] ) {
				return -1;// parallel AND outside box : no intersection possible
			}
			// the ray origin is between the slabs...
		}	
		else
		{
			// ray not parallel to planes in this direction

			// Compute intersection t value of ray with near and far plane of slab
			float t1 = (aabb_min.a[a] - ray.origin.a[a]) * ray.inv_dir.a[a];	// entry time
			float t2 = (aabb_max.a[a] - ray.origin.a[a]) * ray.inv_dir.a[a];	// exit time

			unsigned int b = a;
			// Make (t1) be intersection with near plane, (t2) with far plane
			if(t1>t2)
			{
				// we want t1 to hold values for intersection with near plane
				TSwap(t1,t2);
				b += 3;
			}

			// Compute the intersection of slab intersection intervals
			if(t1>tnear_)
			{
				tnear_ = t1;	// want largest Tnear, keep the maximal tnear
				ret = (int)b;
			}
			if(t2<tfar_) {
				tfar_=t2;	// want smallest Tfar - keep the minimal tfar
			}

			// Exit with no collision as soon as slab intersection becomes empty
			// If the maximal tnear is greater than the minimal tfar on the ray, the ray misses the box.
			if(tnear_>tfar_	// the ray misses the box
				||
				tfar_<LOCAL_EPSILON	// the box is behind the ray
				)
			{
				return -1;
			}
		}
	}

	if(tnear_>tfar_ || tfar_<LOCAL_EPSILON) {
		return -1;
	}

#undef LOCAL_EPSILON

	// Ray intersects all 3 slabs
	return ret;
}

mxSTOLEN("PhysX-3.3");
// Based on GD Mag code, but now works correctly when origin is inside the sphere.
// This version has limited accuracy.
//
// Original comments from "aug01", "schroeder.zip/collision.cpp":
// determines a collision between a ray and a sphere
//
// ray_start:	the start pos of the ray
// ray_dir:		the normalized direction of the ray
// length:		length of ray to check
// sphere_pos:	sphere position
// sphere_rad:	sphere radius
// hit_time:	(OUT) if a collision, contains the distance from ray.pos
// hit_pos:		(OUT) if a collision, contains the world point of the collision
//
// returns: true if a collision occurred
//
bool Sphere_IntersectsRay(
						  const V3f& center,
						  const F32 radius,
						  const V3f& origin,
						  const V3f& dir,
						  const F32 length,
						  F32 &_hit_time,
						  V3f &_hit_pos
						  )
{
	// get the offset vector
	const V3f offset = center - origin;

	// get the distance along the ray to the center point of the sphere
	const F32 ray_dist = V3_Dot( dir, offset );

	// get the squared distances
	const F32 off2 = V3_Dot( offset, offset );
	const F32 rad2 = radius * radius;
	if( off2 <= rad2 )
	{
		// we're in the sphere
		_hit_pos = origin;
		_hit_time = 0.0f;
		return true;
	}

	if( ray_dist <= 0 || (ray_dist - length) > radius )
	{
		// moving away from object or too far away
		return false;
	}

	// find hit distance squared
	const F32 d = rad2 - (off2 - ray_dist * ray_dist);
	if( d < 0.0f )
	{
		// ray passes by sphere without hitting
		return false;
	}

	// get the distance along the ray
	_hit_time = ray_dist - sqrt(d);
	if( _hit_time > length )
	{
		// hit point beyond length
		return false;
	}

	// sort out the details
	_hit_pos = origin + dir * _hit_time;
	_hit_time /= length;
	return true;
}

mxTODO("templated, prefer the 'double' type")
//template< typename REAL >
int RaySphereIntersection(
						   const V3f& ray_origin, const V3f& ray_direction,
						   const V3f& sphere_center, const float sphere_radius,
						   float *t1_, float *t2_
						   )
{
	float a = V3_LengthSquared( ray_direction );
	float b = ray_direction * (ray_origin - sphere_center) * 2.0f;
	float c = V3_LengthSquared( ray_origin - sphere_center ) - sphere_radius * sphere_radius;
	float d = b * b - 4.0f * a * c;

	if( d >= 0 )
	{
		d = mmSqrt( d );

		float t1 = (-b-d) / (2.0f*a);
		float t2 = (-b+d) / (2.0f*a);

		if( t1 > t2 ) {
			TSwap( t1, t2 );	// want smallest t1
		}

		*t1_ = t1;
		*t2_ = t2;

		return (d > 0) ? 2 : 1;
	}

	return 0;
}

// https://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm
// https://www.scratchapixel.com/lessons/3d-basic-rendering/ray-tracing-rendering-a-triangle/moller-trumbore-ray-triangle-intersection
//
int RayTriangleIntersection(
						  const V3f& V1,  // Triangle vertices
						  const V3f& V2,
						  const V3f& V3,
						  const V3f& O,  //Ray origin
						  const V3f& D,  //Ray direction
						  float* out,
						  float epsilon
						  )
{
	//find vectors for two edges sharing V1
	const V3f e1 = V3_Subtract(V2, V1);
	const V3f e2 = V3_Subtract(V3, V1);
	//Begin calculating determinant - also used to calculate u parameter
	const V3f P = V3_Cross(D, e2);
	//if determinant is near zero, ray lies in plane of triangle
	const float det = V3_Dot(e1, P);
	//NOT CULLING
	if(det > -epsilon && det < epsilon) {
		// This ray is parallel to this triangle.
		return 0;
	}

	const float inv_det = 1.f / det;

	//calculate distance from V1 to ray origin
	const V3f T = V3_Subtract(O, V1);

	//Calculate u parameter and test bound
	const float u = V3_Dot(T, P) * inv_det;
	if(u < 0.f || u > 1.f) {
		//The intersection lies outside of the triangle
		return 0;
	}

	//Prepare to test v parameter
	const V3f Q = V3_Cross(T, e1);

	//Calculate V parameter and test bound
	const float v = V3_Dot(D, Q) * inv_det;
	if(v < 0.f || u + v  > 1.f) {
		//The intersection lies outside of the triangle
		return 0;
	}

	// At this stage we can compute t to find out where the intersection point is on the line.
	const float t = V3_Dot(e2, Q) * inv_det;

	if(t > epsilon) { //ray intersection
		*out = t;
		return 1;
	}

	// This means that there is a line intersection but not a ray intersection.

	// No hit
	return 0;
}

float InterpolateTrilinear(
						   const V3f& point,
						   const V3f& min, const V3f& max,
						   const float values[8]
)
{
	const V2f& xy = V3_As_V2( point );
	const V2f& min_xy = V3_As_V2( min );
	const V2f& max_xy = V3_As_V2( max );
	// First bilinearly interpolate along XY plane...
	float xy_upper = InterpolateBilinear2( xy, min_xy, max_xy, values[6], values[7], values[4], values[5] );
	float xy_lower = InterpolateBilinear2( xy, min_xy, max_xy, values[2], values[3], values[0], values[1] );
	// ...then linearly interpolate along the Z-direction.
	float tz = GetLerpParam( min.z, max.z, point.z );
	return Float_Lerp01( xy_lower, xy_upper, tz );
	//xy_upper * ( max.z - point.z ) + xy_lower * ( point.z - min.z );

// Vertex/Octant enumeration:         Using locational (Morton) codes (X - lowest bit, Y - middle, Z - highest):
//	        6___________7                 110__________111
//	       /|           /                  /|           /
//	      / |          /|                 / |          /|
//	     /  |         / |                /  |         / |
//	    4------------5  |             100-----------101 |
//	    |   2________|__3              |  010_______|__011
//	    |   /        |  /              |   /        |  /
//	    |  /         | /               |  /         | /
//	    | /          |/                | /          |/
//	    0/___________1               000/___________001
//	    

}


#if !MM_NO_INTRINSICS

	mxREFLECT_STRUCT_AS_ANOTHER_STRUCT(Vector4, V4f);

	ATextStream & operator << ( ATextStream & log, const Vector4& _v )
	{
		const V4f& v = Vector4_As_V4(_v);
		log.PrintF("(%.3f, %.3f, %.3f, %.3f)",v.x,v.y,v.z,v.w);
		return log;
	}

#endif


#if MM_ENABLE_REFLECTION

	mxBEGIN_STRUCT(V2f)
		mxMEMBER_FIELD(x),
		mxMEMBER_FIELD(y),
	mxEND_REFLECTION

	mxBEGIN_STRUCT(V3f)
		mxMEMBER_FIELD(x),
		mxMEMBER_FIELD(y),
		mxMEMBER_FIELD(z),
	mxEND_REFLECTION

	mxBEGIN_STRUCT(V4f)
		mxMEMBER_FIELD(x),
		mxMEMBER_FIELD(y),
		mxMEMBER_FIELD(z),
		mxMEMBER_FIELD(w),
	mxEND_REFLECTION

	mxBEGIN_STRUCT(V3d)
		mxMEMBER_FIELD(x),
		mxMEMBER_FIELD(y),
		mxMEMBER_FIELD(z),
	mxEND_REFLECTION

	mxBEGIN_STRUCT(Int3)
		mxMEMBER_FIELD(x),
		mxMEMBER_FIELD(y),
		mxMEMBER_FIELD(z),
	mxEND_REFLECTION

	mxBEGIN_STRUCT(UInt3)
		mxMEMBER_FIELD(x),
		mxMEMBER_FIELD(y),
		mxMEMBER_FIELD(z),
	mxEND_REFLECTION

	mxBEGIN_STRUCT(M44f)
		mxMEMBER_FIELD(v0),
		mxMEMBER_FIELD(v1),
		mxMEMBER_FIELD(v2),
		mxMEMBER_FIELD(v3),
	mxEND_REFLECTION

	mxBEGIN_STRUCT(M34f)
		mxMEMBER_FIELD(r),
	mxEND_REFLECTION

	mxBEGIN_STRUCT(Half2)
		mxMEMBER_FIELD(x),
		mxMEMBER_FIELD(y),
	mxEND_REFLECTION

	mxBEGIN_STRUCT(Half4)
		mxMEMBER_FIELD(x),
		mxMEMBER_FIELD(y),
		mxMEMBER_FIELD(z),
		mxMEMBER_FIELD(w),
	mxEND_REFLECTION

	mxBEGIN_STRUCT(U11U11U10)
		mxMEMBER_FIELD(v),
	mxEND_REFLECTION

	mxBEGIN_STRUCT(R10G10B1A2)
		mxMEMBER_FIELD(v),
	mxEND_REFLECTION

	mxBEGIN_STRUCT(AABBf)
		mxMEMBER_FIELD(min_corner),
		mxMEMBER_FIELD(max_corner),
	mxEND_REFLECTION

	mxBEGIN_STRUCT(Sphere16)
		mxMEMBER_FIELD(center),
		mxMEMBER_FIELD(radius),
	mxEND_REFLECTION


	mxBEGIN_STRUCT(CubeMLf)
		mxMEMBER_FIELD(min_corner),
		mxMEMBER_FIELD(side_length),
	mxEND_REFLECTION

	mxBEGIN_STRUCT(CubeMLd)
		mxMEMBER_FIELD(min_corner),
		mxMEMBER_FIELD(side_length),
	mxEND_REFLECTION


	mxBEGIN_STRUCT(CubeCRf)
		mxMEMBER_FIELD(center),
		mxMEMBER_FIELD(radius),
	mxEND_REFLECTION

	mxBEGIN_STRUCT(CubeCRd)
		mxMEMBER_FIELD(center),
		mxMEMBER_FIELD(radius),
	mxEND_REFLECTION

#endif // MM_ENABLE_REFLECTION


#if MM_DEFINE_STREAM_OPERATORS

	ATextStream & operator << ( ATextStream & log, const UShort3& v )
	{
		log.PrintF("(%u, %u, %u)",v.x,v.y,v.z);
		return log;
	}
	ATextStream & operator << ( ATextStream & log, const Int3& v )
	{
		log.PrintF("(%d, %d, %d)",v.x,v.y,v.z);
		return log;
	}
	ATextStream & operator << ( ATextStream & log, const Int4& v )
	{
		log.PrintF("(%d, %d, %d, %d)",v.x,v.y,v.z,v.w);
		return log;
	}
	ATextStream & operator << ( ATextStream & log, const V2f& v )
	{
		log.PrintF("(%.3f, %.3f)",v.x,v.y);
		return log;
	}
	ATextStream & operator << ( ATextStream & log, const V3f& v )
	{
		log.PrintF("(%.3f, %.3f, %.3f)",v.x,v.y,v.z);
		return log;
	}
	ATextStream & operator << ( ATextStream & log, const V4f& v )
	{
		log.PrintF("(%.3f, %.3f, %.3f, %.3f)",v.x,v.y,v.z,v.w);
		return log;
	}
	ATextStream & operator << ( ATextStream & log, const M44f& m )
	{
		log << m.r0 << "\n";
		log << m.r1 << "\n";
		log << m.r2 << "\n";
		log << m.r3 << "\n";
		return log;
	}
	ATextStream & operator << ( ATextStream & log, const UInt3& v )
	{
		log.PrintF("(%u, %u, %u)",v.x,v.y,v.z);
		return log;
	}
	ATextStream & operator << ( ATextStream & log, const UInt4& v )
	{
		log.PrintF("(%u, %u, %u, %u)",v.x,v.y,v.z,v.w);
		return log;
	}
	ATextStream & operator << ( ATextStream & log, const UByte4& v )
	{
		log.PrintF("(%u, %u, %u, %u)",v.x,v.y,v.z,v.w);
		return log;
	}
	ATextStream & operator << ( ATextStream & log, const V3d& v )
	{
		log.PrintF("(%.3f, %.3f, %.3f)",v.x,v.y,v.z);
		return log;
	}
	//ATextStream & operator << ( ATextStream & log, const AABB& bounds )
	//{
	//	log << "{min=" << bounds.min_corner << ", max=" << bounds.max_corner << "}";
	//	return log;
	//}
#endif

#if 0

mxSTOLEN("XNAMath/DirectXMath");
// 3D vector: 11/11/10 floating-point components
// The 3D vector is packed into 32 bits as follows: a 5-bit biased exponent
// and 6-bit mantissa for x component, a 5-bit biased exponent and
// 6-bit mantissa for y component, a 5-bit biased exponent and a 5-bit
// mantissa for z. The z component is stored in the most significant bits
// and the x component in the least significant bits. No sign bits so
// all partial-precision numbers are positive.
// (Z10Y11X11): [32] ZZZZZzzz zzzYYYYY yyyyyyXX XXXxxxxx [0]
struct X11Y11Z10f
{
    union
    {
        struct
        {
            UINT xm : 6;
            UINT xe : 5;

			UINT ym : 6;
            UINT ye : 5;

			UINT zm : 5;
            UINT ze : 5;
        };

		UINT v;
    };
};

#if defined(_MSC_VER) && !defined(_M_ARM) && !defined(_M_ARM64) && !defined(_M_HYBRID_X86_ARM64) && (!_MANAGED) && (!_M_CEE) && (!defined(_M_IX86_FP) || (_M_IX86_FP > 1)) && !defined(_XM_NO_INTRINSICS_) && !defined(_XM_VECTORCALL_)
	#if ((_MSC_FULL_VER >= 170065501) && (_MSC_VER < 1800)) || (_MSC_FULL_VER >= 180020418)
	#define _XM_VECTORCALL_ 1
	#endif
#endif

#if _XM_VECTORCALL_
#define XM_CALLCONV __vectorcall
#else
#define XM_CALLCONV __fastcall
#endif

inline void XM_CALLCONV X11Y11Z10f_StoreFloat3
(
    X11Y11Z10f* pDestination,
    const float* V
)
{
    assert(pDestination);

    __declspec(align(16)) uint32_t IValue[4];
	{
		float* p = reinterpret_cast<float*>(&IValue);
		p[0] = V[0];
		p[1] = V[1];
		p[2] = V[2];
	}

    uint32_t Result[3];

    // X & Y Channels (5-bit exponent, 6-bit mantissa)
    for(uint32_t j=0; j < 2; ++j)
    {
        uint32_t Sign = IValue[j] & 0x80000000;	// get the sign bit
        uint32_t I = IValue[j] & 0x7FFFFFFF;	// remove the sign bit

        if ((I & 0x7F800000) == 0x7F800000)	// 0x7F800000 is the mask for bits of the exponential field.
        {
            // INF or NAN
            Result[j] = 0x7c0;
            if (( I & 0x7FFFFF ) != 0)
            {
                Result[j] = 0x7c0 | (((I>>17)|(I>>11)|(I>>6)|(I))&0x3f);
            }
            else if ( Sign )
            {
                // -INF is clamped to 0 since 3PK is positive only
                Result[j] = 0;
            }
        }
        else if ( Sign )
        {
            // 3PK is positive only, so clamp to zero
            Result[j] = 0;
        }
        else if (I > 0x477E0000U)
        {
            // The number is too large to be represented as a float11, set to max
            Result[j] = 0x7BF;
        }
        else
        {
            if (I < 0x38800000U)
            {
                // The number is too small to be represented as a normalized float11
                // Convert it to a denormalized value.
                uint32_t Shift = 113U - (I >> 23U);
                I = (0x800000U | (I & 0x7FFFFFU)) >> Shift;
            }
            else
            {
                // Rebias the exponent to represent the value as a normalized float11
                I += 0xC8000000U;
            }
     
            Result[j] = ((I + 0xFFFFU + ((I >> 17U) & 1U)) >> 17U)&0x7ffU;
        }
    }

    // Z Channel (5-bit exponent, 5-bit mantissa)
    uint32_t Sign = IValue[2] & 0x80000000;
    uint32_t I = IValue[2] & 0x7FFFFFFF;

    if ((I & 0x7F800000) == 0x7F800000)
    {
        // INF or NAN
        Result[2] = 0x3e0;
        if ( I & 0x7FFFFF )
        {
            Result[2] = 0x3e0 | (((I>>18)|(I>>13)|(I>>3)|(I))&0x1f);
        }
        else if ( Sign )
        {
            // -INF is clamped to 0 since 3PK is positive only
            Result[2] = 0;
        }
    }
    else if ( Sign )
    {
        // 3PK is positive only, so clamp to zero
        Result[2] = 0;
    }
    else if (I > 0x477C0000U)
    {
        // The number is too large to be represented as a float10, set to max
        Result[2] = 0x3df;
    }
    else
    {
        if (I < 0x38800000U)
        {
            // The number is too small to be represented as a normalized float10
            // Convert it to a denormalized value.
            uint32_t Shift = 113U - (I >> 23U);
            I = (0x800000U | (I & 0x7FFFFFU)) >> Shift;
        }
        else
        {
            // Rebias the exponent to represent the value as a normalized float10
            I += 0xC8000000U;
        }
     
        Result[2] = ((I + 0x1FFFFU + ((I >> 18U) & 1U)) >> 18U)&0x3ffU;
    }

    // Pack Result into memory
    pDestination->v = (Result[0] & 0x7ff)
                      | ( (Result[1] & 0x7ff) << 11 )
                      | ( (Result[2] & 0x3ff) << 22 );
}

#endif

/*
nsf/linear_math.c
https://gist.github.com/nsf/796537
*/
