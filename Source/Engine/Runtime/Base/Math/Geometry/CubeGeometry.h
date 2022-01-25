// Cube-related constants.
#pragma once


/*
=======================================================================
	CUBE CORNERS
=======================================================================
*/

//
//		The Coordinate System Used for Voxel Space:
//		   Z
//		   |  /Y
//		   | /
//		   |/_____X
//		 (0,0,0)
//	
//	Face orientation (normal) - apply right-hand rule to the edge loop.
//
//	         UP  NORTH
//	         |  /
//	         | /
//	   WEST--O--EAST
//	        /|
//	       / |
//	  SOUTH  DOWN
//	
//	LABELING OF VERTICES, EDGES AND FACES:
//	
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
//	(see Gray code, Hamming distance, De Bruijn sequence)
//	
//	Each octant gets a 3-bit number between 0 and 7 assigned,
//	depending on the node's relative position to its parent's center.
//	The possible relative positions are:
//	bottom-left-front (000), bottom-right-front (001), bottom-left-back (010), bottom-right-back (011),
//	top-left-front (100), top-right-front (101), top-left-back (110), top-right-back (111).
//	The locational code of any child node in the tree can be computed recursively
//	by concatenating the octant numbers of all the nodes from the root down to the node in question.
//  
//	Numbering of Octree children.
//	Children follow a predictable pattern to make accesses simple.
//  
//	Cells are numbered such that:
//	bit 0 determines the X axis where left   is 0 and right is 1;
//	bit 1 determines the Y axis where front  is 0 and back  is 2;
//	bit 2 determines the Z axis where bottom is 0 and top   is 4;
//	Cells can be referenced by ORing the three axis bits together.
//	(Z- Space Filling Curve)
//	This pattern defines a 3-bit value, where "-" is a zero-bit and "+" is a one-bit.
//	Here, '-' means less than 'origin' in that dimension, '+' means greater than:
//  
//	0 = --- (000b)
//	1 = +-- (001b)
//	2 = -+- (010b)
//	3 = ++- (011b)
//	4 = --+ (100b)
//	5 = +-+ (101b)
//	6 = -++ (110b)
//	7 = +++ (111b)
//
//	NOTE: two vertices are connected by an edge, if their indices differ by one and only one bit.

enum {
	MASK_POS_X = BIT(0),
	MASK_POS_Y = BIT(1),
	MASK_POS_Z = BIT(2)
};

enum ECubeCorner
{
	Cube_Corner0,
	Cube_Corner1,
	Cube_Corner2,
	Cube_Corner3,
	Cube_Corner4,
	Cube_Corner5,
	Cube_Corner6,
	Cube_Corner7,

	Cube_CornerCount
};


// CORNER_MASK - 8 bits for eight cube corners, each bit is 1 if inside, 0 - outside
#define vxIS_CORNER_INSIDE( CORNER_MASK, CORNER )\
	(MapTo01((CORNER_MASK) & BIT(CORNER)))


// (CORNER_MASK == 0 || CORNER_MASK == 0xFF) - the cell is 'trivial' (i.e. fully inside or outside the volume)
// NOTE: it's often faster to fetch the "active edges" mask from the 'g_LUT_cellcase_to_edge_mask' table and compare it with zero
#if 0
	#define vxIS_ZERO_OR_0xFF( CORNER_MASK )\
		(!(CORNER_MASK) || U8(CORNER_MASK) == 0xFF)

	#define vxNONTRIVIAL_CELL( CORNER_MASK )\
		((CORNER_MASK) && U8(CORNER_MASK) != 0xFF)
#else
	// A branch saved - a cycle gained!
	#define vxIS_ZERO_OR_0xFF( CORNER_MASK )\
		(U8((CORNER_MASK) + 1) <= 1)

	#define vxNONTRIVIAL_CELL( CORNER_MASK )\
		(U8((CORNER_MASK) + 1) > 1)
#endif


