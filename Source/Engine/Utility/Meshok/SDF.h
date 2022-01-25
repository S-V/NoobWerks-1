// Signed Distance Field primitives and sampling.
#pragma once

#include <Base/Template/Containers/Array/TInplaceArray.h>
#include <Utility/Meshok/Volumes.h>


namespace SDF
{
	/// prefer doubles for more precision (the overhead is negligible compared to virtual function calls)
	//typedef double Scalar;
	/// but floats are faster
	typedef float Scalar;

	typedef AABBf Bounds;

	/// input parameters for the SampleAt() function
	struct Sample
	{
		V3f	normal;		//!< geometric normal, points outward
		F32	distance;	//!< positive if outside, negative if inside
		V3f	texCoords;	//!< texture coordinates
		U32	materialID;	//!< user-defined
	};

	/// input parameters for the CastRay() function
	struct RayInfo
	{
		V3f	start;		//!< ray origin
		F32 length;		//!< max distance along the ray
		V3f	direction;	//!< normalized direction of the ray
	};
	struct HitInfo
	{
		V3f	normal;	//!< valid only if there was an intersection
		F32	time;	//!< fraction of the ray at intersection, [0..1]
		//V3f	pos;	//!< intersection point (if any)
		//int	id;	//!< id of the hit primitive (e.g. box side, triangle index)
	};

	/// input parameters for the GetAllIntersections() function
	struct TraceParameters
	{
		V3f	start;		//!< ray origin
		F32 length;		//!< max distance along the ray
		V3f	direction;	//!< normalized direction of the ray
		int	resolution;	//!< number of steps (cells) along the ray
//		int	startSolid;	//!<
	};

	/// Abstract base class for implicit objects:
	/// Signed Distance Field / Volume / Density Source interface.
	struct Isosurface
	{
		/// Ideally, this should return the shortest scalar distance to the surface from the given point.
		/// But Euclidean distance fields are expensive to compute, and instead various piecewise approximations are used.
		/// It's important that those approximations do not overestimate the distance.
		/// The distance is positive if the point is outside the solid and negative, if inside.
		virtual F32 DistanceTo( const V3f& _point ) const = 0;

		/// Is the given point inside or outside w.r.t. the implicit function?
		/// The result should be the same as (scalar_distance < 0), but it can be implemented more efficiently.
		virtual bool IsInside( const V3f& _point ) const
		{
			return this->DistanceTo(_point) < 0;
		}

	public:	// The following functions can be overridden for better efficiency:

		/// default implementation estimates the gradient at the given point (normal points outward)
		virtual V3f NormalAt( const V3f& _point ) const;

		/// estimates the density and gradient at the given point
		virtual Sample SampleAt( const V3f& _point ) const;

		// Ray casting can be faster than sphere-tracing based on scalar valued distance computations (where we have to search in *all* directions).

		virtual void CastRay(
			const RayInfo& input,
			HitInfo &_output
		) const;

		// Line-segment intersection testing.

		// NOTE: must not miss any intersections, including 'back-facing' ones.
		// Returns an intersection even if the start point is inside the solid.
		// The returned distance must always be within [0..1].
		virtual bool IntersectsLine(
			const V3f& _start, const V3f& _end,
			F32 &_time, V3f &_normal
		) const;

		//NOTE: assumes that the _intersections array is initially empty
		virtual UINT GetAllIntersections(
			const V3f& _start,
			const float _length,
			const V3f& _direction,
			const int _resolution,	//!< number of steps (cells) along the ray
			DynamicArray< HitInfo > &_intersections	//!< sorted by distance in ascending order
			//, const int _startPointInSolid = false	//HACK for building ray-rep; assume the ray starts in air
		) const;

		/// should be computed once and cached
		/// default implementation returns an infinitely large box
		virtual AABBf GetLocalBounds() const;

	protected:
		virtual ~Isosurface() {}
	};

}//namespace SDF


// Distance-based primitives
namespace SDF
{
	// 'Infinite solid plane'
	struct HalfSpace : Isosurface
	{
		V4f	plane;
	public:
		HalfSpace()
		{
			plane = V4f::set( V3_UP, 0.0f );
		}

		virtual F32 DistanceTo( const V3f& _point ) const override;

		virtual V3f NormalAt( const V3f& _point ) const override;

		virtual Sample SampleAt( const V3f& _point ) const override;

