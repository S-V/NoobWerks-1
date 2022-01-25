// for placing meshes in a voxel terrain
#pragma once

#include <Utility/Meshok/Volumes.h>	// VX::AVolumeSampler
#include <Utility/Meshok/BSP.h>

/// Prefer double precision for BSP-based CSG.
#ifndef BSP_USE_DBL
#define BSP_USE_DBL	0
#endif

//! \brief User-provided class for following the progress of a task
class AProgressListener {
public:
	virtual void setCurrentPercent( const float _percent ) = 0;
	virtual void setCurrentStatus( const char* _text ) = 0;
};

namespace Meshok
{
	class TriMesh;
}//namespace Meshok

namespace BSP2
{

/// Prefer double precision for BSP-based CSG.
#if BSP_USE_DBL
	typedef double Real;
	#define	MAX_REAL		DBL_MAX
	#define	EPS_REAL		DBL_EPSILON
#else
	typedef float Real;
	#define	MAX_REAL		FLT_MAX
	#define	EPS_REAL		FLT_EPSILON
#endif

typedef AabbMM< Real >	AabbT;
typedef Tuple3< Real >	Vec3T;
typedef CTuple3< Real >	CVec3T;

mxPREALIGN(16) struct Plane : Tuple4< Real >, CStruct
{
	mxDECLARE_CLASS(Plane,CStruct);
	mxDECLARE_REFLECTION;

	mmINLINE void fromPointNormal( const Vec3T& _point, const Vec3T& _normal ) {
		this->x = _normal.x;
		this->y = _normal.y;
		this->z = _normal.z;
		this->w = -Vec3T::dot( _normal, _point );
	}
	mmINLINE void set( const Vec3T& _normal, const Real _dist ) {
		this->x = _normal.x;
		this->y = _normal.y;
		this->z = _normal.z;
		this->w = _dist;
	}
	mmINLINE const Vec3T& normal() const {
		return *(Vec3T*) this;
	}
	mmINLINE const Real distance( const Vec3T& _point ) const {
		return (this->x * _point.x + this->y * _point.y) + (this->z * _point.z + this->w);
	}
	bool IsValid( const Real normalEpsilon = (Real)1e-6 ) const {
		float len = this->normal().lengthSquared();
		return mmAbs( len - (Real)1 ) < normalEpsilon;
	}

	static bool Equal( const Plane& planeA, const Plane& planeB, const Real normalEpsilon, const Real distanceEpsilon ) {
		if ( mmAbs( planeA.w - planeB.w ) > distanceEpsilon ) {
			return false;
		}
		const Vec3T normalA = planeA.normal();
		const Vec3T normalB = planeB.normal();
		return Vec3T::equal( normalA, normalB, normalEpsilon );
	}
} mxPOSTALIGN(16);

inline ATextStream & operator << ( ATextStream & log, const Plane& plane )
{
	log.PrintF("(%.4f, %.4f, %.4f, %.4f)", plane.x, plane.y, plane.z, plane.w);
	return log;
}

struct BuildParameters
{
	/*
		Used for searching splitting planes.
		If NULL, a default random # generator will be used.
	 */
	//UserRandom*		rnd;

	/*
		Mesh pre-processing.  The mesh is initially scaled to fit in a unit cube, then (if gridSize is not
		zero), the vertices of the scaled mesh are snapped to a grid of size 1/gridSize.
		A power of two is recommended.
		Default value = 65536.
	*/
	U32	snapGridSize;

	/*
		At each step in the tree building process, the surface with maximum triangle area is compared
		to the other surface triangle areas.  If the maximum area surface is far from the "typical" set of
		surface areas, then that surface is chosen as the next splitting plane.  Otherwise, a random
		test set is chosen and a winner determined based upon the weightings below.
		The value logAreaSigmaThreshold determines how "atypical" the maximum area surface must be to
		be chosen in this manner.
		Default value = 2.0.
	 */
	F32	logAreaSigmaThreshold;

	/*
		Larger values of testSetSize may find better BSP trees, but will take more time to create.
		testSetSize = 0 is treated as infinity (all surfaces will be tested for each branch).
	 */
	U32	testSetSize;

	/*
		How much to weigh the relative number of triangle splits when searching for a BSP surface.
	 */
	F32	splitWeight;

	/*
		How much to weigh the relative triangle imbalance when searching for a BSP surface.
	 */
	F32	imbalanceWeight;