// Checks if the given cube edge has an intersection point, i.e. the corresponding cube corners have opposite signs.
#define vxIS_ACTIVE_EDGE( CORNER_MASK, CORNER_A, CORNER_B )\
	(MapTo01((CORNER_MASK) & BIT(CORNER_A)) != MapTo01((CORNER_MASK) & BIT(CORNER_B)))


// prefer XOR: smaller code size, XOR hardware logic is simpler
//#define Cube_GetOppositeCorner( corner_index )	(7 - (corner_index))
#define Cube_GetOppositeCorner( corner_index )	((corner_index) ^ 7)


mxFORCEINLINE static
const UInt3 Cube_GetCornerOffset_Positive( const ECubeCorner cube_corner )
{
	return UInt3(
		(cube_corner & BIT(0)) ? +1 : 0,
		(cube_corner & BIT(1)) ? +1 : 0,
		(cube_corner & BIT(2)) ? +1 : 0
	);
}

mxFORCEINLINE static
const Int3 Cube_GetCornerNeighborOffset( const ECubeCorner cube_corner )
{
	return Int3(
		(cube_corner & BIT(0)) ? +1 : -1,
		(cube_corner & BIT(1)) ? +1 : -1,
		(cube_corner & BIT(2)) ? +1 : -1
	);
}

template< class VECTOR_3D, typename SCALAR >
mxFORCEINLINE static
const VECTOR_3D Cube_GetCornerPos(
	const VECTOR_3D& cube_minimal_corner
	, const SCALAR cube_side_length
	, const UINT cube_corner_index
	)
{
	VECTOR_3D cube_corner_position = {
		(cube_corner_index & BIT(0)) ? (cube_minimal_corner.x + cube_side_length) : cube_minimal_corner.x,
		(cube_corner_index & BIT(1)) ? (cube_minimal_corner.y + cube_side_length) : cube_minimal_corner.y,
		(cube_corner_index & BIT(2)) ? (cube_minimal_corner.z + cube_side_length) : cube_minimal_corner.z
	};
	return cube_corner_position;
}


/*
=======================================================================
	QUADRANTS
=======================================================================
*/

namespace Quadrants
{
	enum Enum
	{
		Q00 = 0,
		Q01 = 1,
		Q10 = 2,
		Q11 = 3,
	};
	enum { Count = 4 };
}//namespace Quadrants

#define GET_QUADRANT_OPPOSITE_TO(quadrant)	(Quadrants::Enum( 3 - quadrant ))

inline
const Quadrants::Enum Cube_GetCornerQuadrant(
	const ECubeCorner cube_corner
	, const EAxis edge_axis
	)
{
	mxOPTIMIZE("turn into a branchless version");

	if( edge_axis == AXIS_X )
	{
		return (Quadrants::Enum) (cube_corner >> 1);
	}
	else if( edge_axis == AXIS_Y )
	{
		return (Quadrants::Enum) ( ((cube_corner & 0x4) >> 1) | (cube_corner & 0x1) );
	}
	else
	{
		return (Quadrants::Enum) (cube_corner & 0x3);
	}
}



/// inserts a zero bit at the specified position;
/// produces a 3-bit number
inline
const UINT InsertZeroBitAtAxisPosition_SlowReferenceImpl(
	const UINT two_bit_number	// [0..3]
	, const EAxis axis	// [0..2]
	)
{
	if( axis == AXIS_X )
	{
		return (two_bit_number << 1u);
	}
	else if( axis == AXIS_Y )
	{
		return ((two_bit_number & 0x2) << 1u) | (two_bit_number & 0x1);
	}
	else
	{
		return two_bit_number;
	}
}

static mxFORCEINLINE
const UINT InsertZeroBitAtAxisPosition_Conditional(
	const UINT two_bit_number	// [0..3]
	, const EAxis axis	// [0..2]
	)
{
	return (axis == AXIS_Z)
        ? two_bit_number
		: (axis == AXIS_X)
        ? (two_bit_number << 1u)
		: ((two_bit_number & 0x2) << 1u) | (two_bit_number & 0x1)
		;
}

