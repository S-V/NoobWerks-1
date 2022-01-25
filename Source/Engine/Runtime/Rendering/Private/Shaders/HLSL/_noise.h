// utility functions for procedural noise generation
#ifndef __NOISE_H__
#define __NOISE_H__

#include <Utility/_GLSL_to_HLSL.h>


// Linear
// This is the good old Pythagorean theorem.
inline float EuclidianDistanceFunc(vec3 p1, vec3 p2)
{
#ifdef __cplusplus
	return V3_Length( V3_Subtract( p1, p2 ) );
#else
	return distance( p1, p2 );
#endif
}

// Linear Squared
// This is the same as before except you do not take the square root of the ending total. This in turn saves some computation time.

// Manhattan
// This method of computing distances is based off of how one would travel in a city. So in this is the shortest distance that could be traveled with no diagonal lines. The main thing here is that the delta(x) and delta(y) must be positive. (Self error note: This was a problem when I first started in that I would have these perfect diagonal gradients on the screen.
// Also if you do not take the absolute value of any of it you will get some weird results.)
inline float ManhattanDistanceFunc(vec3 p1, vec3 p2)
{
	return abs(p1.x - p2.x) + abs(p1.y - p2.y) + abs(p1.z - p2.z);
}

#if 0
// Chebyshev
// This way is similar to the Manhattan method in that it goes based of individual changes in x or y.
// The only thing different here is that it includes diagonals.
// This comes from this methods is based off how a king can move in chess.
// By adding the diagonal it adds the possibility of traveling both to a and b at the same time.
// This causes the value to be which ever number is lower.
inline float ChebyshevDistanceFunc(vec3 p1, vec3 p2)
{
	vec3 diff = p1 - p2;
	return max(max(abs(diff.x), abs(diff.y)), abs(diff.z));
}
#endif

//-----------------------------------------------------------------------------
// These two function due to Christophe Schlick from "Fast Alternatives to 
// "Perlin's Bias and Gain Functions" Graphics Gems IV, p379-382.
// Also, see: http://blog.demofox.org/2012/09/24/bias-and-gain-are-your-friend/
//-----------------------------------------------------------------------------
inline float bias( float x, float a )
{
	return x / ((1/a - 2)*(1 - x) + 1);
}

inline float gain( float x, float a )
{
	if( x < 0.5 )
		return x / ((1/a - 2)*(1 - 2*x) + 1);
	else
		return ((1/a - 2)*(1 - 2*x) - x) / ((1/a - 2)*(1 - 2*x) - 1);
}


// Worley noise is a noise function introduced by Steven Worley in 1996
// (A Cellular Texture Basis Function, Siggraph 1996).
// Also known as Cell/Cellular Noise or occasionally Voronoi Noise,
// it is built by computing the distance to randomly distributed points,
// and weighting the lightness of the each pixel by the distance from the closest point.
// Evaluating a point using Steven Worley’s algorithm consist of the following steps:
// 1. Determine which cube the evaluation point is in
// 2. Generate a reproducible random number generator for the cube
// 3. Determine how many feature points are in the cube
// 4. Randomly place the feature points in the cube
// 5. find the feature point closest to the evaluation point
// 6. Check the neighboring cubes to ensure their are no closer evaluation points.


//=======================================================================================
// Created by inigo quilez - iq/2014
// http://iquilezles.org/www/articles/voronoise/voronoise.htm
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
// Made more readable and understandable by Manuel Riecke/API_Beast

/// The output is pseudo-random, in the range [0,1].
vec2 random2f(vec2 p)
{
	vec2 q = vec2(dot(p,vec2(127.1,311.7)), 
		dot(p,vec2(269.5,183.3)));
	return fract(sin(q)*43758.5453);
}
float randomf(vec2 p)
{
	return fract(sin(dot(p,vec2(419.2,371.9))) * 833458.57832);
}