		virtual bool IntersectsLine(
			const V3f& _start, const V3f& _end,
			F32 &_time, V3f &_normal
		) const override;

		virtual UINT GetAllIntersections(
			const V3f& _start,
			const float _length,
			const V3f& _direction,
			const int _resolution,
			DynamicArray< HitInfo > &_intersections	//!< sorted by distance in ascending order
		) const override;
	};

	// 'Thick plane' - solid volume between two planes.
	struct Slab : Isosurface
	{
		V4f	plane;
		F32	thickness;	//!< it's actually half the thickness
	public:
		Slab()
		{
			plane = V4_Zero;
			thickness = 1.0f;
		}
		virtual bool IsInside( const V3f& _point ) const
		{
			//return this->UnsignedDistanceTo( _point ) <= thickness;
			return Plane_ContainsPoint( plane, _point, thickness );
		}
		virtual F32 DistanceTo( const V3f& _point ) const override
		{
			return this->UnsignedDistanceTo( _point ) - thickness;
		}
		virtual V3f NormalAt( const V3f& _point ) const override
		{
			const V3f& N = Plane_GetNormal( plane );
			const F32 distance = Plane_PointDistance( plane, _point );
			return distance >= 0.0f ? N : -N;
		}
		virtual Sample SampleAt( const V3f& _point ) const override
		{
			Sample _sample;
			_sample.distance = this->DistanceTo( _point );
			_sample.normal = this->NormalAt( _point );
			return _sample;
		}
		inline float UnsignedDistanceTo( const V3f& _point ) const
		{
			return mmAbs( Plane_PointDistance( plane, _point ) );	// abs() accounts for symmetry
		}
	};

	/// A implicit sphere given its center and its radius: dist(x) = |x| - r.
	struct Sphere : Isosurface, Sphere16
	{
	public:
		Sphere()
		{
			center = V3_Zero();
			radius = 1.0f;
		}
		virtual bool IsInside( const V3f& _point ) const
		{
			return V3_LengthSquared( _point - center ) <= (radius * radius);
		}
		virtual F32 DistanceTo( const V3f& _point ) const override
		{
			return DistanceBetween( center, _point ) - radius;
		}
		virtual V3f NormalAt( const V3f& _point ) const override
		{
			return V3_Normalized( _point - center );
		}
		virtual Sample SampleAt( const V3f& _point ) const override
		{
			const V3f relative_position = _point - center;
			const F32 length = V3_Length( relative_position );
			Sample _sample;
			_sample.distance = length - radius;
			_sample.normal = relative_position / length;
			return _sample;
		}

		virtual UINT GetAllIntersections(
			const V3f& _start,
			const float _length,
			const V3f& _direction,
			const int _resolution,
			DynamicArray< HitInfo > &_intersections	//!< sorted by distance in ascending order
		) const override;

		virtual AABBf GetLocalBounds() const override
		{
			return AABBf::fromSphere( center, radius );
		}
	};

	/// for CSG subtraction
	struct InvertedSphere : Isosurface, Sphere16
	{
	public:
		InvertedSphere()
		{
			center = V3_Zero();
			radius = 1.0f;
		}
		virtual bool IsInside( const V3f& _point ) const
		{
			return V3_LengthSquared( _point - center ) > (radius * radius);
		}
		virtual F32 DistanceTo( const V3f& _point ) const override
		{
			return radius - DistanceBetween( center, _point );
		}
		virtual V3f NormalAt( const V3f& _point ) const override
		{
			return V3_Normalized( center - _point );
		}
		virtual Sample SampleAt( const V3f& _point ) const override
		{
			const V3f relative_position = _point - center;
			const F32 length = V3_Length( relative_position );
			Sample _sample;
			_sample.distance = radius - length;
			_sample.normal = relative_position / length;
			return _sample;
		}
		virtual AABBf GetLocalBounds() const override
		{
			return AABBf::fromSphere( center, radius );
		}
	};

	// http://math.stackexchange.com/questions/639722/is-there-a-real-continuous-function-of-a-cube
	// http://jsfiddle.net/yLcrX/
	struct Cube : Isosurface
	{
		V3f	center;
		F32	extent;	//!< half-size, must be always positive
	public:
		Cube()
		{
			center = V3_Zero();
			extent = 1;
		}
		virtual bool IsInside( const V3f& _point ) const;
		virtual float DistanceTo( const V3f& _point ) const override;