/// inserts a zero bit at the specified position;
/// produces a 3-bit number
inline
const UINT InsertZeroBitAtAxisPosition_Branchless(
	const UINT two_bit_number	// [0..3]
	, const EAxis axis	// [0..2]
	)
{
	const UINT x_axis_result = (two_bit_number << 1u);
	const UINT y_axis_result = ((two_bit_number & 0x2) << 1u) | (two_bit_number & 0x1);

	return (x_axis_result  & -( axis == AXIS_X ))
		|  (y_axis_result  & -( axis == AXIS_Y ))
		|  (two_bit_number & -( axis == AXIS_Z ))
		;
}


/*
=======================================================================
	CUBE EDGES
=======================================================================
*/

//	
//	Cube edge enumeration
//	(edges are split into 3 groups by axes X,Y,Z - this allows to find the edge's axis:
//	    edge_axis = edge_index / 4;
//	Edges are numbered in zig-zag fashion which allows to replace some LUTs with ALUs:
//
//	        ______3______
//	       /|           /|
//	      6 |10        7 |
//       /  |         /  |11
//	    |------2-----|   |
//	    |   |_____1__|___|
//	    |   /        |   /
//	   8|  4        9|  5
//	    | /          | /
//	    |/___________|/
//	           0
//

enum ECubeEdge
{
	// Axis X:
	CubeEdge0,
	CubeEdge1,
	CubeEdge2,
	CubeEdge3,

	// Axis Y:
	CubeEdge4,
	CubeEdge5,
	CubeEdge6,
	CubeEdge7,

	// Axis Z:
	CubeEdge8,
	CubeEdge9,
	CubeEdgeA,
	CubeEdgeB,

	CubeEdgeCount
};

static const U32 CUBE_EDGES_AXIS_X = (BIT(0) | BIT(1) | BIT(2) | BIT(3));
static const U32 CUBE_EDGES_AXIS_Y = (BIT(4) | BIT(5) | BIT(6) | BIT(7));
static const U32 CUBE_EDGES_AXIS_Z = (BIT(8) | BIT(9) | BIT(10) | BIT(11));

namespace CubeEdges
{
	enum Names
	{
		BN = 0,  /* bottom near edge  */
		BF = 1,  /* bottom far edge   */
		TN = 2,  /* top near edge     */
		TF = 3,  /* top far edge      */

		LB = 4,  /* left bottom edge  */
		LT = 6,  /* left top edge     */
		LN = 8,  /* left near edge    */
		LF = 10, /* left far edge     */

		RB = 5,  /* right bottom edge */
		RT = 7,  /* right top edge    */
		RN = 9,  /* right near edge   */
		RF = 11, /* right far edge    */
	};
}//namespace CubeEdges



#if 0//vxCFG_DEBUG_CUBE_LIB

	static mxFORCEINLINE
	EAxis EAxis_from_CubeEdge( const ECubeEdge cube_edge )
	{
		return (EAxis) ( cube_edge / 4 );
	}

#else

	#define EAxis_from_CubeEdge( cube_edge )		( (EAxis)( (UINT(cube_edge)) >> 2u ) )

#endif


/// returns the index of the start vertex (with minimal coordinates) of the edge
/// NOTE: the returned value is in range [0..6], i.e. vertex 7 (the cell's maximal corner) cannot be a starting point
static mxFORCEINLINE
ECubeCorner Cube_GetEdgeStartCorner(const ECubeEdge cube_edge)
{

#if mxARCH_TYPE == mxARCH_64BIT

	const U64 start_vertex_by_edge_index_LUT =
		// Edges along X axis:
		(U64(0)      )|
		(U64(2) <<  4)|
		(U64(4) <<  8)|
		(U64(6) << 12)|
		// Edges along Y axis:
		(U64(0) << 16)|
		(U64(1) << 20)|
		(U64(4) << 24)|
		(U64(5) << 28)|
		// Edges along Z axis:
		(U64(0) << 32)|
		(U64(1) << 36)|
		(U64(2) << 40)|
		(U64(3) << 44);

	//
	return (ECubeCorner) ( (start_vertex_by_edge_index_LUT >> (cube_edge * 4)) & 0xF );

#else

	// avoid __aullshr() in x86 MSVC++

	return (ECubeCorner) InsertZeroBitAtAxisPosition_Branchless(
		cube_edge & 0x3,	// == cube_edge % 4
		EAxis_from_CubeEdge(cube_edge)
		);

#endif

}