	/// inverse of the max surface area
	F32	recipMaxArea;

	/*
		If false, the triangles associated with this BSP will not be kept.  The BSP may be used for CSG, but will
		not provide any mesh data.

		Default = true
	 */
	bool	keepTriangles;

public:
	BuildParameters()
	{
		setToDefault();
	}

	void	setToDefault()
	{
		//rnd = NULL;
		snapGridSize = 65536;
		logAreaSigmaThreshold = (F32)2.0;
		testSetSize = 10;
		splitWeight = (F32)0.5;
		imbalanceWeight = 0;
		recipMaxArea = 1.0f;
		keepTriangles = true;
	}
};

/// Numerical tolerances for building a BSP tree.
struct Tolerances
{
	/*
	A unitless value (relative to mesh size) used to determine mesh triangle coplanarity during BSP building.
	Default value = 1.0e-6.
	*/
	Real	linear;

	/*
	A threshold angle (in radians) used to determine mesh triangle coplanarity during BSP building.
	Default value = 1.0e-5.
	*/
	Real	angular;	//!< NORMAL_EPSILON

	/*
	A unitless value (relative to mesh size) used to determine triangle splitting during BSP building.
	Default value = 1.0e-9.
	*/
	Real	base;	//!< ON_EPSILON

	/*
	A unitless value (relative to mesh size) used to determine a skin width for mesh clipping against BSP
	nodes during mesh creation from the BSP.
	Default value = 1.0e-13.
	*/
	Real	clip;

	/*
	Mesh postprocessing.  A unitless value (relative to mesh size) used to determine merge tolerances for
	mesh clean-up after triangles have been clipped to BSP leaves.  A value of 0.0 disables this feature.
	Default value = 1.0e-6.
	*/
	Real	cleaning;

public:
	Tolerances()
	{
		setToDefault();
	}
	void setToDefault()
	{
#if BSP_USE_DBL
		linear = (Real)1.0e-6;
		angular = (Real)1.0e-5;
		base = (Real)1.0e-9;
		clip = (Real)1.0e-13;
		cleaning = (Real)1.0e-6;
#else
		linear = (Real)0.001;
		angular = (Real)0.00001;
		base = (Real)0.01;
		clip = (Real)0.001;
		cleaning = (Real)1.0e-4;
#endif
	}
};

struct BuildStats
{
	U32		polysBefore;
	U32		polysAfter;	// number of resulting polygons
	U32		numSplits;	// number of cuts caused by BSP

	U32		maxDepth;
	U32		numPlanes;
	U32		numInternalNodes;
	U32		numSolidLeaves, numEmptyLeaves;

	U32		bytesAllocated;
public:
	BuildStats();
	void Reset();
	void Print( U32 elapsedTimeMSec );
};

MAKE_ALIAS( HPlane, U32 );	//!< plane index
MAKE_ALIAS( HNode, U32 );	//!< node index - lower two bits describe the type of the node
MAKE_ALIAS( HFace, U32 );	//!< face index

mxREFLECT_HANDLE(HPlane);
mxREFLECT_HANDLE(HNode);
mxREFLECT_HANDLE(HFace);

const HPlane NIL_PLANE(~0);
const HNode NIL_NODE(~0);
const HFace NIL_FACE(~0);

enum ENamedConstants : U32
{
	BackChild = 0,
	FrontChild = 1,

