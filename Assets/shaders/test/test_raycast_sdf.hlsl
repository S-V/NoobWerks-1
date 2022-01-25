// Renders a Signed Distance Field (SDF) using ray marching.
// Useful links:
// modeling with distance functions:
// http://www.iquilezles.org/www/articles/distfunctions/distfunctions.htm
// "J.C. Hart. Sphere tracing: A geometric method for the antialiased ray tracing of implicit surfaces. The Visual Computer, 12(10):527-545, 1996"
// "GPU Ray Marching of Distance Fields, Lukasz Jaroslaw Tomczak, 2012, P.14, 2.2.2 Sphere tracing"

#include "_screen_shader.h"
#include "_PS.h"

cbuffer Uniforms {
	float4	u_cameraPosition;	// XYZ - camera position in world space, W - far plane
	// these are unit vectors in world space:
	float4	u_cameraForward;	// XYZ - camera look-to direction, W - aspect ratio
	float4	u_cameraRight;	// XYZ - camera 'Right' vector, W - tan of half vertical FoV
	float4	u_cameraUp;	// camera 'Up' vector
//	AtmosphereParameters	u_atmosphere;
};

struct VS_Output
{
    float4 position : SV_Position;
    float2 texCoord : TexCoord0;
	float3 rayFarWS : TexCoord1;	// the ray's end point on the far image plane
};
// The sky is rendered as a fullscreen triangle. The pixel shader traces rays through each pixel.
VS_Output VS_Main( in uint vertexID : SV_VertexID )	// vertexID:=[0,1,2]
{
    VS_Output output;
	GetFullScreenTrianglePosTexCoord( vertexID, output.position, output.texCoord );

	const float farPlaneDist = u_cameraPosition.w;	// distance from eye position to the image plane
	const float aspectRatio = u_cameraForward.w;	// screen width / screen height
	const float tanHalfFoVy = u_cameraRight.w;
	
	const float imagePlaneH = farPlaneDist * tanHalfFoVy;	// half-height of the image plane
	const float imagePlaneW = imagePlaneH * aspectRatio;	// half-width of the image plane

	const float3 f3FarCenter = u_cameraForward.xyz * farPlaneDist;
	const float3 f3FarRight = u_cameraRight.xyz * imagePlaneW;	// scaled 'right' vector on the far plane, in world space
	const float3 f3FarUp = u_cameraUp.xyz * imagePlaneH;	// scaled 'up' vector on the far plane, in world space

	// Compute the positions of the triangle's corners on the far image plane, in world space.
	float3 cornersWS[3];
	// lower-left ( UV = ( 0, 2 ) )
	cornersWS[0] = u_cameraPosition.xyz + f3FarCenter - (f3FarRight * 1) - (f3FarUp * 3);
	// upper left ( UV = ( 0, 0 ) )
	cornersWS[1] = u_cameraPosition.xyz + f3FarCenter - (f3FarRight * 1) + (f3FarUp * 1);
	// upper-right ( UV = ( 2, 0 ) )
	cornersWS[2] = u_cameraPosition.xyz + f3FarCenter + (f3FarRight * 3) + (f3FarUp * 1);

	// the positions on the far image plane will be linearly interpolated and passed to the pixel shader
	output.rayFarWS = cornersWS[ vertexID ];

    return output;
}

/// Plane with normal n (n is normalized) at some distance from the origin
float fPlane( in float3 p, in float3 n, float distanceFromOrigin ) {
	return dot( p, n ) + distanceFromOrigin;
}
float fSphere( in float3 p, in float r ) {
	return length(p) - r;
}

float vmin( in float3 v ) {
	return min(min(v.x, v.y), v.z);
}
float vmax( in float3 v ) {
	return max(max(v.x, v.y), v.z);
}
float fCube( in float3 p, in float3 center, in float3 halfSize )
{
	const float3 d = abs( p - center ) - halfSize;
	return length( max( d, (float3)0 ) ) + vmax( min( d, (float3)0 ) );
}
float fInfiniteCross( in float3 p, in float3 center, in float radius )
{
	float inf = 1e6f;
	float da = fCube(p.xyz, center, float3( inf, radius, radius) );
	float db = fCube(p.yzx, center, float3( radius, inf, radius ) );
	float dc = fCube(p.zxy, center, float3( radius, radius, inf ) );
	return min( min(da,db), dc );
}
// const float3 V3_Mod( in float3 _v, float _f )
// {
	// float3 result;
	// result.x = modf( _v.x, _f );
	// result.y = modf( _v.y, _f );
	// result.z = modf( _v.z, _f );
	// return result;
// }