		virtual AABBf GetLocalBounds() const override
		{
			return AABBf::fromSphere( center, extent * mxSQRT_3 );
		}
	};
	/// Axis-Aligned Box
	struct Box : Isosurface
	{
		V3f	center;
		V3f	extent;	//!< half-size, must be always positive
	public:
		Box()
		{
			center = V3_Zero();
			extent = V3_SetAll(1);
		}
		virtual bool IsInside( const V3f& _point ) const;
		virtual float DistanceTo( const V3f& _point ) const override;
		mxBUG("wrong:")
		//virtual V3f NormalAt( const V3f& _point ) const override;
		//virtual Sample SampleAt( const V3f& _point ) const override;
		virtual void CastRay(
			const RayInfo& input,
			HitInfo &_output
		) const override;
	};

	struct InfiniteCylinder : Isosurface
	{
		V3f	ray_origin;
		V3f	ray_direction;
		F32	radius;	//!< must be always positive
	public:
		InfiniteCylinder()
		{
			ray_origin = V3_Zero();
			ray_direction = V3_Set(0,0,1);
			radius = 1;
		}
		virtual float DistanceTo( const V3f& _point ) const override
		{
			const float distance = Ray_PointDistance( ray_origin, ray_direction, _point );
			mxBIBREF("Interactive Modeling with Distance Fields, Tim-Christopher Reiner [2010], 3.4.5 Cylinder, Eq.3.5, P.22");
			return mmAbs(distance) - radius;
		}
		virtual V3f NormalAt( const V3f& _point ) const override
		{
			const V3f closest_point_on_axis = Ray_ClosestPoint( ray_origin, ray_direction, _point );
			return V3_Normalized( _point - closest_point_on_axis );
		}
		virtual Sample SampleAt( const V3f& _point ) const
		{
			const V3f closest_point_on_axis = Ray_ClosestPoint( ray_origin, ray_direction, _point );
			const V3f diff = _point - closest_point_on_axis;
			const F32 distance = V3_Length( diff );
			Sample _sample;
			_sample.normal = diff / distance;
			_sample.distance = mmAbs(distance) - radius;
			_sample.texCoords = V3_Zero();
			return _sample;
		}
	};

	/// capped cylinder
	struct Cylinder : Isosurface
	{
		V3f		ray_origin;
		V3f		ray_direction;
		F32		radius;	//!< must be always positive
		F32		height;	//!< half-height, starting from the origin
	public:
		Cylinder()
		{
			ray_origin = V3_Zero();
			ray_direction = V3_UP;
			radius = 1;
			height = 1;
		}
		virtual float DistanceTo( const V3f& _point ) const override
		{
			const V3f closest_point_on_axis = Ray_ClosestPoint( ray_origin, ray_direction, _point );
			const F32 distance_to_axis = DistanceBetween( closest_point_on_axis, _point );
			const V3f relative_position = _point - ray_origin;
			const F32 distance_to_cap = mmAbs(relative_position.z) - height;
			mxBIBREF("Interactive Modeling with Distance Fields, Tim-Christopher Reiner [2010], 3.4.5 Cylinder, Eq.3.5, P.22");
			return maxf( mmAbs(distance_to_axis) - radius, distance_to_cap );
		}
	};

	struct Torus : Isosurface
	{
		V3f	center;
		F32	diameter;	//!< outer diameter
		F32	thickness;	//!< torus thickness
	public:
		Torus()
		{
			center = V3_Zero();
			diameter = 1.0f;
			thickness = 1.0f;
		}
		virtual F32 DistanceTo( const V3f& _point ) const override
		{
			V3f p = _point - center;
			V2f q = V2_Set(
				V2_Length(V2_Set(p.x,p.y)) - diameter,
				p.z
			);
			return V2_Length(q) - thickness;
		}
	};