	BSP_MAX_NODES = (1U<<30)-1,
	BSP_MAX_PLANES = MAX_UINT32-1,	// maximum allowed number of planes in a single tree
	BSP_MAX_POLYS = MAX_UINT32-1,
	BSP_MAX_DEPTH = 64,	// size of stack for ray casting (we try to avoid recursion)
};

/// This enum describes the allowed BSP node types.
enum ENodeType
{
	SurfaceNode	= 0,	//!< An internal (auto-)partitioning node (with a polygon-aligned splitting plane).
	//VolumeNode	= 1,	//!< An internal partitioning node (with an arbitrary splitting plane).
	SolidLeaf	= 2,	//!< An incell leaf node ( representing solid matter ).
	EmptyLeaf	= 3,	//!< An outcell leaf node ( representing empty space ).
};

struct Vertex : CStruct {
	Vec3T	xyz;
	Real	pad;
public:
	mxDECLARE_CLASS(Vertex,CStruct);
	mxDECLARE_REFLECTION;
	Vertex() {
	}
};

/// for addressing a contiguous range of items
struct IdxRange
{
	U32	first;	//!< index of the first item
	U32	count;	//!< 
public:
	mxFORCEINLINE IdxRange() { first = count = 0; }
	mxFORCEINLINE U32 Bound() const { return first + count; }
	mxFORCEINLINE bool IsEmpty() const { return !count; }
};

/// for addressing a contiguous range
struct IdxRange2
{
	U32	first;	//!< starting index (inclusive)
	U32	bound;	//!< 'right' bound (exclusive)
public:
	mxFORCEINLINE bool IsEmpty() const { return first == bound; }
	mxFORCEINLINE U32 count() const { return bound - first; }
};

/// 
struct Face : CStruct {
	IdxRange	vertices;	//!< vertices
	Vec3T		normal;
	Real		area;
public:
	mxDECLARE_CLASS(Face,CStruct);
	mxDECLARE_REFLECTION;
	Face() {
		normal = CVec3T(0);
		area = 0;
	}
};

/// polygons are used only during building the tree and then are discarded
struct TempMesh
{
	DynamicArray< Face >	m_F;		//!< convex polygons
	DynamicArray< Vertex >	m_V;		//!< array of vertices (always grows)
public:
	TempMesh( AllocatorI & _allocator )
		: m_F( _allocator )
		, m_V( _allocator )
	{}
};

/// a list of (convex) polygons lying on the same plane (having the same normal, etc.)
struct Surface
{
	U32			plane;	//!< index of the plane; many surfaces may refer to the same plane
	IdxRange2	faces;	//!< convex polygon lying on this surface
	F32			area;	//!< total surface area (combined area of all polygons) for choosing splitting surfaces and tracking percent complete
};

///// a convex volume of space
//struct Region
//{
//	U32	side;
//
//	// Not to be serialized, but we have this extra space since Region is used in a union with Surface
//	U32	tempIndex1;
//	U32	tempIndex2;
//	U32	tempIndex3;
//};

/*
-----------------------------------------------------------------------------
	Polygon-aligned (auto-partitioning) node-storing solid leaf-labeled BSP tree.
	Leaf nodes are not explicitly stored.
	It can be used for ray casting, collision detection and CSG operations.
-----------------------------------------------------------------------------
*/
struct Tree: CStruct {
	///
	struct Node : CStruct {
		HPlane	plane;	//!< (Unique) Hyperplane of the node (index into array of planes).
		HNode	kids[2];//!< Indices of the left child (negative subspace, behind the plane) and the right child (positive subspace, in front of plane).
		HFace	faces;	//«4 linked list of polygons lying on this node's plane (optional).
	public:
		mxDECLARE_CLASS(Node,CStruct);
		mxDECLARE_REFLECTION;
		Node();
	};

	DynamicArray< Node >	m_nodes;	//!< tree nodes (0 = root index)
	DynamicArray< Plane >	m_planes;	//!< unique plane equations
	//U32	m_N;	//!< the first N splitting planes are not polygon-aligned.
//	float					m_epsilon;	//!< plane 'thickness' epsilon used for building the tree
	M44f	m_worldToLocal;	//!< transforms from world-space point to the local space of this BSP tree
	M44f	m_localToWorld;	//!< used for transforming hit normals from BSP tree's space to world space

public:	// Construction.
	mxDECLARE_CLASS(Tree,CStruct);
	mxDECLARE_REFLECTION;
	Tree( AllocatorI & _allocator );

	struct Settings {
		/// should be 0 for better precision; set to 1 as a workaround the voxel terrain bug
		bool	preserveRelativeScale;
	public:
		Settings() {
			preserveRelativeScale = false;
		}
	};
	ERet buildFromMesh(
		const Meshok::TriMesh& _mesh,
		AllocatorI & scratchpad,
		const Settings& _settings = Settings()
	);

	void clear();

	U32 memoryUsed() const;

public:	// Queries.

	// Point-In-Solid
	bool PointInSolid( const V3f& _point ) const;

	// Ray casting

	/// returns no intersection if the ray is fully inside or outside the solid.
	bool CastRay(
		const V3f& _start, const V3f& _direction,
		float _tmin, float _tmax,	//!< *not* normalized (i.e. may lie outside [0..1])
		float *_thit, V3f *_hitNormal,
		const bool _startInSolid = false
	) const;