/// The Menger sponge is a three-dimensional fractal curve that exhibits infinite surface area and zero volume.
/// This sponge has the property that every conceivable curve can be embedded within it.
float fMengerSponge( in float3 samplePoint, in float radius ) {
	float d = fCube( samplePoint, (float3)0, (float3)radius );
	float scale = 1.0f;
	for( int i = 0; i < 3; ++i )
	{
		const float repeat = (2.0f * radius) / scale;
		//const float3 pos2 = modf( samplePoint, repeat );
		int3 intpart;
		const float3 fracpart = modf( samplePoint / repeat, intpart );
		const float3 pos2 = fracpart * repeat;

		const float crossRadius = radius / scale;
		const float distCross = fInfiniteCross( pos2 * 3.f, (float3)0, crossRadius ) / 3.f;
		d = max( d, -distCross );
		scale *= 3.0f;
	}
	return d;
}

float fSceneSDF( in float3 samplePoint ) {
    // float sphereDist = sphereSDF(samplePoint / 1.2) * 1.2;
    // float cubeDist = cubeSDF(samplePoint + vec3(0.0, sin(iGlobalTime), 0.0));
    // return intersectSDF(cubeDist, sphereDist);
	//return fSphere( samplePoint, 2 );
	//return fMengerSponge( samplePoint, 5 );
	return min( fPlane( samplePoint, float3(0,0,1), 0 ), fSphere( samplePoint, 3 ) );
}




static const int MAX_STEPS = 16;
static const float SDF_EPSILON = 1e-3f;

bool CastRay_SphereTracing(
						   in float3 _rayStart,
						   in float3 _rayDir,
						   in float _tmax,
						   out float _thit
						   )
{
	float time = 0.0f;
#if 1
	const float dt = 1e-3f;
	while( time < _tmax )
	{
		const float3 samplePos = _rayStart + _rayDir * time;
		const float dist = fSceneSDF( samplePos );
		if( dist < SDF_EPSILON ) {
			_thit = time;
			return true;
		}
		time += dist;
	}
#else
	for( int iteration = 0; iteration < MAX_STEPS; iteration++ )
	{
		const float3 samplePos = _rayStart + _rayDir * time;
		const float dist = fSceneSDF( samplePos );
		if( dist < SDF_EPSILON ) {
			_thit = time;
			return true;
		}
		time += dist;
	}
#endif
	return false;
}

/// In order that normals be defined along an implicit surface,
/// the function must be continuous and differentiable.
/// That is, the first partial derivatives must be continuous
/// and not all zero, everywhere on the surface.
float3 EstimateNormal( in float3 _pos, in float H = 1e-3f )
{
#if 0
	// The gradient of the distance field yields the normal.
	float	d0	= fSceneSDF( float3( _pos.x, _pos.y, _pos.z ) );
	float	Nx	= fSceneSDF( float3( _pos.x + H, _pos.y, _pos.z ) ) - d0;
	float	Ny	= fSceneSDF( float3( _pos.x, _pos.y + H, _pos.z ) ) - d0;
	float	Nz	= fSceneSDF( float3( _pos.x, _pos.y, _pos.z + H ) ) - d0;
#else
	// "John C. Hart, Daniel J. S, and Louis H. Kauffman T. Ray Tracing Deterministic 3-D Fractals. Computer Graphics, 23:289–296, 1989."
	// 6-neighbor central difference gradient estimator.
	// The 6-point gradient might be extended to versions where 18 or 26 samples are taken.
	// Centered difference approximations are more precise than one-sided approximations.
	const float Nx = fSceneSDF(float3(_pos.x + H, _pos.y, _pos.z)) - fSceneSDF(float3(_pos.x - H, _pos.y, _pos.z));
	const float Ny = fSceneSDF(float3(_pos.x, _pos.y + H, _pos.z)) - fSceneSDF(float3(_pos.x, _pos.y - H, _pos.z));
	const float Nz = fSceneSDF(float3(_pos.x, _pos.y, _pos.z + H)) - fSceneSDF(float3(_pos.x, _pos.y, _pos.z - H));
#endif
	float3	N = normalize( float3( Nx, Ny, Nz ) );
	return N;
}

float3 PS_Main( in VS_Output _inputs ) : SV_Target
{
	float3 f3RayOrigin = u_cameraPosition.xyz;
	float3 f3RayEnd = _inputs.rayFarWS;
	float3 f3RayDirection = normalize( f3RayEnd - f3RayOrigin );

    float3 color = 0;

	float thit;
	if( CastRay_SphereTracing( f3RayOrigin, f3RayDirection, 9999, thit ) )
	{
		const float3 N = EstimateNormal( f3RayOrigin + f3RayDirection * thit );
		color = dot( float3(0,0,1), N );
	}
	
	// Apply exposure.
    color = 1.0 - exp(-1.0 * color);
	return color;
}