// irregular: [0..1]
// smoothness: [0..1]
float iqnoise(in vec2 pos, float irregular, float smoothness)
{
	vec2 cell = floor(pos);
	vec2 cellOffset = fract(pos);

	float sharpness = 1.0 + 63.0 * pow(1.0-smoothness, 4.0);

	float value = 0.0;
	float accum = 0.0;
	// Sample the surrounding cells, from -2 to +2
	// This is necessary for the smoothing as well as the irregular grid.
	for(int x=-2; x<=2; x++ )
	{
		for(int y=-2; y<=2; y++ )
		{
			vec2 samplePos = vec2(float(y), float(x));

			// Center of the cell is not at the center of the block for irregular noise.
			// Note that all the coordinates are in "block"-space, 0 is the current block, 1 is one block further, etc
			vec2 center = random2f(cell + samplePos) * irregular;
			float centerDistance = length(samplePos - cellOffset + center);

			// High sharpness = Only extreme values = Hard borders = 64
			// Low sharpness = No extreme values = Soft borders = 1
			float sample = pow(abs(1.0 - smoothstep(0.0, 1.414, centerDistance)), (sharpness));

			// A different "color" (shade of gray) for each cell
			float color = randomf(cell + samplePos);
			value += color * sample;
			accum += sample;
		}
	}
	return value/accum;
}
//=======================================================================================

float voronoi( in vec2 x )
{
	ivec2 p = floor( x );
	vec2  f = fract( x );

	float res = 8.0;
	for( int j=-1; j<=1; j++ )
		for( int i=-1; i<=1; i++ )
		{
			ivec2 b = ivec2( i, j );
			vec2  r = vec2( b ) - f + random2f( p + b );
			float d = dot( r, r );

			res = min( res, d );
		}
		return sqrt( res );
}

// http://iquilezles.org/www/articles/smoothvoronoi/smoothvoronoi.htm

float smoothVoronoi( in vec2 x )
{
	ivec2 p = floor( x );
	vec2  f = fract( x );

	float res = 0.0;
	for( int j=-1; j<=1; j++ )
		for( int i=-1; i<=1; i++ )
		{
			ivec2 b = ivec2( i, j );
			vec2  r = vec2( b ) - f + random2f( p + b );
			float d = dot( r, r );

			res += 1.0/pow( d, 8.0 );
		}
		return pow( 1.0/res, 1.0/16.0 );
}
float smoothVoronoi2( in vec2 x )
{
	ivec2 p = floor( x );
	vec2  f = fract( x );

	float res = 0.0;
	for( int j=-1; j<=1; j++ )
		for( int i=-1; i<=1; i++ )
		{
			ivec2 b = ivec2( i, j );
			vec2  r = vec2( b ) - f + random2f( p + b );
			float d = length( r );

			res += exp( -32.0*d );
		}
		return -(1.0/32.0)*log( res );
}

#if 0
/// <summary>
/// Constant used in FNV hash function.
/// FNV hash: http://isthe.com/chongo/tech/comp/fnv/#FNV-source
/// </summary>
const uint FNV_OFFSET_BASIS = 2166136261;
/// <summary>
/// Constant used in FNV hash function
/// FNV hash: http://isthe.com/chongo/tech/comp/fnv/#FNV-source
/// </summary>
const uint FNV_PRIME = 16777619;
/// <summary>
/// Hashes three integers into a single integer using FNV hash.
/// FNV hash: http://isthe.com/chongo/tech/comp/fnv/#FNV-source
/// </summary>
/// <returns>hash value</returns>
inline uint FNV_Hash(uint i, uint j, uint k)
{
	return (uint)((((((FNV_OFFSET_BASIS ^ (uint)i) * FNV_PRIME) ^ (uint)j) * FNV_PRIME) ^ (uint)k) * FNV_PRIME);
}
#endif




// Stolen from FlightGear Flight Simulator: free open-source multiplatform flight sim:
// http://wiki.flightgear.org/Procedural_Texturing
// https://sourceforge.net/p/flightgear/fgdata/ci/next/tree/Shaders/noise.frag
//
// http://www.science-and-fiction.org/rendering/noise.html
// 
// This is a library of noise functions, taking a coordinate vector and a wavelength
// as input and returning a number [0:1] as output.