static mxFORCEINLINE
void Cube_Endpoints_from_Edge(
	const ECubeEdge cube_edge,
	ECubeCorner &vertex_a_, ECubeCorner &vertex_b_
	)
{
	vertex_a_ = Cube_GetEdgeStartCorner(cube_edge);
	vertex_b_ = (ECubeCorner) (vertex_a_ | BIT(EAxis_from_CubeEdge(cube_edge)));
}


/// NOTE: corners must differ by one bit!
static mxFORCEINLINE
const EAxis EdgeAxis_from_Endpoints(
	const ECubeCorner vertex_a, const ECubeCorner vertex_b
	)
{
	mxASSERT(vertex_a != vertex_b);

	// axis = number_of_trailing_zeros_from_lsb( vertex_a xor vertex_b )
	return (EAxis) CountTrailingZeros32( vertex_a ^ vertex_b );
}

inline
const ECubeEdge ECubeEdge_from_Endpoints(
	const ECubeCorner vertex_a, const ECubeCorner vertex_b
	)
{
	mxASSERT(vertex_a != vertex_b);

	const EAxis edge_axis = EdgeAxis_from_Endpoints(vertex_a, vertex_b);

	const UINT start_edge_index = edge_axis * 4;

	// Vertices differ only by the "edge axis" bit,
	// so we use any of them to compose the 2-bit edge offset.

	// Take the first vertex.

	const Quadrants::Enum edge_corner_quadrant =
		Cube_GetCornerQuadrant(vertex_a, edge_axis);

	return (ECubeEdge) (start_edge_index + edge_corner_quadrant);
}

namespace CubeEdges
{
	const Int3 GetEdgeNeighborOffset(const ECubeEdge cube_edge);
}


/*
=======================================================================
	EDGE TABLE
=======================================================================
*/

/// Edge-table gives all the edges intersected in a given cube configuration.
/// The lower 12 bits store a bitmask of intersecting cube edges,
/// the upper 4 bits store the number of intersecting cube edges.
mxPREALIGN(16) extern const U16	g_LUT_cellcase_to_edge_mask[256];

static mxFORCEINLINE
U32 Cube_GetActiveEdgesMask( U8 cube_sign_configuration )
{
	return g_LUT_cellcase_to_edge_mask[ cube_sign_configuration ]
		& ((1<<12) - 1)// take the lower 12 bits
		;
}

struct CubeEdgesMask: NonCopyable
{
	/// Set bits correspond to active (surface-crossing) edges.
	/// A 32-bit int so that we can use TakeNextTrailingBit32().
	U32	active_edges_mask;

	int	num_intersecting_edges;

public:
	mxFORCEINLINE CubeEdgesMask( U8 cube_sign_configuration )
	{
		const U32 mask = g_LUT_cellcase_to_edge_mask[ cube_sign_configuration ];
		active_edges_mask = mask & ((1<<12) - 1);	// take the lower 12 bits
		num_intersecting_edges = mask >> 12;	// take the upper 4 bits
	}

	/// returns the the next intersecting cube edge, [0..11]
	mxFORCEINLINE ECubeEdge TakeNextActiveEdge()
	{
		return (ECubeEdge) TakeNextTrailingBit32( active_edges_mask );
	}
};