	/// Infinite cone
	struct DoubleConeSDF : public Isosurface
	{
		V3f	ray_origin;
		V3f	ray_direction;
		F32	angle;	//!< cone angle around Z axis, must be always positive
	public:
		DoubleConeSDF()
		{
			ray_origin = V3_Zero();
			ray_direction = V3_Set(0,0,1);
			angle = DEG2RAD(45);
		}
		virtual float DistanceTo( const V3f& _point ) const override
		{
			mxBUG("poor precision");
			const V3f closest_point_on_axis = Ray_ClosestPoint( ray_origin, ray_direction, _point );
			const F32 distance_to_axis = DistanceBetween( closest_point_on_axis, _point );
			F32 sinAngle, cosAngle;
			mmSinCos( angle, sinAngle, cosAngle );
			//const V3f relative_position = _point - ray_origin;
			mxBIBREF("Interactive Modeling with Distance Fields, Tim-Christopher Reiner [2010], 3.4.6 Cone, Eq.3.6, P.23");
			return distance_to_axis * cosAngle - mmAbs(closest_point_on_axis.z) * sinAngle;
		}
	};

	struct ConeSDF : public Isosurface
	{
		V3f	ray_origin;
		V3f	ray_direction;
		F32	radius;	//!< must be always positive
		F32	height;	//!< half-height, starting from the origin
		F32	angle;	//!< cone angle around Z axis, must be always positive
	public:
		ConeSDF()
		{
			ray_origin = V3_Zero();
			ray_direction = V3_Set(0,0,1);
			radius = 1;
			height = 1;
			angle = DEG2RAD(45);
		}
		virtual float DistanceTo( const V3f& _point ) const override
		{
			const F32 distance_to_axis = Ray_PointDistance( ray_origin, ray_direction, _point );
			F32 sinAngle, cosAngle;
			mmSinCos( angle, sinAngle, cosAngle );
			const V3f relative_position = _point - ray_origin;
			mxBIBREF("Interactive Modeling with Distance Fields, Tim-Christopher Reiner [2010], 3.4.6 Cone, Eq.3.6, P.23");
			return distance_to_axis * cosAngle - mmAbs(relative_position.z) * sinAngle;
		}
	};

	/// A heart-shaped surface given by the sextic equation:
	/// (x^2+9/4y^2+z^2-1)^3-x^2z^3-9/(80)y^2z^3=0
	/// Taubin, G. "An Accurate Algorithm for Rasterizing Algebraic Curves and Surfaces." IEEE Comput. Graphics Appl. 14, 14-23, 1994.
	struct HeartCurve : Isosurface
	{
		V3f scale;
	public:
		HeartCurve()
		{
			scale = V3_SetAll(1);
		}
		virtual F32 DistanceTo( const V3f& _point ) const override
		{
			const V3f scaled = V3_Multiply( _point, scale );
			float x = scaled.x;
			float y = scaled.y;
			float z = scaled.z;
			float xx = x * x;
			float yy = y * y;
			float zz = z * z;
			float zzz = zz * z;
			return cubef( xx + (9.0f/4.0f) * yy + zz - 1) - xx * zzz - (9.0f / 80.0f) * yy * zzz;
		}
	};

	/// "Smile"/"Saddle" surface from "Robust adaptive meshes for implicit surfaces"[2006].
	struct Smile : Isosurface
	{
		V3f scale;
	public:
		Smile()
		{
			scale = V3_SetAll(1);
		}
		virtual F32 DistanceTo( const V3f& _point ) const override
		{
			const V3f scaled = V3_Multiply( _point, scale );
			float x = scaled.x;
			float y = scaled.y;
			float z = scaled.z;
			float xx = x * x;
			float yy = y * y;
			float zz = z * z;
			return FourthPower( y - xx - yy + 1 ) + FourthPower( xx + yy + zz ) - 1;
		}
	};

	/// The Klein bottle is a closed nonorientable surface of Euler characteristic 0 (Dodson and Parker 1997, p. 125)
	/// that has no inside or outside, originally described by Felix Klein (Hilbert and Cohn-Vossen 1999, p. 308).
	/// (x^2+y^2+z^2+2y-1)[(x^2+y^2+z^2-2y-1)^2-8z^2] +16xz(x^2+y^2+z^2-2y-1)=0 
	/// good for testing thin iso-surface ray casting and extraction.
	struct KleinBottle : Isosurface
	{
		V3f scale;	//!< the smaller the scale, the larger the surface
	public:
		KleinBottle()
		{
			scale = V3_SetAll(1);
		}
		virtual F32 DistanceTo( const V3f& _point ) const override
		{
			const V3f scaled = V3_Multiply( _point, scale );
			float x = scaled.x;
			float y = scaled.y;
			float z = scaled.z;
			float xx = x * x;
			float yy = y * y;
			float zz = z * z;
			float y2 = y * 2.0f;
			float sum = xx + yy + zz;
			float S = sum - y2 - 1;
			return (sum + y2 - 1) * mmAbs(squaref(S) - 8*zz) + (16 * x * z * S);
		}
	};