	// Line/Segment/Intersection testing

	// overlap testing

	// sweep tests

	// oriented capsule collision detection

public:	// Hermite data

	class Sampler : public VX::AVolumeSampler
	{
		const Tree &	m_tree;
		/*const*/ V3f	m_cellSize;	//!< the size of each dual cell
		/*const*/ V3f	m_offset;	//!< the world-space offset of each sample
	public:
		Sampler( const Tree& _tree, const AABBf& _bounds, const Int3& _resolution );

		virtual bool IsInside(
			int iCellX, int iCellY, int iCellZ
		) const override;

		bool isPointInside( const V3f& localPosition ) const;

		/// returns no intersection if the ray is fully inside or outside the solid.
		bool CastRay(
			const V3f& start, const V3f& direction,
			float tmax,	//!< *not* normalized (i.e. may lie outside [0..1])
			float *thit, V3f *hitNormal,
			const bool startInSolid = false
		) const;

		virtual U32 SampleHermiteData(
			int iCellX, int iCellY, int iCellZ,
			VX::HermiteDataSample & _sample
		) const override;

		virtual bool getEdgeIntersection(
			int iCellX, int iCellY, int iCellZ	//!< coordinates of the cell's minimal corner (or the voxel at that corner)
			, const EAxis edgeAxis	//!< axis index [0..3)
			, float &distance_
			, V3f &normal_	//!< directed distance along the positive axis direction and unit normal to the surface
		) const override;
	};

public:	// Testing & Debugging.
	void Debug_Print() const;
	bool Debug_Validate() const;

public:	// Serialization.

	struct Header_d {
		U32	num_nodes;
		U32	num_planes;
		F32	plane_epsilon;	//!< plane 'thickness' epsilon used for building the tree
		U32	unused_padding;
	};

	/// Header_d, nodes, planes
	ERet SaveToBlob(
		NwBlobWriter & _stream
	) const;

private:
	ERet buildTree(
		DynamicArray< Surface > & _surfaceStack,
		const BuildParameters& _buildOptions,
		const Tolerances& _tolerances,
		TempMesh & _mesh,
		AllocatorI & scratchpad
		);
};

struct StaticTree: CStruct {
	///
	struct Node : CStruct {
		HPlane	plane;	//!< (Unique) Hyperplane of the node (index into array of planes).
		HNode	child;	//!< Indices of the left child (negative subspace, behind the plane) and the right child (positive subspace, in front of plane).
	public:
		mxDECLARE_CLASS(Node,CStruct);
		mxDECLARE_REFLECTION;
		Node();
	};
	DynamicArray< Node >	m_nodes;	//!< tree nodes (0 = root index)
	DynamicArray< Plane >	m_planes;	//!< unique plane equations
};

}//namespace BSP2

namespace VX
{

/// for placing meshes in a voxel terrain
class VoxelizedMesh : CStruct, public ReferenceCountedX
{
public:
	BSP2::Tree	m_tree;	//!< the tree is stored in [-1..+1]
//	AABBf		m_aabb;	//!< bounding box in local space - must be in sync with above
	AllocatorI &		m_storage;
public:
	mxDECLARE_CLASS( VoxelizedMesh, CStruct );
	mxDECLARE_REFLECTION;
	VoxelizedMesh( AllocatorI & _storage )
		: m_tree(_storage)
		, m_storage(_storage)
	{}

	ERet CreateFromMesh(
		const char* _filepath
		, AllocatorI & scratchpad
	);

	ERet Load( AReader& _stream );
	ERet Save( AWriter &_stream ) const;

	ERet Save( NwBlob &_blob ) const;

	/// ReferenceCountedX
	virtual void deleteSelf() override;
};

/// for placing a voxelized model on the terrain
class MeshTransform
{
public:
	V3f		translation;
	V3f		Euler_angles;	//!< in radians
	float	scaling;	//!<
public:
	MeshTransform()
	{
		translation = CV3f(0);
		Euler_angles = CV3f(0);
		scaling = 1.0f;
	}
};

ERet loadOrCreateModel( TRefPtr< VX::VoxelizedMesh > &model,
					   const char* _sourceMesh, AllocatorI & _storage, const char* _pathToCache, bool _forceRebuild = false );

}//namespace VX