#if vxCFG_CUBE_LIB_EXPOSE_LUT_GENERATORS

	void Offline_Build_LUT_cellcase_to_edge_mask();

#endif // vxCFG_CUBE_LIB_EXPOSE_LUT_GENERATORS


/*
=======================================================================
	CUBE FACES
=======================================================================
*/


/*
//	Faces are numbered in the order -X, +X, -Y, +Y, -Z, +Z:
//	Face index - face normal:
//	0 (Left)   is -X,
//	1 (Right)  is +X,
//	2 (Front)  is -Y,
//	3 (Back)   is +Y,
//	4 (Bottom) is -Z,
//	5 (Top)    is +Z.
//
//	Cubes are unrolled in the following order,
//	looking along +Y direction (labelled '1'):
//	        _____
//	        | 5 |
//	    -----------------
//	    | 0 | 2 | 1 | 3 |
//	    -----------------
//	        | 4 |
//	        -----
//	
//             +----------+
//	           |+Y       5|
//	           | ^  +Z    |
//	           | |        |
//	           | +---->+X |
//	+----------+----------+----------+----------+
//	|+Z       0|+Z       2|+Z       1|+Z       3|
//	| ^  -X    | ^  -Y    | ^  +X    | ^  +Y    |
//	| |        | |        | |        | |        |
//	| +---->-Y | +---->+X | +---->+Y | +---->-X |
//	+----------+----------+----------+----------+
//             |-Y       4|
//	           | ^  -Z    |
//	           | |        |
//	           | +---->+X |
//	           +----------+
//	
//	Face Edges are numbered as follows:
//	
//	                  Z
//	      2           ^
//	  ---------       |
//    |       |       |
//	3 |       | 1    -Y----> X
//	  |       |
//    ---------      (Y axis points into the screen)
//        0
*/

namespace CubeFace
{
	enum Enum
	{
		NegX = 0, // -X
		PosX = 1, // +X
		NegY = 2, // -Y
		PosY = 3, // +Y
		NegZ = 4, // -Z
		PosZ = 5, // +Z

		Right  = PosX, // +X
		Back   = PosY, // +Y
		Top    = PosZ, // +Z
		Left   = NegX, // -X
		Front  = NegY, // -Y
		Bottom = NegZ, // -Z

		Count = 6
	};
	static mxFORCEINLINE Enum posFaceFoxAxis( UINT axis ) {
		return Enum( axis * 2 + 1 );
	}
	static mxFORCEINLINE UINT oppositeTo( UINT face ) {
		return face % 2 ? face - 1 : face + 1;
	}

#if MX_DEVELOPER
	String64 DbgMaskToString( int cube_face_mask );
#endif

}//namespace CubeFace
mxDECLARE_ENUM( CubeFace::Enum, U8, CubeFaceEnum );
mxDECLARE_FLAGS( CubeFace::Enum, U8, CubeFaceFlags );


struct CubeFaceMask
{
	unsigned int	bits;
};

struct CubeFaceMaskU8
{
	U8	bits;
};

mxFORCEINLINE Int3 CubeFaceNeighborOffset( const CubeFace::Enum cube_face )
{
	const UINT faceAxis = cube_face / 2u;	// [0..3), see CubeFace::Enum
	const UINT posAxisDir = cube_face % 2u; // can only be 1 or 0 if positive or negative direction
	Int3 faceNeighborOffset( 0 );
	faceNeighborOffset[ faceAxis ] = posAxisDir ? +1 : -1;
	return faceNeighborOffset;
}

static mxFORCEINLINE
const V3f GetCubeFaceNormal(
	const CubeFace::Enum cube_face
	)
{
	const UINT face_axis = cube_face / 2;	// [0..3), see CubeFace::Enum
	const UINT face_axis_is_pos_dir = cube_face % 2; // can only be 1 or 0 if positive or negative direction

	//
	V3f	face_normal = { 0, 0, 0 };
	face_normal.a[ face_axis ] = face_axis_is_pos_dir ? +1 : -1;
	return face_normal;
}