	/// A repeating porous structure. Good for testing chunk seams.
	struct Gyroid : Isosurface
	{
		V3f scale;
	public:
		Gyroid()
		{
			scale = V3_SetAll(1);
		}
		virtual F32 DistanceTo( const V3f& _point ) const override
		{
			const V3f scaled = V3_Multiply( _point, scale );
			float sx, cx, sy, cy, sz, cz;
			mmSinCos( scaled.x, sx, cx );
			mmSinCos( scaled.y, sy, cy );
			mmSinCos( scaled.z, sz, cz );
			return 2.0 * (cx * sy + cy * sz + cz * sx);
		}
	};

	/// http://0fps.net/2012/07/07/meshing-minecraft-part-2/
	/// Good for testing chunk seams.
	class Sinusoid : public Isosurface
	{
		float	s;	//!< precomputed
		float	frequency;	// [0..10] are good values
		int		resolution;
	public:
		Sinusoid()
		{
			this->Initialize( 5, 32 );
		}
		void Initialize( float _frequency, int _resolution )
		{
			this->frequency = _frequency;
			this->resolution = _resolution;
			this->s = (0.5f * _frequency) * (mxPI / _resolution);
		}
		virtual F32 DistanceTo( const V3f& _point ) const override
		{
			float x = _point.x;
			float y = _point.y;
			float z = _point.z;
			return sinf( this->s * x ) + sinf( this->s * y ) + sinf( this->s * z );
		}
	};

	class Cosines : public Isosurface
	{
		float	s;	//!< precomputed
		float	frequency;	// [0..10] are good values
		int		resolution;
	public:
		Cosines()
		{
			this->Initialize( 5, 32 );
		}
		void Initialize( float _frequency, int _resolution )
		{
			this->frequency = _frequency;
			this->resolution = _resolution;
			this->s = (0.5f * _frequency) * (mxPI / _resolution);
		}
		virtual F32 DistanceTo( const V3f& _point ) const override
		{
			float x = _point.x;
			float y = _point.y;
			float z = _point.z;
			return cosf( this->s * x ) + cosf( this->s * y ) + cosf( this->s * z );
		}
	};


	/// concentric 'mountains'
	struct SineRidges : public Isosurface
	{
		V3f	scale;
	public:
		SineRidges()
		{
			scale = CV3f(1.0f);
		}
		virtual F32 DistanceTo( const V3f& _point ) const override
		{
			const V3f scaled = V3_Multiply( _point, scale );
			float height = sinf( sqrtf( squaref(scaled.x) + squaref(scaled.y) ) );
			return scaled.z - height;
		}
	};

	/// infinite terrain with rolling hills
	struct SineHills2D : SDF::Isosurface
	{
		V3f	scale;
	public:
		SineHills2D()
		{
			scale = CV3f(1.0f);
		}
		virtual F32 DistanceTo( const V3f& _point ) const override
		{
			const V3f scaled = V3_Multiply( _point, scale );
			float sum = sinf( scaled.x ) + sinf( scaled.y );	// [-2..+2]
			sum *= 0.5f;	// [-1..+1]
			sum += 1.0f;	// [0..1]
			return scaled.z*0.8f - sum;
		}
	};

	// http://users.csc.calpoly.edu/~zwood/teaching/csc570/final06/mebarry/images/normal_mapped_large.jpg
	struct PerturbedSphere : Isosurface, Sphere16
	{
		float	a, b;
	public:
		PerturbedSphere()
		{
			center = V3_Zero();
			radius = 1.0f;
			a = 1.0;
			b = 7.4;
		}
		virtual F32 DistanceTo( const V3f& _point ) const override
		{
			const V3f p = _point - center;
			return V3_Length( p )
				+ a * ( cosf( b * p.x ) + cosf( b * p.y ) + cosf( b * p.z ) ) - radius;
				;
		}
	};

