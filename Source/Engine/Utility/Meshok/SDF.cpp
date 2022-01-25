// Signed Distance Fields primitives and sampling.
#include "stdafx.h"
#pragma hdrstop

// for SDF::HeightMap
#include <stb/stb_image.h>

#include <algorithm>	// std::swap

#include <Base/Template/Containers/Blob.h>
#include <Meshok/Meshok.h>
#include <Meshok/SDF.h>

namespace SDF
{

/// must be much smaller than the size of a voxel
const int RAY_MARCHING_STEPS = 64;

V3f Isosurface::NormalAt( const V3f& _position ) const
{
	return EstimateNormal( *this, _position.x, _position.y, _position.z );
}

Sample Isosurface::SampleAt( const V3f& _position ) const
{
	Sample result;
	result.normal = this->NormalAt(_position);
	result.distance = this->DistanceTo(_position);
	result.texCoords = V3_Zero();
	result.materialID = ~0;
	return result;
}

void Isosurface::CastRay(
	const RayInfo& input,
	HitInfo &_output
) const
{
	F32 thit;
	bool hit = CastRay_SphereTracing2(
		this,
		input.start,
		input.direction,
		input.length,
		thit
	);
	V3f p = input.start + input.direction * thit;
	_output.normal = this->NormalAt( p );
	_output.time = thit;
	//_output.pos = p;	
	//_output.id = MapTo01(hit);
}

UINT Isosurface::GetAllIntersections(
	const V3f& _start,
	const float _length,
	const V3f& _direction,
	const int _resolution,
	DynamicArray< HitInfo > &_intersections	//!< sorted by distance in ascending order
	//, const int _startPointInSolid /*= false*/
) const
{
	mxASSERT(_intersections.IsEmpty());
	const float cellSize = _length / _resolution;

	bool thisSegmentStartsInSolid = this->IsInside( _start );

	for( int iStep = 0; iStep < _resolution; iStep++ )
	{
		const float distance = cellSize * iStep;
		const V3f cellStart = _start + _direction * distance;
		const V3f cellEnd = cellStart + _direction * cellSize;

		const bool nextSegmentStartsInSolid = this->IsInside( cellEnd );

		if( thisSegmentStartsInSolid != nextSegmentStartsInSolid )
		{
			HitInfo	newHit;

			bool bHit = ( this->IntersectsLine( cellStart, cellEnd, newHit.time, newHit.normal ) );
			if( bHit )
			{
				const float distFromStart = distance + cellSize * newHit.time;
				newHit.time = distFromStart / _length;
				mxASSERT( newHit.time <= 1.0f );
				_intersections.add( newHit );
			}
			//else
			//{
			//	HitInfo	newHit;
			//	_intersections.add( newHit );
			//}
		}
		thisSegmentStartsInSolid = nextSegmentStartsInSolid;
	}

	return _intersections.num();
}

bool Isosurface::IntersectsLine(
	const V3f& _start, const V3f& _end,
	F32 &_time, V3f &_normal
) const
{
#if 0
	const V3f	diff = _end - _start;

	mxTODO("Use lambdas in C++ 11");
	class Wrapper
	{
		const Isosurface &	m_SDF;
		const V3f	m_start;
		const V3f	m_diff;
	public:
		mxFORCEINLINE Wrapper( const Isosurface& _SDF, const V3f& _start, const V3f& _diff )
			: m_SDF( _SDF )
			, m_start( _start )
			, m_diff( _diff )
		{
		}
		mxFORCEINLINE Scalar operator () ( const Scalar _t ) const
		{
			const V3f position = m_start + m_diff * _t;
			return m_SDF.DistanceTo( position );
		}
	};
	const bool intersects = RootFinding::Brent(
		Wrapper( *this, _start, diff ),
		Scalar(0), Scalar(1),
		Scalar(0), 8,
		_time
	);
	if( intersects ) {
		_normal = this->NormalAt( _start + diff * _time );
	}
	return intersects;
#else
	V3f	hitPos;
	const bool intersects = FindIntersection_Bruteforce(
		this,
		_start,
		_end,
		hitPos,
		_time,
		RAY_MARCHING_STEPS
	);
	if( intersects ) {
		_normal = this->NormalAt( hitPos );
	}
	return intersects;
#endif
}

AABBf Isosurface::GetLocalBounds() const
{
	AABBf result;
	result.clear();
	return result;
}

F32 HalfSpace::DistanceTo( const V3f& _point ) const
{
	return Plane_PointDistance( plane, _point );
}
V3f HalfSpace::NormalAt( const V3f& _point ) const
{
	return Plane_GetNormal( plane );
}
Sample HalfSpace::SampleAt( const V3f& _point ) const
{
	mxASSERT(V3_IsNormalized( V4_As_V3( plane ) ));
	Sample _sample;
	_sample.distance = Plane_PointDistance( plane, _point );
	_sample.normal = V4_As_V3( plane );
	return _sample;
}
bool HalfSpace::IntersectsLine(
	const V3f& _start, const V3f& _end,
	F32 &_time, V3f &_normal
) const
{
	bool hit = Plane_LineIntersection( plane, _start, _end, _time );
	_normal = Plane_GetNormal( plane );
	return hit;
}

UINT HalfSpace::GetAllIntersections(
	const V3f& _start,
	const float _length,
	const V3f& _direction,
	const int _resolution,
	DynamicArray< HitInfo > &_intersections	//!< sorted by distance in ascending order
) const
{
	//return Isosurface::GetAllIntersections( _start, _length, _direction, _resolution, _intersections );
	HitInfo	newHit;
	if( Plane_RayIntersection( plane, _start, _direction, newHit.time )
		&& (newHit.time > 0.0f) && (newHit.time < _length) )
	{
		newHit.time /= _length;
		newHit.normal = Plane_GetNormal( plane );
		_intersections.add( newHit );
		return 1;
	}
	return 0;
}

UINT Sphere::GetAllIntersections(
	const V3f& _start,
	const float _length,
	const V3f& _direction,
	const int _resolution,
	DynamicArray< HitInfo > &_intersections	//!< sorted by distance in ascending order
) const
{
//	return Isosurface::GetAllIntersections( _start, _length, _direction, _resolution, _intersections );
	float t[2];
	const int numRoots = RaySphereIntersection( _start, _direction, center, radius, &t[0], &t[1] );

	int numValidRoots = 0;
	for( int iRoot = 0; iRoot < numRoots; iRoot++ )
	{
		const float timeOfHit = t[ iRoot ];
		if( timeOfHit >= 0 && timeOfHit <= _length )
		{
			HitInfo	newHit;
			{
				const V3f hitPos = _start + _direction * timeOfHit;
				newHit.normal = V3_Normalized( hitPos - center );

				newHit.time = timeOfHit / _length;
			}
			_intersections.add( newHit );

			numValidRoots++;
		}
	}
	return numValidRoots;
}

bool Cube::IsInside( const V3f& _point ) const
{
	return this->DistanceTo( _point ) < 0;
}

float Cube::DistanceTo( const V3f& _point ) const
{
	// position relative to the center of the cube
	const V3f relativePosition = _point - center;
	// Measure how far each coordinate is from the center of the cube along that coordinate axis.
	const V3f offsetFromCenter = V3_Abs( relativePosition );	// abs() accounts for symmetry/congruence
	// Outside the cube, each point has at least one coordinate of distance greater than 'half-size',
	// points on the edges have at least one coordinate at distance 'half-size',
	// and points on the interior have all coordinates within 'half-size' of the center.
	// NOTE: distances inside the object are negative, so the maximum of these distances yields the correct result.
	// The Box Approximation: the returned result may be less than the correct Euclidean distance.
	mxBIBREF("Interactive Modeling with Distance Fields, Tim-Christopher Reiner [2010], 3.4.4 Box Approximation, P.20");
	// This SDF returns a Chebyshev distance, which can be less than the Euclidean distance: max( |x|-W/2, |y|-D/2, |z|-H/2 ).
	mxBIBREF("Interactive Modeling of Implicit Surfaces using a Direct Visualization Approach with Signed Distance Functions [2011], 3.3. Describing Primitives, P.3");
	return Max3( offsetFromCenter.x - extent, offsetFromCenter.y - extent, offsetFromCenter.z - extent );
}
bool Box::IsInside( const V3f& _point ) const
{
	const AABBf aabb = { center - extent, center + extent };
	return AABBf_ContainsPoint( aabb, _point );
}
float Box::DistanceTo( const V3f& _position ) const
{
	// position relative to the center of the cube
	const V3f relativePosition = _position - center;
	// Measure how far each coordinate is from the center of the cube along that coordinate axis.
	const V3f offsetFromCenter = V3_Abs( relativePosition );	// abs() accounts for symmetry/congruence
	// Outside the cube, each point has at least one coordinate of distance greater than 'half-size',
	// points on the edges have at least one coordinate at distance 'half-size',
	// and points on the interior have all coordinates within 'half-size' of the center.
	// NOTE: distances inside the object are negative, so the maximum of these distances yields the correct result.
	// The Box Approximation: the returned result may be less than the correct Euclidean distance.
	mxBIBREF("Interactive Modeling with Distance Fields, Tim-Christopher Reiner [2010], 3.4.4 Box Approximation, P.20");
	// This SDF returns a Chebyshev distance, which can be greater than the Euclidean distance: max( |x|-W/2, |y|-D/2, |z|-H/2 ).
	mxBIBREF("Interactive Modeling of Implicit Surfaces using a Direct Visualization Approach with Signed Distance Functions [2011], 3.3. Describing Primitives, P.3");
	return Max3( offsetFromCenter.x - extent.x, offsetFromCenter.y - extent.y, offsetFromCenter.z - extent.z );
mxREMOVE_THIS
//const V3f d = V3_Abs( relativePosition ) - extent;
//const float m = Max(d.x, Max(d.y, d.z));
//return Min(m, V3_Length(V3_Max(d, V3_Zero())));
}
//V3f Box::NormalAt( const V3f& _position ) const
//{
//	const V3f relativePosition = _position - center;
//	const V3f offsetFromCenter = V3_Abs( relativePosition );
//	const int maximumCoordinate = Max3Index( offsetFromCenter.x, offsetFromCenter.y, offsetFromCenter.z );
//	return Axes[maximumCoordinate] * Sign(relativePosition[maximumCoordinate]);
//}
//Sample Box::SampleAt( const V3f& _position ) const
//{
//	const V3f relativePosition = _position - center;
//	const V3f offsetFromCenter = V3_Abs( relativePosition );
//	const int maximumCoordinate = Max3Index( offsetFromCenter.x, offsetFromCenter.y, offsetFromCenter.z );
//	Sample _sample;
//	_sample.normal = Axes[maximumCoordinate] * Sign(relativePosition[maximumCoordinate]);
//	_sample.distance = Max3( offsetFromCenter.x - extent.x, offsetFromCenter.y - extent.y, offsetFromCenter.z - extent.z );
//	_sample.texCoords = GenTexCoords_CubeMapping( _position, extent );
//	return _sample;
//}
void Box::CastRay(
	const RayInfo& input,
	HitInfo &_output
) const
{
//	Isosurface::CastRay(input, _output);
	float t0, t1;
	const int ret = intersectRayAABB( center - extent, center + extent, input.start, input.direction, t0, t1 );
	if( -1 != ret )
	{
		V3f p = input.start + input.direction * t0;
		_output.normal = this->NormalAt( p );
	}
	//_output.id = ret;
}

HeightMap::HeightMap()
{
	m_bounds.clear();
}

ERet HeightMap::Load(
					 const char* _heightmapFile,
					 const AABBf& _terrainBounds,
					 const float _heightScale /*= 1*/,
					 const float _heightBias /*= 0*/
					 )
{
	NwBlob	sourceData(MemoryHeaps::temporary());
	mxDO(NwBlob_::loadBlobFromFile( sourceData, _heightmapFile ));

	const int requiredComponents = STBI_grey;

	int imageWidth, imageHeight;
	int bytesPerPixel;
	unsigned char* imageData = stbi_load_from_memory(
		(stbi_uc*) sourceData.raw(), sourceData.rawSize(),
		&imageWidth, &imageHeight, &bytesPerPixel, requiredComponents
		);
	mxENSURE(imageData, ERR_FAILED_TO_PARSE_DATA, "Failed to load heightmap from '%s'", _heightmapFile);

	mxASSERT(bytesPerPixel == 1);

	const int sizeOfImageData = (imageWidth * imageHeight) * (requiredComponents * sizeof(char));

	LogStream(LL_Info) << "Loaded heightmap '" << _heightmapFile << "', resolution: " << imageWidth << " by " << imageHeight << ", (" << sizeOfImageData << " bytes).";

	mxDO(m_heightMap.setNum( imageWidth * imageHeight ));


	m_resolution.x = imageWidth;
	m_resolution.y = imageHeight;

	m_bounds.min_corner = _terrainBounds.min_corner;
	m_bounds.max_corner = _terrainBounds.max_corner;

	m_bounds.min_corner.z = +BIG_NUMBER;
	m_bounds.max_corner.z = -BIG_NUMBER;

	float heightScale = _heightScale / 255.0f;

	//int maxHeight = 0;
	for( int iRow = 0; iRow < imageHeight; iRow++ )
	{
		for( int iColumn = 0; iColumn < imageWidth; iColumn++ )
		{
			const int index = iRow * imageWidth + iColumn;
			const int sourceValue = imageData[ index ];	// [0..255]
			//maxHeight = Max(maxHeight, sourceValue);
			const float heightValue = sourceValue * heightScale - _heightBias;
			m_heightMap[ index ] = heightValue;

			m_bounds.min_corner.z = minf( m_bounds.min_corner.z, heightValue );
			m_bounds.max_corner.z = maxf( m_bounds.max_corner.z, heightValue );
		}
	}

	stbi_image_free( imageData );


	const V3f terrainSize = m_bounds.size();
	const V2f cellSize(
		terrainSize.x / m_resolution.x,
		terrainSize.y / m_resolution.y
	);

	m_tileSize = cellSize;

	m_invTileSize.x = mmRcp( cellSize.x );
	m_invTileSize.y = mmRcp( cellSize.y );

	return ALL_OK;
}

float HeightMap::GetInterpolatedHeight( float _x, float _y ) const
{
	// Translate world-space x,y coordinates into integer offsets.
	const float relativeX = _x - m_bounds.min_corner.x;
	const float relativeY = _y - m_bounds.min_corner.y;
	const float fTilesX = relativeX * m_invTileSize.x;
	const float fTilesY = relativeY * m_invTileSize.y;
	const float fX0 = ::floorf( fTilesX );
	const float fY0 = ::floorf( fTilesY );
	const int iX0 = (int) fX0;
	const int iY0 = (int) fY0;

	// check range
	if( iX0 < 0 || iX0 >= (int)m_resolution.x ||
		iY0 < 0 || iY0 >= (int)m_resolution.y )
	{
		return -BIG_NUMBER;	// out of bounds
	}

	const int iX1 = Min( iX0 + 1, (int)(m_resolution.x - 1) );
	const int iY1 = Min( iY0 + 1, (int)(m_resolution.y - 1) );

#if 0
	const float height00 = GetRawHeight( iX0, iY0 );	// bottom left
	const float height10 = GetRawHeight( iX1, iY0 );	// bottom right
	const float height01 = GetRawHeight( iX0, iY1 );	// top left
	const float height11 = GetRawHeight( iX1, iY1 );	// top right

	//return height00;

	const float dX = (relativeX - iX0 * m_tileSize.x) * m_invTileSize.x;	// how the height changes with respect to x
	const float dY = (relativeY - iY0 * m_tileSize.y) * m_invTileSize.y;	// how the height changes with respect to y
	mxASSERT(dX <= 1 && dY <= 1);

	// We first do linear interpolation in the x-direction.
	const float lower = height00 * dX + height10 * (1.0f - dX);
	const float upper = height01 * dX + height11 * (1.0f - dX);

	// We proceed by interpolating in the y-direction.
	//return lower * dY + upper * (1 - dY);
	return lower * (1.0f - dY) + upper * dY;	//which is right?
#else

	// What triangle are we in? (top left or bottom right)
	//
	//     Y ^
	//       |
	//(X0,Y1) ____ (X1,Y1)
	//       |   /|
	//       |  / |
	//       | /  |
	//       |/___| - - - -> X
	// (X0,Y0)  (X1,Y0)

	const float fractionalPartX = fTilesX - fX0;
	const float fractionalPartY = fTilesY - fY0;
	//mxASSERT(fractionalPartX >= -1e-4f && fractionalPartX <= 1);
	//mxASSERT(fractionalPartY >= -1e-4f && fractionalPartY <= 1);

	if( fractionalPartX > fractionalPartY ) {
		// in top left triangle
		const float height00 = GetRawHeight( iX0, iY0 );	// bottom left
		const float height10 = GetRawHeight( iX1, iY0 );	// bottom right
		//const float height01 = GetRawHeight( iX0, iY1 );	// top left
		const float height11 = GetRawHeight( iX1, iY1 );	// top right
		return height00 + fractionalPartX * (height10 - height00) + fractionalPartY * (height11 - height10);
	} else {
		// in bottom right triangle
		const float height00 = GetRawHeight( iX0, iY0 );	// bottom left
		//const float height10 = GetRawHeight( iX1, iY0 );	// bottom right
		const float height01 = GetRawHeight( iX0, iY1 );	// top left
		const float height11 = GetRawHeight( iX1, iY1 );	// top right
		return height00 + fractionalPartX * (height11 - height01) + fractionalPartY * (height01 - height00);
	}
#endif
}

namespace CSG
{
	mxBIBREF("Realtime Volume Rendering Aimed at Terrain: http://www.volume-gfx.com/volume-rendering/density-source/constructive-solid-geometry/operations/");