// * Noise2D(in vec2 coord, in float wavelength) is 2d Perlin noise
// * Noise3D(in vec3 coord, in float wavelength) is 3d Perlin noise
// * DotNoise2D(in vec2 coord, in float wavelength, in float fractionalMaxDotSize, in float dDensity)
//   is sparse dot noise and takes a dot density parameter
// * DropletNoise2D(in vec2 coord, in float wavelength, in float fractionalMaxDotSize, in float dDensity)
//   is sparse dot noise modified to look like liquid and takes a dot density parameter
// * VoronoiNoise2D(in vec2 coord, in float wavelength, in float xrand, in float yrand)
//   is a function mapping the terrain into random domains, based on Voronoi tiling of a regular grid
//   distorted with xrand and yrand
// * SlopeLines2D(in vec2 coord, in vec2 gradDir, in float wavelength, in float steepness)
//   computes a semi-random set of lines along the direction of steepest descent, allowing to
//   simulate e.g. water erosion patterns
// * Strata3D(in vec3 coord, in float wavelength, in float variation)
//   computers a vertically stratified random pattern, appropriate e.g. for rock textures 

// Thorsten Renk 2014


float rand2D(in vec2 co)
{
	return fract(sin(dot(co.xy, vec2(12.9898,78.233))) * 43758.5453);
}

float rand3D(in vec3 co)
{
	return fract(sin(dot(co.xyz, vec3(12.9898,78.233,144.7272))) * 43758.5453);
}

//float cosine_interpolate(in float a, in float b, in float x)
//{
//	float ft = x * 3.1415927;
//	float f = (1.0 - cos(ft)) * .5;
//
//	return  a*(1.0-f) + b*f;
//}

float simple_interpolate(in float a, in float b, in float x)
{
	return a + smoothstep(0.0,1.0,x) * (b-a);
}

float interpolatedNoise2D(in float x, in float y)
{
	float integer_x    = x - fract(x);
	float fractional_x = x - integer_x;

	float integer_y    = y - fract(y);
	float fractional_y = y - integer_y;

	float v1 = rand2D(vec2(integer_x, integer_y));
	float v2 = rand2D(vec2(integer_x+1.0, integer_y));
	float v3 = rand2D(vec2(integer_x, integer_y+1.0));
	float v4 = rand2D(vec2(integer_x+1.0, integer_y +1.0));

	float i1 = simple_interpolate(v1 , v2 , fractional_x);
	float i2 = simple_interpolate(v3 , v4 , fractional_x);

	return simple_interpolate(i1, i2, fractional_y);
}

float interpolatedNoise3D(in float x, in float y, in float z)
{
	float integer_x    = x - fract(x);
	float fractional_x = x - integer_x;

	float integer_y    = y - fract(y);
	float fractional_y = y - integer_y;

	float integer_z    = z - fract(z);
	float fractional_z = z - integer_z;

	float v1 = rand3D(vec3(integer_x, integer_y, integer_z));
	float v2 = rand3D(vec3(integer_x+1.0, integer_y, integer_z));
	float v3 = rand3D(vec3(integer_x, integer_y+1.0, integer_z));
	float v4 = rand3D(vec3(integer_x+1.0, integer_y +1.0, integer_z));

	float v5 = rand3D(vec3(integer_x, integer_y, integer_z+1.0));
	float v6 = rand3D(vec3(integer_x+1.0, integer_y, integer_z+1.0));
	float v7 = rand3D(vec3(integer_x, integer_y+1.0, integer_z+1.0));
	float v8 = rand3D(vec3(integer_x+1.0, integer_y +1.0, integer_z+1.0));


	float i1 = simple_interpolate(v1,v5, fractional_z);
	float i2 = simple_interpolate(v2,v6, fractional_z);
	float i3 = simple_interpolate(v3,v7, fractional_z);
	float i4 = simple_interpolate(v4,v8, fractional_z);

	float ii1 = simple_interpolate(i1,i2,fractional_x);
	float ii2 = simple_interpolate(i3,i4,fractional_x);

	return simple_interpolate(ii1 , ii2 , fractional_y);
}


float Noise2D(in vec2 coord, in float wavelength)
{
	return interpolatedNoise2D(coord.x/wavelength, coord.y/wavelength);
}

float Noise3D(in vec3 coord, in float wavelength)
{
	return interpolatedNoise3D(coord.x/wavelength, coord.y/wavelength, coord.z/wavelength);
}