	//(pow(x, 4) + pow(y, 4) + pow(z, 4) - 1.5 * (x * x + y * y + z * z) + 1)

#if 0
	struct FractalNoise : Volume
	{
		int octaves;
		float frequency;
		float lacunarity;
		float persistence;
	public:
		FractalNoise()
		{
			center = V3_Zero();
			T = V2_Set(1.0f, 1.0f);
		}
		virtual F32 DistanceTo( const V3f& _point ) const override
		{
			const float SCALE = 1.f / 128.f;
			V2f p = V2_Set(_point.x, _point.y) * SCALE;
			float noise = 0.f;

			float amplitude = 1.f;
			p *= frequency;

			for (int i = 0; i < octaves; i++)
			{
				noise += simplex(p) * amplitude;
				p *= lacunarity;	
				amplitude *= persistence;	
			}

			// move into [0, 1] range
			return 0.5f + (0.5f * noise);
		}
	};
#endif


	/// 'elevation data source'
	class HeightMap : public SDF::Isosurface
	{
		/// The elevation data
		TArray< F32 >	m_heightMap;	//!< The whole terrain height map
		UInt2			m_resolution;	//!< {width, height}
		AABBf			m_bounds;		//!< world-space bounds of the whole terrain
		/// precomputed: the size of a single terrain tile and its inverse
		V2f		m_tileSize;
		V2f		m_invTileSize;

	public:
		HeightMap();

		ERet Load(
			const char* _heightmapFile,
			const AABBf& _terrainBounds,
			const float _heightScale = 1,	//!< Scale factor in the Z domain
			const float _heightBias = 0		//!< Offset to apply after scaling
		);

		float GetRawHeight( U32 iX, U32 iY ) const
		{
			const U32 index = iY * m_resolution.x + iX;
			mxASSERT( index < m_heightMap.num() );
			return m_heightMap[ index ];
		}

		/*! Scale factor in the Z domain */
		float GetInterpolatedHeight( float _x, float _y ) const;

		virtual F32 DistanceTo( const V3f& _point ) const override
		{
			return _point.z - this->GetInterpolatedHeight( _point.x, _point.y );
		}
	};


	struct UnaryOperation : Isosurface
	{
		TPtr< const Isosurface >	O;

	public:
		//virtual void GetBounds( Bounds &_bounds ) const override
		//{
		//	O->GetBounds( _bounds );
		//}
	};

	struct BinaryOperation : Isosurface
	{
		TPtr< const Isosurface >	A;
		TPtr< const Isosurface >	B;

	public:
		//virtual void GetBounds( Bounds &_bounds ) const override
		//{
		//	Bounds boundsA, boundsB;
		//	A->GetBounds( _bounds );
		//	B->GetBounds( _bounds );
		//	AABBf_Merge( boundsA, boundsB, _bounds );
		//}
	};

	struct OperationWithManyArguments : Isosurface
	{
		TInplaceArray< const Isosurface*, 8 >	arguments;
	};

	struct Transform : UnaryOperation
	{
		M44f	localToWorld;	//!< world matrix
		M44f	worldToLocal;	//!< inverse of the above matrix ('world-to-local' transform)
	public:
		Transform()
		{
			localToWorld = M44_Identity();
			worldToLocal = M44_Identity();
		}
		virtual F32 DistanceTo( const V3f& _point ) const override
		{
			// world space => our local space
			const V3f localPosition = M44_TransformPoint( worldToLocal, _point );
			return O->DistanceTo( localPosition );
		}
		virtual V3f NormalAt( const V3f& _point ) const override
		{
			const V3f localPosition = M44_TransformPoint( worldToLocal, _point );
			const V3f localNormal = O->NormalAt( localPosition );
			return V3_Normalized( M44_TransformNormal( localToWorld, localNormal ) );
		}
	};

	//@todo: Boy's surface

	// Constructive Solid Geometry
	namespace CSG
	{
		/// this can be used to 'invert' the operand inside-out for boolean subtraction
		struct Negation : UnaryOperation
		{
		public:
			virtual F32 DistanceTo( const V3f& _point ) const override;
			virtual Sample SampleAt( const V3f& _point ) const override;
		};

		/// Union: A + B.
		/// dist( A + B ) = min( dist(A), dist(B) ).
		struct Union : BinaryOperation
		{
		public:
			virtual bool IsInside( const V3f& _point ) const override;
			virtual F32 DistanceTo( const V3f& _point ) const override;
			virtual Sample SampleAt( const V3f& _point ) const override;
		};

