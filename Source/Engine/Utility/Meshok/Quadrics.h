// Quadric Error Metrics (QEM). Linear Least Squares (aka QEF) solvers.
#pragma once

#include <Utility/Meshok/Meshok.h>
#include <Utility/Meshok/Volumes.h>

mxBIBREF("Garland, M. and Heckbert, P. S. 1997. Surface simplification using quadric error metrics. In Proceedings of the 24th Annual Conference on Computer Graphics and interactive Techniques International Conference on Computer Graphics and Interactive Techniques. ACM Press/Addison-Wesley Publishing Co., New York, NY, 209-216.");
/// A quadric is a symmetric 4x4 matrix which can be stored within ten floats.
/// Quadric Error Metrics are a measurement of error that determines how far a vertex is from an ideal spot.
/// Each individual quadric is found using the plane equation derived from a triangle.
/// The plane equation is ax + by + cz + d = 0 where a^2 + b^2 + c^2 = 1.
/// The fundamental error quadric matrix Kp for the plane (a, b, c, d) is a symmetric 4x4 matrix:
/// 
///                |a^2  ab   ac   ad|
/// Kp = p * p^T = |ab  b^2   bc   bd|
///                |ac   bc  c^2   cd|
///                |ad   bd   cd  d^2|
///
template< typename Scalar >	// templated so that we can use floats or doubles (doubles are preferred)
class Quadric
{
	union {
		struct {
			Scalar a2, ab, ac, ad;
			Scalar     b2, bc, bd;
			Scalar         c2, cd;
			Scalar             d2;
		};
		Scalar	m[10];	//!< symmetric 4x4 matrix
	};

public:
	mxFORCEINLINE Quadric( Scalar c = Scalar(0) )
	{
		for( int i = 0; i < mxCOUNT_OF(m); i++ ) {
			m[i] = c;
		}
	}

	///sets the quadric representing the squared distance to _pt
	template <class _Point>
	void set_distance_to_point( const _Point& _pt )
	{
		*this = Quadric(
			1, 0, 0, -_pt[0],
			   1, 0, -_pt[1],
			      1, -_pt[2],
			         V3_Dot(_pt,_pt)
		);
	}

	/// Sets the quadric representing the squared distance to the plane [a,b,c,d].
	/// Computes the fundamental error quadric matrix for the given plane: Kp = p * p^T.
	mxFORCEINLINE Quadric( Scalar a, Scalar b, Scalar c, Scalar d )
	{
		a2 = a*a;  ab = a*b;  ac = a*c;  ad = a*d;
		b2 = b*b;  bc = b*c;  bd = b*d;
		c2 = c*c;  cd = c*d;
		d2 = d*d;
	}

	mxFORCEINLINE Quadric(
		Scalar m11, Scalar m12, Scalar m13, Scalar m14, 
		Scalar m22, Scalar m23, Scalar m24,
		Scalar m33, Scalar m34,
		Scalar m44
		)
	{
		m[0] = m11;  m[1] = m12;  m[2] = m13;  m[3] = m14; 
		m[4] = m22;  m[5] = m23;  m[6] = m24; 
		m[7] = m33;  m[8] = m34;
		m[9] = m44;
	}

	Scalar operator[] ( int i ) const { return m[i]; }

	Scalar Determinant(
		int a11, int a12, int a13,
		int a21, int a22, int a23,
		int a31, int a32, int a33 ) const
	{
		Scalar det =
			m[a11]*m[a22]*m[a33] +
			m[a13]*m[a21]*m[a32] +
			m[a12]*m[a23]*m[a31] -
			m[a13]*m[a22]*m[a31] -
			m[a11]*m[a23]*m[a32] -
			m[a12]*m[a21]*m[a33];
		return det;
	}

	mxFORCEINLINE const Quadric operator + ( const Quadric& n ) const
	{ 
		return Quadric(
			m[0]+n[0],   m[1]+n[1],   m[2]+n[2],   m[3]+n[3], 
			m[4]+n[4],   m[5]+n[5],   m[6]+n[6], 
			m[7]+n[7],   m[8]+n[8],
			m[9]+n[9]
		);
	}

	mxFORCEINLINE Quadric& operator += ( const Quadric& n )
	{
		m[0]+=n[0];   m[1]+=n[1];   m[2]+=n[2];   m[3]+=n[3]; 
		m[4]+=n[4];   m[5]+=n[5];   m[6]+=n[6];   m[7]+=n[7]; 
		m[8]+=n[8];   m[9]+=n[9];
		return *this;
	}

	/// Computes v^T * Q * v,
	/// i.e. multiply the transpose with the quadric
	/// and then multiply with the original position again.
	mxFORCEINLINE Scalar ComputeVertexError( Scalar x, Scalar y, Scalar z ) const
	{
#if 0
		return a2*x*x + 2*ab*x*y + 2*ac*x*z + 2*ad*x + b2*y*y
			+ 2*bc*y*z + 2*bd*y + c2*z*z + 2*cd*z + d2;
#else
		// reordered subexpressions - this should be faster, but could lower precision/stability
		return (a2*x*x + b2*y*y) + (c2*z*z + d2)
			+ ( (ab*x*y + ac*x*z + bc*y*z) + (ad*x + bd*y + cd*z) ) * Scalar(2);
#endif
	}
};