	F32 Negation::DistanceTo( const V3f& _position ) const
	{
		const F32 d = O->DistanceTo(_position);
		return -d;
	}
	Sample Negation::SampleAt( const V3f& _position ) const
	{
		Sample _sample = O->SampleAt( _position );
		_sample.distance = -_sample.distance;
		_sample.normal = V3_Negate(_sample.normal);
		return _sample;
	}

	bool Union::IsInside( const V3f& _point ) const
	{
		return A->IsInside(_point) || B->IsInside(_point);
	}
	F32 Union::DistanceTo( const V3f& _position ) const
	{
		const F32 dA = A->DistanceTo(_position);
		const F32 dB = B->DistanceTo(_position);
		// The distance to a list of implicit surfaces is the smallest
		// distance originating from their respective distance functions.
		return minf( dA, dB );
		//return ( dA < dB ) ? dA : dB;
	}
	Sample Union::SampleAt( const V3f& _position ) const
	{
		const Sample sA = A->SampleAt( _position );
		const Sample sB = B->SampleAt( _position );
		if( sA.distance < sB.distance ) {
			return sA;	// e.g. if point B is outside
		} else {
			return sB;
		}
	}

	bool Difference::IsInside( const V3f& _point ) const
	{
		return A->IsInside(_point) && !B->IsInside(_point);
	}
	F32 Difference::DistanceTo( const V3f& _position ) const
	{
		const F32 dA = A->DistanceTo(_position);
		const F32 dB = B->DistanceTo(_position);
#if 1
		return maxf( dA, -dB );
#else
		// if point B is outside, return A
		return ( dB > 0.0f ) ? dA : -dB;
#endif
	}
	Sample Difference::SampleAt( const V3f& _position ) const
	{
		const Sample sA = A->SampleAt( _position );
		const Sample sB = B->SampleAt( _position );
#if 1
		if( sA.distance > -sB.distance ) {
			return sA;	// e.g. if point B is outside
		} else {
			Sample _sample( sB );
			_sample.normal = -sB.normal;
			_sample.distance = -sB.distance;
			return _sample;
		}
#else
		if( sB.distance > 0 ) {
			return sA;	// outside B => return A
		} else {
			Sample _sample( sB );
			_sample.normal = -sB.normal;
			_sample.distance = -sB.distance;
			return _sample;
		}
#endif
	}