		/// Difference: A - B
		/// dist( A - B ) = max( dist(A), -dist(B) ).
		struct Difference : BinaryOperation
		{
		public:
			virtual bool IsInside( const V3f& _point ) const override;
			virtual F32 DistanceTo( const V3f& _point ) const override;
			virtual Sample SampleAt( const V3f& _point ) const override;

			//virtual bool DirectedDistance(
			//	const V3f& _start,
			//	const float _length,
			//	const V3f& _direction,
			//	F32 &_distance,
			//	V3f &_normal
			//) const;
		};

		struct UnionMulti : OperationWithManyArguments
		{
		public:
			virtual F32 DistanceTo( const V3f& _point ) const override;
			virtual Sample SampleAt( const V3f& _point ) const override;
		};

		/// Intersection: A & B.
		/// dist( A & B ) = max( dist(A), dist(B) ).
		struct Intersection : BinaryOperation
		{
		public:
			virtual F32 DistanceTo( const V3f& _point ) const override;
			//virtual Sample SampleAt( const V3f& _point ) const override;
		};


		/// smooth blend between two shapes
		struct Blend : BinaryOperation
		{
			//NOTE: blendA + blendB should be 1 !
			float		blendA;	//!< must be always positive
			float		blendB;	//!< must be always positive
		public:
			Blend()
			{
				blendA = 0.5f;
				blendB = 0.5f;
			}
			virtual float DistanceTo( const V3f& _point ) const override
			{
				return A->DistanceTo(_point) * blendA + B->DistanceTo(_point) * blendB;
			}
		};

		struct CSG_AddOffset : UnaryOperation
		{
			float		offset;
		public:
			CSG_AddOffset()
			{
				offset = 0.0f;
			}
			virtual float DistanceTo( const V3f& _point ) const override
			{
				return O->DistanceTo(_point) + offset;
			}
		};

		struct Offset : UnaryOperation
		{
			V3f	offset;	// 'negative' translation
		public:
			Offset()
			{
				offset = V3_Zero();
			}
			virtual float DistanceTo( const V3f& _point ) const override
			{
				return O->DistanceTo( _point + offset );
			}
		};
		struct RotateZ : UnaryOperation
		{
			F32	angle;
		public:
			RotateZ()
			{
				angle = 0.0f;
			}
			virtual float DistanceTo( const V3f& _point ) const override
			{
				F32 sinAngle, cosAngle;
				mmSinCos( angle, sinAngle, cosAngle );

				V3f rotated_position;
				rotated_position.x = _point.x * cosAngle;
				rotated_position.y = _point.y * sinAngle;
				rotated_position.z = _point.z;

				return O->DistanceTo( rotated_position );
			}
		};

		struct CSG_Rotate : UnaryOperation
		{
			M33f	matrix;
		public:
			CSG_Rotate()
			{
				matrix = M33_Identity();
			}
			virtual float DistanceTo( const V3f& _point ) const override
			{
				return O->DistanceTo( M33_Transform( matrix, _point ) );
			}
		};


	}//namespace CSG

}//namespace SDF


namespace SDF
{
	static inline bool IsSolid( F32 distance ) { return distance < 0.0f; }

	/// In order that normals be defined along an implicit surface,
	/// the function must be continuous and differentiable.
	/// That is, the first partial derivatives must be continuous
	/// and not all zero, everywhere on the surface.
	/// useful links:
	/// modeling with distance functions:
	/// http://www.iquilezles.org/www/articles/distfunctions/distfunctions.htm
	template< class ISOSURFACE >	// where ISOSURFACE has "DistanceTo( F32 x, F32 y, F32 z ) : F32"
	V3f EstimateNormal( const ISOSURFACE& field, F32 _x, F32 _y, F32 _z, F32 H = 1e-3f )
	{
	#if 0
		// The gradient of the distance field yields the normal.
		F32	d0	= field.DistanceTo( V3_Set( _x, _y, _z ) );
		F32	Nx	= field.DistanceTo( V3_Set( _x + H, _y, _z ) ) - d0;
		F32	Ny	= field.DistanceTo( V3_Set( _x, _y + H, _z ) ) - d0;
		F32	Nz	= field.DistanceTo( V3_Set( _x, _y, _z + H ) ) - d0;
	#else
		mxBIBREF("John C. Hart, Daniel J. S, and Louis H. Kauffman T. Ray Tracing Deterministic 3-D Fractals. Computer Graphics, 23:289–296, 1989.");
		// 6-neighbor central difference gradient estimator.
		// The 6-point gradient might be extended to versions where 18 or 26 samples are taken.
		// Centered difference approximations are more precise than one-sided approximations.
		const F32 Nx = field.DistanceTo(V3_Set(_x + H, _y, _z)) - field.DistanceTo(V3_Set(_x - H, _y, _z));
		const F32 Ny = field.DistanceTo(V3_Set(_x, _y + H, _z)) - field.DistanceTo(V3_Set(_x, _y - H, _z));
		const F32 Nz = field.DistanceTo(V3_Set(_x, _y, _z + H)) - field.DistanceTo(V3_Set(_x, _y, _z - H));
	#endif
		V3f	N = V3_Normalized( V3_Set( Nx, Ny, Nz ) );
	//	mxASSERT(V3_IsNormalized(N));
		return N;
	}


