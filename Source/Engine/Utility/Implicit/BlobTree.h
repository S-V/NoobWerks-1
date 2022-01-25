// Signed Distance Fields primitives.
// SIMD-Accelerated SDF/BlobTree evaluation.
/*
References:

HyperFun
http://hyperfun.org/wiki/doku.php?id=hyperfun:main
http://paulbourke.net/dataformats/hyperfun/HF_oper.html

Implementing the Blob Tree
https://web.uvic.ca/~etcwilde/papers/ImplementingTheBlobTree.pdf

Unifying Procedural Graphics (MSci Individual Project Report) [2009]
https://www.doc.ic.ac.uk/teaching/distinguished-projects/2009/d.birch.pdf
*/
#pragma once

#include <Base/Math/MiniMath.h>
#include <Base/Memory/FreeList/FreeList.h>

struct lua_State;

namespace implicit {

typedef int MaterialID;

enum NodeType: int {
	//=== Operators (Internal nodes):

	// Unary:
	//MT_NEGATION,	//!< Set-theoretic negation

	// Binary Operators:
	// CSG
	NT_UNION,	//!< Set-theoretic union
	NT_DIFFERENCE,	//!< Set-theoretic subtraction (difference): A \ B
	//NT_COMPLEMENT,
	NT_INTERSECTION,	//!< Set-theoretic intersection
	//MT_PRODUCT,	//!< Set-theoretic Cartesian product

	// Unary Operators:

	//=== Primitives (Leaf nodes):
	NT_PLANE,
	NT_SPHERE,
	NT_BOX,
	NT_INF_CYLINDER,	//!< infinite vertical cylinder (Z-up)
	NT_CYLINDER,	//!< capped vertical cylinder
	NT_TORUS,	//!< torus in the XY-plane
	NT_SINE_WAVE,	//!< concentric waves with a 'mountain' at the center
	NT_GYROID,
	NT_LAST_PRIM = NT_SPHERE,

	NT_TRANSFORM,	//!< not used in optimized (linearized) blob tree

	//NT_FIRST_OP_TYPE = NT_UNION
	NT_FIRST_PRIM_TYPE = NT_PLANE
};

//struct Node {
//	NodeType	type;
//};
//
//struct Plane: Node {
//	V4f		normal_and_distance;
//};
//
//struct Sphere: Node {
//	V3f		center;
//	float	radius;
//};

struct Node {
	NodeType	type;
	//MaterialID			materialId;
	union {
		//=== Operators (Internal nodes):
		struct {
			Node *	left_operand;
			Node *	right_operand;
		} binary;

		struct {
			Node *	operand;
		} unary;

		//=== Primitives (Leaf nodes):
		struct {
			V4f		normal_and_distance;
			U32		material_id;
		} plane;

		struct {
			V3f		center;
			float	radius;
			U32		material_id;
		} sphere;

		//struct {
		//	V3f		center;
		//	F32		extent;	// half size
		//} cube;

		struct {
			V3f		center;
			V3f		extent;	// half size
			U32		material_id;
		} box;

		struct {
			V3f		origin;
			float	radius;
			U32		material_id;
		} inf_cylinder;

		// Torus in the XY-plane
		struct {
			V3f		center;
			float	big_radius;		//!< the radius of the big circle (around the center)
			float	small_radius;	//!< the radius of the small revolving circle
			U32		material_id;
		} torus;

		struct {
			V3f		direction;
			float	radius;
			V3f		origin;
			float	height;	//!< half-height, starting from the origin
			U32		material_id;
		} cylinder;

		struct {
			V3f		coord_scale;
			U32		material_id;
		} sine_wave;

		struct {
			V3f		coord_scale;
			U32		material_id;
		} gyroid;
	};
};

enum UpperDesignLimits {
	MAX_NODES = 128,
};


class BlobTree {
	//TPtr< Node >		_root;
	FreeListAllocator	_nodePool;
public:
	BlobTree();

	Node &	allocNode() { return *(Node*) _nodePool.AllocateItem(); }
	//compile
};

class BlobTreeBuilder: public TGlobal<BlobTreeBuilder>
{
public:
	FreeListAllocator	_node_pool;
public:
	BlobTreeBuilder();
	~BlobTreeBuilder();

	ERet Initialize();
	void Shutdown();

	void bindToLua( lua_State * L );
	void unbindFromLua( lua_State * L );
};

}//namespace implicit