static mxFORCEINLINE
const V3f CubeFaceCenterPos(
	const CubeMLf& cube
	, const CubeFace::Enum cube_face
	)
{
	const float	cube_half_extent = cube.side_length * 0.5f;

	CV3f	center( cube.min_corner + CV3f(cube_half_extent) );

	//
	const UINT face_axis = cube_face / 2;	// [0..3), see CubeFace::Enum
	const UINT face_axis_is_pos_dir = cube_face % 2; // can only be 1 or 0 if positive or negative direction

	//
	center.a[ face_axis ] += face_axis_is_pos_dir ? +cube_half_extent : -cube_half_extent;
	return center;
}

/*
=======================================================================
	OCTREE ROUTINES
=======================================================================
*/

/// Returns the octant of the next adjacent cube sharing the common vertex.
/// E.g.:
/// (my_octant = 2, neighbor_number = 1) => neighbor_octant = 3
/// (my_octant = 2, neighbor_number = 2) => neighbor_octant = 0
/// (my_octant = 2, neighbor_number = 4) => neighbor_octant = 6
/// (my_octant = 7, neighbor_number = 1) => neighbor_octant = 6
/// (my_octant = 7, neighbor_number = 3) => neighbor_octant = 4
static mxFORCEINLINE
UINT Cube_GetCornerNeighborOctant(
	const UINT octant_of_this_cube,	// [0..8)
	const UINT corner_neighbor_number	// [1..8)
	)
{
	return octant_of_this_cube ^ corner_neighbor_number;
}

/// Returns the 3D integer offset of the neighbor relative to my octant.
inline
Int3 Cube_GetNeighborOctantOffset(
	const UINT my_octant,	// [0..8)
	const UINT neighbor_octant	// [0..8)
	)
{
	const bool my_octant_is_pos_x = (my_octant & MASK_POS_X);
	const bool my_octant_is_pos_y = (my_octant & MASK_POS_Y);
	const bool my_octant_is_pos_z = (my_octant & MASK_POS_Z);

	const bool neighbor_octant_is_pos_x = (neighbor_octant & MASK_POS_X);
	const bool neighbor_octant_is_pos_y = (neighbor_octant & MASK_POS_Y);
	const bool neighbor_octant_is_pos_z = (neighbor_octant & MASK_POS_Z);

	const Int3 neighbor_offset(
		(neighbor_octant_is_pos_x > my_octant_is_pos_x) ? +1 : (neighbor_octant_is_pos_x < my_octant_is_pos_x) ? -1 : 0,
		(neighbor_octant_is_pos_y > my_octant_is_pos_y) ? +1 : (neighbor_octant_is_pos_y < my_octant_is_pos_y) ? -1 : 0,
		(neighbor_octant_is_pos_z > my_octant_is_pos_z) ? +1 : (neighbor_octant_is_pos_z < my_octant_is_pos_z) ? -1 : 0
	);
	return neighbor_offset;
}


/*
=======================================================================
	NEIGHBOR ENUMERATION
=======================================================================
*/

/// Describes dual [cell|vertex] type: a [vertex|cell] can be lie fully inside the chunk (internal), touch its face or lie on its edge.
/// These types are used by modified (augmented?) Dual Contouring algorithm
/// which solves the problem of inter-cell dependency between adjacent chunks
/// by creating additional vertices on boundary faces and edges of each chunk.
/// During rendering, positions of boundary vertices must coincide between chunks for geomorphing/CLOD,
/// so we explicitly pass vertex types and LOD blend/morph factors to the CLOD vertex shader.
enum ECellType: unsigned
{
	// the vertex is fully inside the chunk
	Cell_Internal = 0,

	// Boundary vertex types start here:

	// boundary vertices on chunk faces
	Cell_FaceNegX, // -X
	Cell_FacePosX, // +X
	Cell_FaceNegY, // -Y
	Cell_FacePosY, // +Y
	Cell_FaceNegZ, // -Z
	Cell_FacePosZ, // +Z