	// Texture mapping.

	V3f GenTexCoords_CubeMapping(
		const V3f& _point,
		const V3f& _halfSize
	);
	V3f GenTexCoords_SphereMapping(
		const V3f& _point,
		const V3f& _halfSize
	);

	/// Constant step ray marching.
	/// Ray marching suffers from performance issues
	/// since it requires fine sampling along the ray,
	/// in order to achieve adequate level of detail.
	bool FindIntersection_Bruteforce(
		const Isosurface* _SDF,
		const V3f& _start,
		const float _max,
		const V3f& _dir,
		F32 &_timeHit,	// in range [0..1]
		int numSteps = 32	// can be increased to improve precision
	);
	bool FindIntersection_Bruteforce(
		const Isosurface* _SDF,
		const V3f& _start,
		const V3f& _end,
		V3f &_hitPoint,
		F32 &_timeHit,	// in range [0..1]
		int numSteps = 32	// can be increased to improve precision
	);

	/// Sphere Tracing. Way faster than the constant step ray marching above!
	bool CastRay_SphereTracing(
		const Isosurface* _SDF,
		const V3f& _start,
		const V3f& _end,
		F32 &_time01
	);

	bool CastRay_SphereTracing2(
		const Isosurface* _SDF,
		const V3f& _start,
		const V3f& _rdir,
		const F32 _rmax,
		F32 &_tWhenHit
	);

	void RayTraceImage(
		const Isosurface* _surface,
		const V3f& camera_position_in_world_space,
		const V3f& _cameraForward,
		const V3f& _cameraRight,
		const V3f& _cameraUp,
		const F32 _farPlane,
		const F32 _halfFoVy,// half of vertical field of view, in radians
		const F32 _aspect,	// aspect ratio = image_width / image_height
		const U32 _sizeX,	// image width in pixels
		const U32 _sizeY,	// image height in pixels
		float* _imagePtr,	// pointer to image data
		U32 _startY = 0,	// first vertical line
		U32 _endY = ~0		// last vertical line
	);

	namespace Testing
	{
		const Isosurface* GetTestScene( const V3f& sceneSize );
	}//namespace Testing

}//namespace SDF

namespace VX
{

/// NOTE: I don't recommend its usage because of redundant calculations when building an octree (12 edges for octrees vs 3 edges on a uniform grid)
class SDF_Sampler : public AVolumeSampler
{
	const SDF::Isosurface &	m_isosurface;
	/*const*/ V3f	m_cellSize;	//!< the size of each dual cell
	/*const*/ V3f	m_offset;	//!< the world-space offset of each sample

public:
	SDF_Sampler( const SDF::Isosurface& _surface, const AABBf& _bounds, const Int3& _resolution )
		: m_isosurface( _surface )
	{
		m_cellSize = V3_Divide( _bounds.size(), V3f::fromXYZ(_resolution) );
		m_offset = _bounds.min_corner;
	}

	virtual bool IsInside(
		int iCellX, int iCellY, int iCellZ
	) const override;

	virtual U32 SampleHermiteData(
		int iCellX, int iCellY, int iCellZ,
		HermiteDataSample & _sample
	) const override;

	//bool IsInside_NonVirtual(
	//	int iCellX, int iCellY, int iCellZ
	//) const;
};

}//namespace VX

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