	F32 UnionMulti::DistanceTo( const V3f& _position ) const
	{
		F32 minimumDistance = BIG_NUMBER;
		for( UINT i = 0; i < this->arguments.num(); i++ )
		{
			const Isosurface* O = this->arguments[i];
			const F32 distance = O->DistanceTo(_position);
			if( distance < minimumDistance ) {
				minimumDistance = distance;
			}
		}
		return minimumDistance;
	}
	Sample UnionMulti::SampleAt( const V3f& _position ) const
	{
		F32 minimumDistance = BIG_NUMBER;
		Sample result;
		for( UINT i = 0; i < this->arguments.num(); i++ )
		{
			const Isosurface* O = this->arguments[i];
			const Sample sample = O->SampleAt(_position);
			if( sample.distance < minimumDistance ) {
				minimumDistance = sample.distance;
				result = sample;
			}
		}
		return result;
	}

}//namespace CSG

}//namespace SDF


namespace SDF
{

V3f GenTexCoords_CubeMapping(
	const V3f& _position,
	const V3f& _halfSize
)
{
	return V3_Multiply( _position, _halfSize );
}
V3f GenTexCoords_SphereMapping(
	const V3f& _position,
	const V3f& _halfSize
)
{
	const V2f& xy = V3_As_V2( _position );
	UNDONE;
	return V3_Zero();
	//V3f result = {
	//};
	//return V3_Multiply( _position, _halfSize );
	//return TexCoord(rounds*vec.Teta()/PIx2, rounds*vec.Phi()/PIx2, 0);
}

// see: Numerical Methods for Ray Tracing Implicitly Defined Surfaces
// http://graphics.williams.edu/courses/cs371/f14/reading/implicit.pdf
mxBIBREF("Numerical Methods for Ray Tracing Implicitly Defined Surfaces");
mxBIBREF("GPU Ray Marching of Distance Fields, Lukasz Jaroslaw Tomczak, 2012, P.10, 2.2.1 Ray marching");
mxBIBREF("K. Perlin and E. M. Hoffert. Hypertexture. SIGGRAPH Comput. Graph., 23(3):253–262, July 1989.");
bool FindIntersection_Bruteforce(
	const Isosurface* _SDF,
	const V3f& _start,
	const V3f& _end,
	V3f &_hitPoint,
	F32 &_timeHit,
	int numSteps
	)
{
#define LOCAL_EPSILON 1e-4f

	// A simple ray-marching algorithm that finds zero-crossings of the given implicit surface.

	const V3f diff = _end - _start;
	const F32 length = V3_Length( diff );
	mxASSERT(length > LOCAL_EPSILON);
	const F32 invLength = mmRcp(length);
	const F32 stepSize = length / numSteps;
	const V3f rayDir = V3_Multiply( diff, V3_SetAll(invLength) );

	F32 currentTime = 0.0f;	// [0..length]
	F32 minimumTime = 0.0f;	// [0..length]
	F32 minimumDist = BIG_NUMBER;
	for( int iteration = 0; iteration < numSteps; iteration++ )
	{
		const V3f currentPoint = _start + rayDir * currentTime;
		const F32 distance = _SDF->DistanceTo( currentPoint );
		const F32 d = mmAbs( distance );
		if( d < minimumDist )
		{
			minimumDist = d;
			minimumTime = currentTime;
		}
		currentTime += stepSize;
	}

#undef LOCAL_EPSILON

	_hitPoint = _start + rayDir * minimumTime;
	_timeHit = minimumTime * invLength;

	return _timeHit >= 0.0f && _timeHit <= 1.0f;
}

mxBIBREF("J.C. Hart. Sphere tracing: A geometric method for the antialiased ray tracing of implicit surfaces. The Visual Computer, 12(10):527-545, 1996");
mxBIBREF("GPU Ray Marching of Distance Fields, Lukasz Jaroslaw Tomczak, 2012, P.14, 2.2.2 Sphere tracing");
bool CastRay_SphereTracing(
						   const SDF::Isosurface* _SDF,
						   const V3f& _start,
						   const V3f& _end,
						   F32 &_time01
						   )
{
#define LOCAL_EPSILON 1e-4f

	enum { MAX_STEPS = 32 };	// prevent infinite looping
	const V3f diff = _end - _start;
	const F32 length = V3_Length( diff );
	mxASSERT(length > LOCAL_EPSILON);
	const F32 invLength = mmRcp(length);
	const V3f rayDir = V3_Multiply( diff, V3_SetAll(invLength) );

	F32 currentTime = 0.0f;
	for( U32 iteration = 0; iteration < MAX_STEPS; iteration++ )
	{
		const V3f currentPoint = _start + rayDir * currentTime;
		const F32 distance = _SDF->DistanceTo( currentPoint );
		if( distance < LOCAL_EPSILON ) {
			break;	// including cases when distance < 0 (i.e. the ray starts inside solid)
		}
		if( currentTime > length ) {
			return false;
		}
		currentTime += distance;
	}

#undef LOCAL_EPSILON

	_time01 = currentTime * invLength;

	return true;
}

bool CastRay_SphereTracing2(
	const Isosurface* _SDF,
	const V3f& _start,
	const V3f& _rdir,
	const F32 _rmax,
	F32 &_tWhenHit
)
{

#define LOCAL_EPSILON 1e-4f

	mxASSERT(V3_IsNormalized(_rdir));
	mxASSERT(_rmax > LOCAL_EPSILON);

	F32 currentTime = 0.0f;
	while( currentTime < _rmax )
	{
		const V3f currentPoint = _start + _rdir * currentTime;
		const F32 distance = _SDF->DistanceTo( currentPoint );
		if( distance < LOCAL_EPSILON ) {
			break;	// including cases when distance < 0 (i.e. inside solid)
		}
		currentTime += distance;
	}

#undef LOCAL_EPSILON

	_tWhenHit = currentTime;

	return currentTime >= 0.0f && currentTime < _rmax;
}

void RayTraceImage(
	const Isosurface* _surface,
	const V3f& camera_position_in_world_space,
	const V3f& _cameraForward,
	const V3f& _cameraRight,
	const V3f& _cameraUp,
	const F32 _farPlane,
	const F32 _halfFoVy,
	const F32 _aspect,
	const U32 _sizeX,
	const U32 _sizeY,
	float* _imagePtr,
	U32 _startY,
	U32 _endY
)
{
	mxASSERT_PTR(_surface);
	mxASSERT(V3_IsNormalized(_cameraForward));
	mxASSERT(V3_IsNormalized(_cameraRight));
	mxASSERT(V3_IsNormalized(_cameraUp));
	mxASSERT(_farPlane > 0);
	mxASSERT(_halfFoVy > 0);
	mxASSERT(_aspect > 0);
	mxASSERT(_sizeX > 0);
	mxASSERT(_sizeY > 0);
	_endY = smallest( _endY, _sizeY );
	mxASSERT(_startY < _endY);

	// For each ray, compute its corresponding point on the image plane.
	// The ray direction can be found by taking the difference
	// between the image plane position and the eye.

	mxBIBREF("Raymarching Distance Fields: http://9bitscience.blogspot.ru/2013/07/raymarching-distance-fields_14.html");

	// I prefix constant vectors with 'c' and varyings with 'v'.
	const V3f cRayFrom = camera_position_in_world_space;
	const V3f cForward = _cameraForward;
	const V3f cRight = _cameraRight;
	const V3f cUp = _cameraUp;

	// distance from eye position to the image plane
	const float imagePlaneH = _farPlane * tanf(_halfFoVy);	// half-height of the image plane
	const float imagePlaneW = imagePlaneH * _aspect;	// half-width of the image plane

	// precompute a bunch of constants; hoist as much as possible out of the inner ray-casting loop
	const V3f cRightScaled = cRight * imagePlaneW;
	const V3f cUpScaled = cUp * -imagePlaneH;	// (-1) because screen Y-axis goes UP, while image Y goes down

	const V3f cDeltaX = cRightScaled * (2.0f / (float)_sizeX);
	const V3f cDeltaY = cUpScaled * (2.0f / (float)_sizeY);

	const V3f cStartX = -cRightScaled;
	const V3f cStartY = -cUpScaled;

	V3f vCurrentY = cStartX + cStartY + (cForward * _farPlane) + cRayFrom;

	//if( _startY != 0 )// ensured by assert
	{
		_imagePtr = _imagePtr + ( _startY * _sizeX );
		vCurrentY += cDeltaY * (float)_startY;
	}

	// For each row...
	for( U32 iY = _startY; iY < _endY; iY++ )
	{
		V3f vRayTo = vCurrentY;

		// and For each column...
		for( U32 iX = 0; iX < _sizeX; iX++ )
		{
			float time = 1.0f;
#if 1
			SDF::CastRay_SphereTracing( _surface, cRayFrom, vRayTo, time );
#elif 0
			V3f hitNormal;
			_surface->IntersectsLine( cRayFrom, vRayTo, time, hitNormal );
#elif 0
			float length;
			V3f rayDir = V3_Normalized( vRayTo - cRayFrom, length );
			SDF::CastRay_SphereTracing2( _surface, cRayFrom, rayDir, length, time );
			time /= length;
#else
			RayInfo input;
			input.start = cRayFrom;
			input.direction = V3_Normalized( vRayTo - cRayFrom, input.length );

			HitInfo output;
			_surface->CastRay( input, output );

			output.time /= input.length;
			time = output.time;
#endif
			*_imagePtr++ = 1.0f - time;

			vRayTo += cDeltaX;
		}

		vCurrentY += cDeltaY;
	}
}

/*
@todo: Try some isosurface equations for testing from "Appendix A: Reference Implicits",
"Fast Ray Tracing of Arbitrary Implicit Surfaces with Interval and Affine Arithmetic"(2009)
*/
namespace Testing
{
	// it would be faster with function pointers/templates,
	// but some noise generators maintain state (and must be initialized)
	const SDF::Isosurface* GetTestScene( const V3f& sceneSize )
	{
#if 0
		SDF::HeartCurve	heart;
		return &hills;
#endif
#if 0	// BUG WITH MESH DECIMATION - OVERLAPPING TRIANGLES
		static Smile smile;
		smile.scale = V3_SetAll(0.05);
		return &smile;
#endif
#if 0
		static KleinBottle kleinBottle;	// NOISY, NOT ENOUGH PRECISION
		//kleinBottle.scale = V3_Reciprocal(sceneSize) * 4.0f;
		kleinBottle.scale = V3_SetAll(0.01);//1.0f/32.0f);
		return &kleinBottle;
#endif
#if 0
		// SELF-INTERSECTIONS WHEN USING STANDARD DC SIMPLIFICATIONS - GOOD TEST CASE FOR MANIFOLD DC SIMPLIFICATION!
		static Sinusoid sinusoid;
		sinusoid.Initialize( 8, 16 );
		return &sinusoid;
#endif
#if 0
		// SELF-INTERSECTIONS WHEN USING STANDARD DC SIMPLIFICATIONS - GOOD TEST CASE FOR MANIFOLD DC SIMPLIFICATION!
		static Sinusoid sinusoid;
		sinusoid.Initialize( 8, 16 );

		//const V3f boxSize = sceneSize * 0.5f;
		const V3f boxSize = sceneSize * 32.0f;

		static Box	box;
		box.center = boxSize * 0.5f;
		box.extent = boxSize * 0.49f;

		static CSG::Difference difference;
		difference.A = &box;
		difference.B = &sinusoid;
		return &difference;
#endif
#if 0	// NOISY
		static Gyroid gyroid;
		return &gyroid;
#endif
#if 0
		static Torus torus;
		torus.center = sceneSize * 0.5f;
		torus.diameter = sceneSize.x * 0.4f;
		torus.thickness = sceneSize.x * 0.2f;
		return &torus;
#endif
#if 0
		static SineRidges sines;
		return &sines;	
#endif

#if 0	// SIMPLEST TEST CASE FOR POLYGON REDUCTION
		static HalfSpace ground;

//		ground.plane = Plane_FromPointNormal(sceneSize * 4.0f, V3_UP);

//		ground.plane = Plane_FromPointNormal(sceneSize * 0.0f, V3_UP);

//		ground.plane = Plane_FromPointNormal(sceneSize * 9.5f, V3_UP);

//		ground.plane = Plane_FromPointNormal(sceneSize * 0.5f, V3_Normalized(V3_Set(-1,-1,-1)));
		ground.plane = Plane_FromPointNormal(sceneSize * 0.5f, V3_Normalized(V3_Set(1,1,1)));

//		ground.plane = Plane_FromPointNormal(sceneSize * 0.5f, V3_Normalized(V3_Set(-1,0,0)));

//		ground.plane = Plane_FromPointNormal(sceneSize * 0.5f, V3_Normalized(V3_Set(0,0,1)));
		return &ground;
#endif

#if 0	//TEST SEAMS - sceneSize*0.17f
		static SDF::SineRidges sines;
return &sines;
		static SDF::Box	box1;
		box1.center = V3_Set( sceneSize.x, sceneSize.y, sceneSize.z );
		box1.extent = V3_Set( sceneSize.x * 2.6, sceneSize.y * 2.6, sceneSize.z * 2.5 );

		static SDF::CSG::Difference difference2;
		difference2.A = &sines;
		difference2.B = &box1;
		return &difference2;
#endif

#if 0	// SIMPLE TEST CASE FOR POLYGON REDUCTION / QEF simplification
		{
			static Box	box;
			box.center = sceneSize * 0.5f;
			box.extent = sceneSize * 0.49f;
			return &box;
		}
#endif
#if 0
		static PerturbedSphere sphere;
		sphere.radius = sceneSize.x * 0.8f;
		sphere.a = 1.0f;
		sphere.b = 0.5f;
		return &sphere;
#endif
#if 0	// fully solid
		{
			static Sphere	sphere;
			sphere.radius = 9999.f;
			return &sphere;
		}
#endif
#if 0	// first octant
		{
			static Sphere	sphere;
			sphere.radius = sceneSize.x * 0.7f;
			return &sphere;
		}
#endif
#if 0	// LARGE SPHERE
		{
			static Sphere	sphere;
			//sphere.radius = 16 * 16.0f;	// 5 clipmap levels
			sphere.radius = 8 * 8.0f;
			//sphere.radius = sceneSize.x * 1.3f;
			return &sphere;
		}
#endif
#if 0	// GOOD FOR TESTING SPHERE-RAY INTERSECTOR AND QUANTIZED HERMITE DATA
		{
			static Sphere	sphere;
			sphere.center = sceneSize * 0.2f;
			sphere.radius = sceneSize.x * 0.9f;
//sphere.center = sceneSize * 0.9f;
//sphere.radius = sceneSize.x * 1.2f;
			return &sphere;
		}
#endif
#if	0	//SPHERE
		{
			static Sphere	sphere;
//			sphere.center = sceneSize * 0.5f;
			sphere.radius = sceneSize.x * 0.45f;
			return &sphere;
		}
#endif
#if 0	// SIMPLEST TEST CASE FOR POLYGON REDUCTION / QEF simplification
		{
			static Box	box;
//			box.center = sceneSize * 0.5f;
			box.extent = sceneSize * 0.4f;
			return &box;
		}
#endif
#if 0	// FOR TESTING SEAMS
		{
			static Box	box;
			box.center = sceneSize * 0.5f;
			box.extent = V3_Multiply( sceneSize, V3_Set( 10, 0.4f, 0.4f ) );
			return &cylinder;
		}
#endif
#if 0	// FOR TESTING SEAMS
		{
			static Cylinder	cylinder;
			cylinder.ray_origin = sceneSize * 0.5f;
			cylinder.ray_direction = V3_Set(1,0,0);
			//cylinder.ray_direction = V3_Set(0,1,0);

			cylinder.radius = sceneSize.x * 0.4f;
			cylinder.height = sceneSize.z * 0.4f;
			return &cylinder;
		}
#endif


#if 1	// FOR TESTING SEAMS (GROUND PLANE) [0+)
		{
			static HalfSpace ground;
			ground.plane = Plane_FromPointNormal(-sceneSize * 0.3f, V3_UP);

			const V3f center = -sceneSize * 0.3f;

			static Sphere	sphere;
			sphere.center = center;
			sphere.radius = sceneSize.x * 0.3f;

			static CSG::Difference difference;
			difference.A = &ground;
			difference.B = &sphere;
			//return &difference;

			static CSG::Union union2;

			static Box	box;
			box.center = center;
			box.extent = V3_Set(sceneSize.x * 0.05f, sceneSize.y * 0.05f, sceneSize.z * 0.9f);

			union2.A = &difference;
			union2.B = &box;
			//return &union2;

			static Torus torus;
			torus.center = center + V3_Set(0, 0, sceneSize.z * 0.3f);
			torus.diameter = sceneSize.x * 0.45f;
			torus.thickness = sceneSize.x * 0.04f;
			//return &torus;

			static CSG::Union union3;
			union3.A = &union2;
			union3.B = &torus;
			return &union3;
		}
#endif

#if 0	// FOR TESTING SEAMS (GROUND PLANE)
		{
			//static Cube	box;
			//box.center = sceneSize * 0.5f;
			//box.extent = sceneSize.x * 0.4f;
			static Box	box;
			box.center = sceneSize * 0.5f;
			box.extent = sceneSize * 0.4f;

			static Sphere	sphere;
			sphere.center = sceneSize * 0.5f;
			sphere.center.z = sceneSize.z * 0.7f;

			sphere.radius = sceneSize.x * 0.3f;

			static CSG::Difference difference;
			difference.A = &box;
			difference.B = &sphere;
			return &difference;			
		}
#endif

#if 0
		// TESTING SHADOWS
		{
			static CSG::Union union2;

			static Box	box;
			box.center = V3_SetAll(50);
			box.extent = V3_Set(10, 10, 200);

			static HalfSpace ground;
			ground.plane = Plane_FromPointNormal(V3_Zero(), V3_UP);
			union2.A = &box;
			union2.B = &ground;
			return &union2;
		}
#endif

		// BUG - SHARP FEATURES!
#if 0	//+ FOR TESTING SEAMS - GOOD CASE FOR ADAPTIVE DC (OCTREE CELLS)
		{
			static Box	box;
			box.center = sceneSize * 0.5f;
			box.extent = sceneSize * 0.3f;
			box.extent.x *= 10;

			static Sphere	sphere;
			sphere.center = sceneSize * 0.5f;
			sphere.radius = sceneSize.x * 0.35f;

			static Cylinder	cylinder;
			cylinder.ray_origin = sceneSize * 0.5f;
			cylinder.ray_direction = V3_Set(0,1,0);
			cylinder.radius = sceneSize.x * 0.2f;
			cylinder.height = sceneSize.y * 0.5f;

			static CSG::Difference difference;
			difference.A = &box;
			difference.B = &sphere;
			difference.B = &cylinder;
			return &difference;			
		}
#endif
#if 0	// BOX - BOX
		{
			static Box	box;
			box.center = sceneSize * 0.5f;
			box.extent = sceneSize * 0.4f;

			static Box	box2;
			box2.center = sceneSize * 0.4f;
			box2.extent = sceneSize * 0.2f;
			box2.extent.x = sceneSize.x * 0.4f;

			static CSG::Difference difference;
			difference.A = &box;
			difference.B = &box2;
			return &difference;
		}
#endif
#if 0	//+ BUG UNDER SIMPLIFICATION: NOTCHES
		{
			static Box	box;
			box.center = sceneSize * 0.5f;
			box.extent = sceneSize * 0.3f;
			box.extent.x *= 10;

			static Sphere	sphere;
			sphere.center = sceneSize * 0.5f;
			sphere.radius = sceneSize.x * 0.4f;

			static CSG::Difference difference;
			difference.A = &box;
			difference.B = &sphere;
			return &difference;
		}
#endif
#if 0	//SUBTRACT CUBE FROM SPHERE: BUG UNDER SIMPLIFICATION: NOTCHES
		{
			static Box	box;
			box.center = sceneSize * 0.5f;
			box.extent = sceneSize * 0.3f;
			box.extent.x *= 10;

			static Sphere	sphere;
			sphere.center = sceneSize * 0.5f;
			sphere.radius = sceneSize.x * 0.43f;

			static Sphere	sphere2;
			sphere2.center = sceneSize * 0.3f;
			sphere2.radius = sceneSize.x * 0.3f;

			static CSG::Difference difference;
			difference.A = &sphere;
			difference.B = &box;
			//TSwap(difference.A, difference.B);
			//difference.A = &sphere;
			//difference.B = &sphere2;
			return &difference;
		}
#endif
#if 0	// FOR TESTING SEAMS
		{
			static Slab	slab;
			//slab.plane = Plane_FromPointNormal( V3_Set(sceneSize.x*0.5f,10,100), V3_Set(1,0,0));
			slab.plane = Plane_FromPointNormal( sceneSize*0.5f, V3_Set(0,0,1));
			slab.thickness = sceneSize.x * 0.2f;
			//return &slab;

			static Sphere	sphere;
			sphere.center = sceneSize * 0.5f;
			sphere.radius = sceneSize.x * 0.3f;
//sphere.radius = sceneSize.x * 0.4f;
			//return &sphere;

			static CSG::Difference difference;
			difference.A = &slab;
			difference.B = &sphere;
			return &difference;
		}
#endif
#if 0 // GOOD TEST CASE FOR SEAMS (UPPER PART IS SPLIT)
		{
			static Box	box;
			box.center = sceneSize * 0.5f;
			box.extent = sceneSize*0.42f;

			static Sphere	sphere;
			sphere.center = box.center;
			sphere.center.z *= 1.5f;
			sphere.radius = box.extent.x * 1.3f;

			static CSG::Blend blend;
			blend.A = &box;
			blend.B = &sphere;
			return &blend;
		}
#endif
#if 0 // THE SAME VERSION AS ABOVE, ONLY A BIT LARGER
		{
			static Box	box;
			box.center = sceneSize * 0.5f;
			box.extent = sceneSize*0.7f;

			static Sphere	sphere;
			sphere.center = box.center;
			sphere.center.z *= 1.5f;
			sphere.radius = box.extent.x * 1.5f;

			static CSG::Blend blend;
			blend.A = &box;
			blend.B = &sphere;
			return &blend;
		}
#endif
#if 0	// VERY UGLY - JAGGIES! SUBTRACT CUBE FROM SPHERE
		{
			static Sphere	sphere;
			sphere.center = sceneSize * 0.5f;
			sphere.radius = sceneSize.x * 0.45f;

			static Box	box;
			box.center = sceneSize * 0.5f;
			box.extent = sceneSize*0.3f;

			static CSG::Difference difference;
			difference.A = &sphere;
			difference.B = &box;
			return &difference;			
		}
#endif

#if 0	// too big to fit inside a chunk; visible seams
		{
			static Sphere	sphere;
			sphere.radius = sceneSize.x * 1.1;
			return &sphere;
		}
#endif
#if 0
		{
			static Box	thinBox;
			thinBox.center = sceneSize * 0.5f;
			thinBox.extent = V3_Multiply( sceneSize, V3_Set(0.5f,0.05f,0.5f) );
			return &thinBox;
		}
#endif
#if 0
		{
			static Box	box;
			box.center = sceneSize * 0.5f;
			//box.extent = V3_Multiply( sceneSize, V3_Set(0.4f,0.4f,2.0f) );
			box.extent = V3_Multiply( sceneSize, V3_Set(0.3f,0.3f,0.4f) );
			return &box;
		}
#endif

#if 0	// QUATER of INF CYLINDER, LOOKS OK UNDER SIMPLIFICATION
		{
			static Cylinder	cylinder;
			cylinder.ray_origin = sceneSize * 0.5f;
			cylinder.ray_direction = V3_Set(1,0,0);
			cylinder.radius = sceneSize.x * 0.4f;
			cylinder.height = sceneSize.z * 0.4f;
//return &cylinder;

			static CSG::RotateZ	rotate;
			rotate.O = &cylinder;
			rotate.angle = DEG2RAD(45);

			return &rotate;
		}
#endif
#if 0
		{
			static Cube	box;
			box.center = sceneSize * 0.5f;
			box.extent = sceneSize.x*0.4f;
//return &box;
			static CSG_Rotate	rotate;
			rotate.O = &box;
			rotate.matrix = M33_RotationZ(DEG2RAD(45));

			return &rotate;
		}
#endif
#if 0
		{
			static Box	box;
			box.center = sceneSize * 0.5f;
			box.extent = sceneSize*0.3f;

			static CSG_Rotate	rotate;
			rotate.O = &box;
			//rotate.angle = DEG2RAD(45);			
			rotate.matrix = M33_RotationZ(DEG2RAD(45));

			return &rotate;
		}
#endif
#if 0
		{
			static Sphere	sphere;
			sphere.center = sceneSize * 0.5f;
			sphere.radius = sceneSize.x * 0.4f;
return &sphere;
			static CSG_Translate	translate;
			translate.op = &sphere;
			translate.offset = V3_Set(0,0,-200);
			return &translate;
		}
#endif
#if 0	//PROBLEMATIC CASE FOR DUAL CONTOURING
		// Hourglass - separated at the thinnest part; good for testing manifold Dual Contouring
		{
			static ConeSDF	cone;
			cone.ray_origin = sceneSize * 0.5f;
			cone.radius = sceneSize.x * 0.5f;
			return &cone;
		}
#endif
#if 0	//PROBLEMATIC CASE FOR DUAL CONTOURING
		// WIDE 'HOURGLASS' - LOOKS VERY BAD UNDER VERTEX CLUSTERING/OCTREE-BASED SIMPLIFICATION
		{
			static ConeSDF	cone;
			cone.ray_origin = sceneSize * 0.5f;
			cone.radius = sceneSize.x * 1.5f;
			return &cone;
		}
#endif
#if 0
		// FINITE CYLINDER - ALL OK!
		{
			static Cylinder	cylinder;
			cylinder.ray_origin = sceneSize * 0.5f;
			cylinder.radius = sceneSize.x * 0.3f;
			cylinder.height = sceneSize.z * 0.2f;
			return &cylinder;
		}
#endif
#if 0
		{
			static Box	box;
			box.center = sceneSize * 0.5f;
			box.extent = sceneSize*0.3f;

			static InfiniteCylinder	cylinder;
			cylinder.ray_origin = box.center;
			cylinder.radius = box.extent.x * 0.7f;

			static CSG::Blend blend;
			blend.A = &box;
			blend.B = &cylinder;
			blend.blendA = 0.7;
			blend.blendB = 0.3;
			return &blend;
		}
#endif

#if 0	// GOOD TEST FOR MANIFOLD DC
		{
			static Box	box;
			box.center = sceneSize * 0.3f;
			box.extent = sceneSize * 0.2f;

			static Sphere	sphere;
			sphere.center = sceneSize * 0.5f;
			sphere.radius = sceneSize.x * 0.2f;

			static CSG::Union union1;
			union1.A = &box;
			union1.B = &sphere;
//			return &union1;

			static InfiniteCylinder	cylinder;
			cylinder.ray_origin = sceneSize * 0.8f;
			cylinder.radius = sceneSize.x * 0.2f;
			cylinder.ray_direction = V3_Set(0,1,0);

			static CSG::Union union2;
			union2.A = &union1;
			union2.B = &cylinder;
			return &union2;
		}
#endif


#if 1	// LARGE; JAGGIEZ!
		{
			static CSG::Union union2;

			{
				static CSG::Difference difference;
				{
					static CSG::Union union1;
					{
						static CSG::Difference diff1;
						{
							static Box	outer;
							outer.center = sceneSize * 0.5f;
							outer.extent = sceneSize*0.40f;

							static Box	inner;
							inner.center = sceneSize * 0.5f;
							inner.extent = sceneSize*0.32f;

							diff1.A = &outer;
							diff1.B = &inner;
						}
						static CSG::Difference diff2;
						{
							static Box	inner2;
							inner2.center = sceneSize * 0.5f;
							inner2.extent = sceneSize*0.25f;

							static Box	inner3;
							inner3.center = sceneSize * 0.5f;
							inner3.extent = inner2.extent * 0.7f;

							diff2.A = &inner2;
							diff2.B = &inner3;
						}
						union1.A = &diff1;
						union1.B = &diff2;
					}
//return &union1;
					difference.A = &union1;

					static Sphere	sphere;
					sphere.center = V3_Set(sceneSize.x*0.1f, sceneSize.y*0.1f, sceneSize.z*0.6f);
					sphere.radius = sceneSize.x/2;
					difference.B = &sphere;
				}
				//return &difference;


				//static CSG::CSG_Rotate rotate;
				//{
				//	static Box	thinHorizontalBox;
				//	thinHorizontalBox.center = sceneSize * 0.5f;
				//	thinHorizontalBox.extent = V3_Multiply( sceneSize, V3_Set(0.5f,0.5f,0.1f) );
				//	//return &thinHorizontalBox;

				//	rotate.O = &thinHorizontalBox;
				//	rotate.matrix = M33_RotationPitchRollYaw(0.4, 0.9, 1.6);
				//}

				//union2.A = &difference;
				//union2.B = &rotate;


				static HalfSpace ground;
				ground.plane = Plane_FromPointNormal(V3_Zero(), V3_UP);

				union2.A = &difference;
				union2.B = &ground;
			}
			return &union2;

			//static InfiniteCylinder cylinder;
			//cylinder.ray_origin = sceneSize * 0.5f;
			//cylinder.ray_direction = V3_Set(0,1,0);
			//cylinder.radius = V3_Length(sceneSize)*0.1f;

			//static Slab	slab;
			////slab.plane = Plane_FromPointNormal( V3_Set(sceneSize.x*0.5f,10,100), V3_Set(1,0,0));
			//slab.plane = Plane_FromPointNormal( sceneSize*0.5f, V3_Set(0,0,1));
			//slab.thickness = sceneSize.x * 0.2f;
			////return &slab;

		}
#endif

#if 0	// ONE BOX INSIDE ANOTHER
		{
			static CSG::Union union2;
			{
				static CSG::Union union1;
				{
					static CSG::Difference diff1;
					{
						static Box	outer;
						outer.center = sceneSize * 0.5f;
						outer.extent = sceneSize*0.4f;

						static Box	inner;
						inner.center = sceneSize * 0.5f;
						inner.extent = sceneSize*0.32f;

						diff1.A = &outer;
						diff1.B = &inner;
					}

					static CSG::Difference diff2;
					{
						static Box	inner2;
						inner2.center = sceneSize * 0.5f;
						inner2.extent = sceneSize*0.24f;

						static Box	inner3;
						inner3.center = sceneSize * 0.5f;
						inner3.extent = inner2.extent * 0.8f;

						diff2.A = &inner2;
						diff2.B = &inner3;
					}

					union1.A = &diff1;
					union1.B = &diff2;
				}

				//static Sphere	sphere;
				//sphere.center = V3_Set(470, 270, 470);
				//sphere.radius = 270;

				//static CSG::Difference difference3;
				//difference3.A = &diff1;
				//difference3.B = &sphere;

				static HalfSpace ground;
				ground.plane = Plane_FromPointNormal(V3_Zero(), V3_UP);

				union2.A = &union1;
				union2.B = &ground;
			}


//static InfiniteCylinder cylinder;
//cylinder.ray_origin = sceneSize * 0.5f;
//cylinder.ray_direction = V3_Normalized(V3_Set(0, sceneSize.y*0.1f, sceneSize.z) - cylinder.ray_origin);
//cylinder.radius = V3_Length(sceneSize)*0.2f;

static InfiniteCylinder cylinder;
cylinder.ray_origin = sceneSize * 0.5f;
cylinder.ray_direction = V3_Set(0, 1, 0);
cylinder.radius = V3_Length(sceneSize)*0.15f;


			static CSG::Difference difference3;
			difference3.A = &union2;
			difference3.B = &cylinder;

			return &difference3;
		}
#endif

#if 0	// COMPLEX
		{
			static CSG::Difference difference;
			{
				static CSG::Union union1;
				{
					static CSG::Difference diff1;
					{
						static Box	outer;
						outer.center = sceneSize * 0.5f;
						outer.extent = sceneSize*0.4f;

						static Box	inner;
						inner.center = sceneSize * 0.5f;
						inner.extent = sceneSize*0.32f;

						diff1.A = &outer;
						diff1.B = &inner;
					}

					static CSG::Difference diff2;
					{
						static Box	inner2;
						inner2.center = sceneSize * 0.5f;
						inner2.extent = sceneSize*0.24f;

						static Box	inner3;
						inner3.center = sceneSize * 0.5f;
						inner3.extent = inner2.extent * 0.8f;

						diff2.A = &inner2;
						diff2.B = &inner3;
					}

					union1.A = &diff1;
					union1.B = &diff2;
				}

				static InfiniteCylinder cylinder;
				cylinder.ray_origin = sceneSize * 0.5f;
				cylinder.radius = V3_Length(sceneSize)*0.1f;

				difference.A = &union1;
				difference.B = &cylinder;
			}
//return &difference;
			static Sphere	sphere;
			sphere.center = V3_Set(sceneSize.x/2.5f, sceneSize.y/6, sceneSize.z/2.5f);
			sphere.radius = sceneSize.x/5;


//static ConeSDF	cone;
////cone.ray_origin = V3_Set(sceneSize.x*0.5f, sceneSize.y*0.5f, sceneSize.z*0.3f);
//cone.ray_origin = sceneSize*0.5f;
//cone.ray_direction = V3_Normalized(V3_Set(0, sceneSize.y*0.1f, sceneSize.z) - cone.ray_origin);
//cone.angle = DEG2RAD(45);
////cone.radius = sceneSize.x * 1.0f;
//
//static InfiniteCylinder cylinder;
//cylinder.ray_origin = sceneSize * 0.5f;
//cylinder.ray_direction = V3_Normalized(V3_Set(0, sceneSize.y*0.1f, sceneSize.z) - cylinder.ray_origin);
//cylinder.radius = V3_Length(sceneSize)*0.1f;


			static CSG::Difference difference3;
			difference3.A = &difference;
			difference3.B = &sphere;//&cone;
			//difference3.B = &cylinder;//&cone;

			return &difference3;
		}
#endif



#if 1	// MECH PART
		{
			static CSG::Difference difference;
			{
				static CSG::Union union1;
				{
					static CSG::Difference diff1;
					{
						static Box	outer;
						outer.center = sceneSize * 0.5f;
						outer.extent = sceneSize*0.4f;

						static Box	inner;
						inner.center = sceneSize * 0.5f;
						inner.extent = sceneSize*0.32f;

						diff1.A = &outer;
						diff1.B = &inner;
					}

					static CSG::Difference diff2;
					{
						static Box	inner2;
						inner2.center = sceneSize * 0.5f;
						inner2.extent = sceneSize*0.24f;

						static Box	inner3;
						inner3.center = sceneSize * 0.5f;
						inner3.extent = inner2.extent * 0.8f;

						diff2.A = &inner2;
						diff2.B = &inner3;
					}

					union1.A = &diff1;
					union1.B = &diff2;
				}

				static InfiniteCylinder cylinder;
				cylinder.ray_origin = sceneSize * 0.5f;
				cylinder.radius = V3_Length(sceneSize)*0.1f;
				//cylinder.ray_origin = V3_UP;

				difference.A = &union1;
				difference.B = &cylinder;
			}
			//return &difference;

static InfiniteCylinder cylinder2;
cylinder2.ray_origin = sceneSize * 0.7f + sceneSize * 0.15f;
cylinder2.ray_direction = V3_FORWARD;
cylinder2.radius = V3_Length(sceneSize)*0.2f;

			static CSG::Difference difference3;
			difference3.A = &difference;
			difference3.B = &cylinder2;

			return &difference3;
		}
#endif



#if 0
		{
			static HalfSpace halfSpace;
			halfSpace.plane = Plane_FromPointNormal( V3_Set(100,10,100), V3_Set(0,-1,0));

			static Sphere	sphere;
			sphere.center = V3_Set(480, 140, 474);
			sphere.radius = 250;

			static CSG::Difference difference2;
			difference2.A = &halfSpace;
			difference2.B = &sphere;

			return &difference2;
		}
#endif

#if 0
		{
			static Box	box1;
			box1.center = sceneSize * 0.5f;
			box1.extent = V3_Multiply( sceneSize, V3_Set(0.42f,0.4f,0.3f) );

			static Box	box2;
			box2.center = sceneSize * 0.5f;
			box2.extent = box1.extent * 0.8f;

			static CSG::Difference difference1;
			difference1.A = &box1;
			difference1.B = &box2;

			static Sphere	sphere;
			sphere.center = V3_Set(480, 140, 474);
			sphere.radius = 250;

			static CSG::Difference difference2;
			difference2.A = &difference1;
			difference2.B = &sphere;

			return &difference2;
		}

		{
			static Box	box;
			box.center = sceneSize * 0.5f;
			box.extent = V3_Multiply( sceneSize, V3_Set(0.42f,0.4f,0.3f) );
			return &box;
		}

		{
			static Cube	cube;
			cube.center = sceneSize * 0.5f;
			cube.extent = sceneSize.x * 0.4f;
			return &cube;
		}

		{
			static Sphere	sphere;
			sphere.center = V3_Set(256,256,256);
			sphere.radius = 150;
			return &sphere;
		}

		{
			static Box	box;
			box.center = sceneSize * 0.5f;
			box.extent = sceneSize*0.4f;
			return &box;
		}

		//return &aaboxSDF;

		//return &planeSDF;

		{
			const float CENTER_X = sceneSize.x*0.2;

			static Box	box;
			box.center = V3_Set(CENTER_X,CENTER_X,CENTER_X);
			box.extent = sceneSize*0.9f;
			box.extent.x = sceneSize.x * 0.2f;

			static CSG::Difference subtract;
			subtract.A = &box;
			{
				static Sphere	sphere;
				sphere.center = V3_Set(CENTER_X,256,256);
				sphere.radius = 150;
				subtract.B = &sphere;
			}
			return &subtract;
		}
#endif

	}
}//namespace Testing
}//namespace SDF