float dotNoise2D(in float x, in float y, in float fractionalMaxDotSize, in float dDensity)
{
	float integer_x    = x - fract(x);
	float fractional_x = x - integer_x;

	float integer_y    = y - fract(y);
	float fractional_y = y - integer_y;

	if (rand2D(vec2(integer_x+1.0, integer_y +1.0)) > dDensity)
	{return 0.0;}

	float xoffset = (rand2D(vec2(integer_x, integer_y)) -0.5);
	float yoffset = (rand2D(vec2(integer_x+1.0, integer_y)) - 0.5);
	float dotSize = 0.5 * fractionalMaxDotSize * max(0.25,rand2D(vec2(integer_x, integer_y+1.0)));


	vec2 truePos = vec2 (0.5 + xoffset * (1.0 - 2.0 * dotSize) , 0.5 + yoffset * (1.0 -2.0 * dotSize));

	float distance = length(truePos - vec2(fractional_x, fractional_y));

	return 1.0 - smoothstep (0.3 * dotSize, 1.0* dotSize, distance);
}

float DotNoise2D(in vec2 coord, in float wavelength, in float fractionalMaxDotSize, in float dDensity)
{
	return dotNoise2D(coord.x/wavelength, coord.y/wavelength, fractionalMaxDotSize, dDensity);
}

float dropletNoise2D(in float x, in float y, in float fractionalMaxDotSize, in float dDensity)
{
	float integer_x    = x - fract(x);
	float fractional_x = x - integer_x;

	float integer_y    = y - fract(y);
	float fractional_y = y - integer_y;

	if (rand2D(vec2(integer_x+1.0, integer_y +1.0)) > dDensity)
	{return 0.0;}

	float xoffset = (rand2D(vec2(integer_x, integer_y)) -0.5);
	float yoffset = (rand2D(vec2(integer_x+1.0, integer_y)) - 0.5);
	float dotSize = 0.5 * fractionalMaxDotSize * max(0.25,rand2D(vec2(integer_x, integer_y+1.0)));

	float x1offset = 2.0 * (rand2D(vec2(integer_x+5.0, integer_y)) -0.5);
	float y1offset = 2.0 * (rand2D(vec2(integer_x, integer_y + 5.0)) - 0.5);
	float x2offset = 2.0 * (rand2D(vec2(integer_x-5.0, integer_y)) -0.5);
	float y2offset = 2.0 * (rand2D(vec2(integer_x-5.0, integer_y -5.0)) - 0.5);
	float smear = (rand2D(vec2(integer_x + 3.0, integer_y)) -0.5);


	vec2 truePos = vec2 (0.5 + xoffset * (1.0 - 4.0 * dotSize) , 0.5 + yoffset * (1.0 -4.0 * dotSize));
	vec2 secondPos = truePos + vec2 (dotSize * x1offset, dotSize * y1offset);
	vec2 thirdPos = truePos + vec2 (dotSize * x2offset, dotSize * y2offset);

	float distance = length(truePos - vec2(fractional_x, fractional_y));
	float dist1 = length(secondPos - vec2(fractional_x, fractional_y));
	float dist2 = length(thirdPos - vec2(fractional_x, fractional_y));	

	return clamp(3.0 - smoothstep (0.3 * dotSize, 1.0* dotSize, distance) - smoothstep (0.3 * dotSize, 1.0* dotSize, dist1) - smoothstep ((0.1 + 0.5 * smear) * dotSize, 1.0* dotSize, dist2), 0.0,1.0);
}

float DropletNoise2D(in vec2 coord, in float wavelength, in float fractionalMaxDotSize, in float dDensity)
{
	return dropletNoise2D(coord.x/wavelength, coord.y/wavelength, fractionalMaxDotSize, dDensity);
}