	// boundary vertices on chunk edges
	Cell_Edge0,
	Cell_Edge1,
	Cell_Edge2,
	Cell_Edge3,
	Cell_Edge4,
	Cell_Edge5,
	Cell_Edge6,
	Cell_Edge7,
	Cell_Edge8,
	Cell_Edge9,
	Cell_EdgeA,
	Cell_EdgeB,

	// boundary vertices on chunk corners
	Cell_Corner0,
	Cell_Corner1,
	Cell_Corner2,
	Cell_Corner3,
	Cell_Corner4,
	Cell_Corner5,
	Cell_Corner6,
	Cell_Corner7,

	// we don't handle boundary vertices on corners (yet)
	Cell_TypeCount27	// marker, don't use
};
mxDECLARE_ENUM( ECellType, U8, CellVertexTypeE );
mxDECLARE_FLAGS( ECellType, U32, CellVertexTypeF );	// must have at least 27 bits

//mxFORCEINLINE ECellType CubeFace_To_BoundaryCellType( CubeFace::Enum _cubeFace ) {
//	return (ECellType) ( _cubeFace + FacePosX );	// CubeFace::Enum ->  ECellType
//}
mxFORCEINLINE ECellType CubeEdge_To_BoundaryCellType( int _cubeEdge ) {
	return (ECellType) ( _cubeEdge + Cell_Edge0 );	// Cube Edge Index ->  ECellType
}
mxFORCEINLINE ECellType CubeCorner_To_BoundaryCellType( int _cubeCorner ) {
	return (ECellType) ( _cubeCorner + Cell_Corner0 );
}



namespace CubeNeighbor
{
	enum Enum
	{
		// Face-adjacent neighbors:
		FaceNegX, // -X
		FacePosX, // +X
		FaceNegY, // -Y
		FacePosY, // +Y
		FaceNegZ, // -Z
		FacePosZ, // +Z
		// Edge-adjacent neighbors:
		Edge0,
		Edge1,
		Edge2,
		Edge3,
		Edge4,
		Edge5,
		Edge6,
		Edge7,
		Edge8,
		Edge9,
		EdgeA,
		EdgeB,
		// Corner-adjacent neighbors:
		Corner0,
		Corner1,
		Corner2,
		Corner3,
		Corner4,
		Corner5,
		Corner6,
		Corner7,
		// we don't handle boundary vertices on corners (yet)
		Count	// marker, don't use
	};
}//namespace CubeNeighbor

mxDECLARE_ENUM( CubeNeighbor::Enum, U8, CubeNeighborE );


/// A 26-bit mask indicating which neighbors of the cube (in the 3D Moore neighborhood)
/// (see ECubeNeighbor enum) are of interest.
/// LSB to MSB: 6 bits for faces, 12 bits for edges and 8 bits for corners.
struct Cube26NeighborsMask
{
	U32	u;	// bits from CubeNeighbor::Enum
};

namespace CubeNeighbor
{
	// NONE_OF_26_CELLS_IN_MOORE_NEIGHBORHOOD_HAVE_CHANGED
	static const Cube26NeighborsMask ZERO_MASK = { 0 };

	mxFORCEINLINE CubeNeighbor::Enum FromCellType(const ECellType cell_type)
	{
		mxASSERT(cell_type != ECellType::Cell_Internal);
		return (CubeNeighbor::Enum) (cell_type - 1);
	}

	CubeNeighbor::Enum GetOppositeNeighbor(const CubeNeighbor::Enum neighbor);

#if MX_DEVELOPER
	String128 DbgMaskToString(const Cube26NeighborsMask& cube_neighbors_mask);
#endif


}//namespace CubeNeighbor


void get_Coords_of_26_Neighbors(
	Int3 neighbors_coords_[CubeNeighbor::Count]
	, const Int3& my_coords
);