namespace VX
{

bool SDF_Sampler::IsInside(
	int iCellX, int iCellY, int iCellZ
) const
{
	const V3f cornerXYZ = V3f::set( iCellX, iCellY, iCellZ );
	const V3f cellCorner = m_offset + V3_Multiply( cornerXYZ, m_cellSize );mxTEMP
	return m_isosurface.IsInside( cellCorner );
}

U32 SDF_Sampler::SampleHermiteData(
	int iCellX, int iCellY, int iCellZ,
	HermiteDataSample & _sample
) const
{
	V3f corners[8];	// cell corners in world space
	UINT signMask = 0;
	for( UINT iCorner = 0; iCorner < 8; iCorner++ )
	{
		const int iCornerX = ( iCorner & MASK_POS_X ) ? iCellX+1 : iCellX;
		const int iCornerY = ( iCorner & MASK_POS_Y ) ? iCellY+1 : iCellY;
		const int iCornerZ = ( iCorner & MASK_POS_Z ) ? iCellZ+1 : iCellZ;
		const V3f cornerXYZ = V3f::set( iCornerX, iCornerY, iCornerZ );
		const V3f cellCorner = m_offset + V3_Multiply( cornerXYZ, m_cellSize );
mxTEMP
		const bool cornerIsInside = m_isosurface.IsInside( cellCorner );
		signMask |= (cornerIsInside << iCorner);

		corners[iCorner] = cellCorner;
	}
	if( vxIS_ZERO_OR_0xFF(signMask) )
	{
		return signMask;
	}

	int numWritten = 0;

UNDONE;
	//U32 edgeMask = CUBE_ACTIVE_EDGES[ signMask ];
	//while( edgeMask )
	//{
	//	U32	iEdge;
	//	edgeMask = TakeNextTrailingBit32( edgeMask, &iEdge );

	//	const UINT iCornerA = CUBE_EDGE[iEdge][0];
	//	const UINT iCornerB = CUBE_EDGE[iEdge][1];

	//	const V3f& cornerA = corners[iCornerA];
	//	const V3f& cornerB = corners[iCornerB];

	//	F32 intersectionTime;
	//	const bool intersects = m_isosurface.IntersectsLine(
	//		cornerA, cornerB,
	//		intersectionTime, _sample.normals[ numWritten ]
	//	);
	//	mxASSERT(intersects);
	//	if( intersects ) {
	//		_sample.positions[ numWritten ] = cornerA + (cornerB - cornerA) * intersectionTime;
	//		numWritten++;
	//	}
	//}

	_sample.num_points = numWritten;

	return signMask;
}

}//namespace VX

/*

An Implicit Surface Polygonizer, by Jules Bloomenthal [1994]

More Implicit Surfaces:
http://www.lama.univ-savoie.fr/~lachaud/Research/Digital-surfaces-and-singular-surfaces/body.html
http://mathworld.wolfram.com/QuarticSurface.html

Implicit Surfaces Seminar, Spring 2012
University of Waterloo Technical Report CS-2013-08
https://cs.uwaterloo.ca/sites/ca.computer-science/files/uploads/files/CS-2013-08.pdf


Extending the CSG Tree. Warping, Blending and Boolean Operations in an Implicit Surface Modeling System [1999]
DOI: 10.1111/1467-8659.00365
*/

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