float voronoiNoise2D(in float x, in float y, in float xrand, in float yrand)
{
	float integer_x    = x - fract(x);
	float fractional_x = x - integer_x;

	float integer_y    = y - fract(y);
	float fractional_y = y - integer_y;

	float val[4];

	val[0] = rand2D(vec2(integer_x, integer_y));
	val[1] = rand2D(vec2(integer_x+1.0, integer_y));
	val[2] = rand2D(vec2(integer_x, integer_y+1.0));
	val[3] = rand2D(vec2(integer_x+1.0, integer_y+1.0));

	float xshift[4];

	xshift[0] = xrand * (rand2D(vec2(integer_x+0.5, integer_y)) - 0.5);
	xshift[1] = xrand * (rand2D(vec2(integer_x+1.5, integer_y)) -0.5);
	xshift[2] = xrand * (rand2D(vec2(integer_x+0.5, integer_y+1.0))-0.5);
	xshift[3] = xrand * (rand2D(vec2(integer_x+1.5, integer_y+1.0))-0.5);

	float yshift[4];

	yshift[0] = yrand * (rand2D(vec2(integer_x, integer_y +0.5)) - 0.5);
	yshift[1] = yrand * (rand2D(vec2(integer_x+1.0, integer_y+0.5)) -0.5);
	yshift[2] = yrand * (rand2D(vec2(integer_x, integer_y+1.5))-0.5);
	yshift[3] = yrand * (rand2D(vec2(integer_x+1.5, integer_y+1.5))-0.5);


	float dist[4];

	dist[0] = sqrt((fractional_x + xshift[0]) * (fractional_x + xshift[0]) + (fractional_y + yshift[0]) * (fractional_y + yshift[0]));
	dist[1] = sqrt((1.0 -fractional_x + xshift[1]) * (1.0-fractional_x+xshift[1]) + (fractional_y +yshift[1]) * (fractional_y+yshift[1]));
	dist[2] = sqrt((fractional_x + xshift[2]) * (fractional_x + xshift[2]) + (1.0-fractional_y +yshift[2]) * (1.0-fractional_y + yshift[2]));
	dist[3] = sqrt((1.0-fractional_x + xshift[3]) * (1.0-fractional_x + xshift[3]) + (1.0-fractional_y +yshift[3]) * (1.0-fractional_y + yshift[3]));

	int i, i_min;
	float dist_min = 100.0;
	for (i=0; i<4;i++)
	{
		if (dist[i] < dist_min)
		{
			dist_min = dist[i];
			i_min = i;
		}
	}

	return val[i_min];
}

float VoronoiNoise2D(in vec2 coord, in float wavelength, in float xrand, in float yrand)	
{
	return voronoiNoise2D(coord.x/wavelength, coord.y/wavelength, xrand, yrand);
}

float slopeLines2D(in float x, in float y, in float sx, in float sy, in float steepness)
{
	float integer_x    = x - fract(x);
	float fractional_x = x - integer_x;

	float integer_y    = y - fract(y);
	float fractional_y = y - integer_y;

	vec2 O = vec2 (0.2 + 0.6* rand2D(vec2 (integer_x, integer_y+1)), 0.3 + 0.4* rand2D(vec2 (integer_x+1, integer_y)));
	vec2 S = vec2 (sx, sy);
	vec2 P = vec2 (-sy, sx);
	vec2 X = vec2 (fractional_x, fractional_y);

	float radius = 0.0 + 0.3  * rand2D(vec2 (integer_x, integer_y));

	float b = (X.y - O.y + O.x * S.y/S.x - X.x * S.y/S.x) / (P.y - P.x * S.y/S.x);
	float a = (X.x - O.x - b*P.x)/S.x;

	return (1.0 - smoothstep(0.7 * (1.0-steepness), 1.2* (1.0 - steepness), 0.6* abs(a))) * (1.0 - smoothstep(0.0, 1.0 * radius,abs(b)));


}


float SlopeLines2D(in vec2 coord, in vec2 gradDir, in float wavelength, in float steepness)
{
	return slopeLines2D(coord.x/wavelength, coord.y/wavelength, gradDir.x, gradDir.y, steepness);
}


float strata3D(in float x, in float y, in float z, in float variation)
{
	float integer_x    = x - fract(x);
	float fractional_x = x - integer_x;

	float integer_y    = y - fract(y);
	float fractional_y = y - integer_y;

	float integer_z = z - fract(z);
	float fractional_z = z - integer_z;

	float rand_value_low = rand3D(vec3(0.0, 0.0, integer_z));
	float rand_value_high = rand3D(vec3(0.0, 0.0, integer_z+1));

	float rand_var = 0.5 - variation + 2.0 * variation * rand3D(vec3(integer_x, integer_y, integer_z));

	return (1.0 - smoothstep(rand_var -0.15, rand_var + 0.15,  fract(z))) * rand_value_low +  smoothstep(rand_var-0.15, rand_var + 0.15, fract(z)) * rand_value_high;
}

float Strata3D(in vec3 coord, in float wavelength, in float variation)
{
	return strata3D(coord.x/wavelength, coord.y/wavelength, coord.z/wavelength, variation);
}

#endif // __NOISE_H__