typedef Quadric<float> QuadricF;
typedef Quadric<double> QuadricD;





mxREFACTOR("rename")
//HACK: enums in namespaces are not supported
enum vxDRAW_MODE
{
	vxDRAW_NORMAL,//!< preserves both sharp and smooth features (Dual Contouring)
	vxDRAW_BLOXEL,//!< Bloxel/Cuberille/Minecraft-style blocks - cube centers are connected
	vxDRAW_SMOOTH,//!< Surface Nets
	//vxDRAW_BLOBBY,//!< sloped/bevelled cubes (Marching Cubes/Vertex Clustering/'Quantized')
	vxDRAW_MAX
};
mxDECLARE_ENUM( vxDRAW_MODE, U8, vxDRAW_MODE_T );



enum EFeatureType
{
	Feature_None	= 0,	//!< ("plane") the vertex doesn't lie on a sharp feature
	Feature_Edge	= 1,	//!< ("edge") the vertex lies on a crease
	Feature_Corner	= 2,	//!< ("corner") the vertex is a sharp corner
};

/// QEF solver may return bad vertices (that lie outside the cell's bounds),
/// they must be clamped to avoid triangle mesh self-intersections/overlaps
enum CLAMP_MODE
{
	/// don't clamp bad positions, the resulting mesh may have weird stretched triangles
	CLAMP_NONE = 0,

	/// rogue vertices are clamped to cell bounds which causes jaggy, blocky appearance
	CLAMP_TO_CELL_BOUNDS,//CLAMP_TO_CELL_AABB,

	/// smoothes out the surface
	CLAMP_TO_MASS_POINT,

	//// run the solver twice
	//CLAMP_EXPERIMENTAL
};
mxDECLARE_ENUM( CLAMP_MODE, U8, CLAMP_MODE_E );

/*
=======================================================================
	Linear Least Squares (LLS) solvers (aka QEF solvers)
=======================================================================
*/

/// Represents a generic QEF- / Least-Squares solver.
struct QEF_Solver
{
	struct Input : VX::HermiteDataSample
	{
		AABBf bounds;	//!< can be used for clamping the solution
		float threshold;	//!< error threshold
		int maxIterations;	//!< only for iterative solver
	public:
		Input();

		//ERet AddPlane( const V4f& _plane );
		ERet addPoint( const V3f& _position, const V3f& _normal );

		V3f CalculateAveragePoint() const;
		V3f CalculateAverageNormal() const;

		/// Estimates the angle of the normal cone.
		float EstimateNormalConeAngle() const;
	};
	struct Output
	{
		V3f position;	//!< position with least error
		float error;	//!< always positive (least-squares error)
		EFeatureType	feature;	//!< The dimension of the feature: 0 = plane (no sharp feature), 1 = edge, 2 = corner.
		int pad16[3];
		//V3f normal;
		char qef[64];	//!< internal solver data
		//bool clamped;
	};
	virtual void Solve( const Input& input, Output &output ) const = 0;
	//virtual void const svd::QefData& GetQefData() { return svd::QefData(); }
};

/// Blocky, Minecraft-style / Cuberille
struct QEF_Solver_Bloxel : QEF_Solver
{
	virtual void Solve( const Input& input, Output &output ) const override
	{
		output.position = AABBf_Center(input.bounds);
		output.error = 0.0f;
		output.feature = Feature_None;
	}
};

/// smooth, Surface-Nets style 
struct QEF_Solver_Simple : QEF_Solver
{
	virtual void Solve( const Input& input, Output &output ) const override
	{
		output.position = input.CalculateAveragePoint();
		output.error = 0.0f;
		output.feature = Feature_None;
	}
};

// too smooth
// Uses Leonardo Augusto Schmitz's easy-to-understand method
// with exact normals at intersection points to reduce complexity.
//
struct QEF_Solver_ParticleBased : QEF_Solver
{
	static const int MAX_ITERATIONS = 32;
	virtual void Solve( const Input& input, Output &output ) const override;
};

// good
struct QEF_Solver_SVD : QEF_Solver
{
	virtual void Solve( const Input& input, Output &output ) const override;
};

// good
struct QEF_Solver_QR : QEF_Solver
{
	virtual void Solve( const Input& input, Output &output ) const override;
};

mxSTOLEN("comments: ");
/*
Solves A*x = b for over-determined systems.

Solves using  At*A*x = At*b   trick where At is the transponate of A
*/
struct QEF_Solver_Direct_AtA : QEF_Solver
{
	virtual void Solve( const Input& input, Output &_output ) const override
	{
UNDONE;
	}
};

struct QEF_Solver_SVD_Eigen : QEF_Solver
{
	virtual void Solve( const Input& input, Output &output ) const override;
};

struct QEF_Solver_SVD2 : QEF_Solver
{
	virtual void Solve( const Input& input, Output &output ) const override;
};

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
