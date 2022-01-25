// SIMD-Accelerated SDF/BlobTree evaluation.
#pragma once

#include <Base/Template/Containers/Blob.h>
#include <Utility/Implicit/BlobTree.h>

namespace VX
{
	struct ADebugView;
}//namespace VX

namespace SDF {

#if 0
typedef __m128	SIMDf;	// V4f_SoA

/// number of SIMD lanes
#define SIMD_WIDTH	(4)
#define ALIGNMENT	(16)

#define SIMD_ALIGN(X)	tbALIGN( (X), ALIGNMENT )


/// doesn't have any functions, because in C/C++ a function cannot return an array
template< typename ARRAY >
struct SoA_3D {
	union {
		struct {
			ARRAY	xs;
			ARRAY	ys;
			ARRAY	zs;
		};
		ARRAY		v[3];
	};
};


/// maximum batch size
enum { MAX_POINTS = 64 };

typedef mxALIGN_BY_CACHELINE U32 Array_of_I32[MAX_POINTS];
typedef mxALIGN_BY_CACHELINE F32 Array_of_F32[MAX_POINTS];

// 3D Vectors in SoA (Structure of Arrays) arrangement:
typedef mxALIGN_BY_CACHELINE SoA_3D< Array_of_I32 >	V3i_SoA;
typedef mxALIGN_BY_CACHELINE SoA_3D< Array_of_F32 >	V3f_SoA;

static mxFORCEINLINE
UINT getNumBatches( UINT element_count_, UINT batch_size_ )
{
	return (element_count_ / batch_size_) + !!(element_count_ % batch_size_);
}

//#define STORE_INT4()	_mm_store_si128((__m128i*)ptr,v)



typedef AABBf	AABB;

const U32 NODE_TYPE_MASK = 0x1F;

typedef U32 NodeIndex;

const NodeIndex NODE_INDEX_MASK = (1u << 9u) - 1u;

struct BlobTreeSoA {
	///
	U32 *	types;

	// SIMD-aligned pointers:
	AABBf *	aabbs;	//!< bounding boxes for both internal operators and leaf primitives

	SIMDf *	params;	//!< vector parameters both for internal operators and leaf primitives

	//U32 *	materials;
	//U32 *	colors;

	U32		numOps;	//!< total number of operators (or internal nodes in the BlobTree)
	U32		numPrims;	//!< total number of leaf primitives

	U32		numOpParams;
	U32		numPrimParams;

	UINT	last_index;
	NwBlob	storage;	// owns dynamically allocated data

	float	user_param;	// for debugging

#if 0
	mxPREALIGN(SIMD_ALIGN) struct Primitives {
		U32	type_and_transformId[MAX_NODES];	//!< primitive type and transform index

		float posX[MAX_NODES];
		float posY[MAX_NODES];
		float posZ[MAX_NODES];

		float colorX[MAX_NODES];
		float colorY[MAX_NODES];
		float colorZ[MAX_NODES];

		// AABBs in SoA:
		float aabbMinX[MAX_NODES];
		float aabbMinY[MAX_NODES];
		float aabbMinZ[MAX_NODES];
		float aabbMaxX[MAX_NODES];
		float aabbMaxY[MAX_NODES];
		float aabbMaxZ[MAX_NODES];
	} mxPOSTALIGN(SIMD_ALIGN);

	mxPREALIGN(SIMD_ALIGN) struct Operators {
		///
		U32	type_and_flags[MAX_NODES];

		/// AABBs in SoA:
		float aabbMinX[MAX_NODES];
		float aabbMinY[MAX_NODES];
		float aabbMinZ[MAX_NODES];
		float aabbMaxX[MAX_NODES];
		float aabbMaxY[MAX_NODES];
		float aabbMaxZ[MAX_NODES];

		U32		count;	//!< number of operators (or internal nodes in the BlobTree)
	} mxPOSTALIGN(SIMD_ALIGN);

	Primitives	leafs;
	Operators	nodes;
#endif

	BlobTreeSoA( AllocatorI & allocator ) : storage( allocator ) {}

	static U32 PackOp( const NodeType opType_, const U32 opParamIdx_, const NodeIndex leftChildIdx_, const NodeIndex rightChildIdx_ ) {
		return (rightChildIdx_ << 23) | (leftChildIdx_ << 14) | (opParamIdx_ << 5) | opType_;
	}
	static mxFORCEINLINE void UnpackOperator( U32 opCode_, U32 &opParamIdx_, NodeIndex &leftChildIdx_, NodeIndex &rightChildIdx_ ) {
		opParamIdx_ = (opCode_ >> 6) & NODE_INDEX_MASK;
		leftChildIdx_ = (opCode_ >> 14) & NODE_INDEX_MASK;
		rightChildIdx_ = (opCode_ >> 23) & NODE_INDEX_MASK;
	}

	static U32 PackPrimitive( const NodeType opType_, const U32 opParamIdx_ ) {
		return (opParamIdx_ << 5) | opType_;
	}
	static mxFORCEINLINE void unpackPrimitive( U32 primitive_id_, U32 &first_parameter_index_ ) {
		first_parameter_index_ = (primitive_id_ >> 5) & 65535;
	}
};

class BlobTreeCompiler
{
	DynamicArray< Node* >	_nodes;
	DynamicArray< Node* >	_leaves;

public:
	BlobTreeCompiler( AllocatorI & scratchpad );

	/// Linearizes the blob tree into SoA format and flattens primitive transforms.
	ERet compile(
		const Node* root,
		BlobTreeSoA &output_
		);

private:
	void gatherNodesAndLeafs_recursive( const Node* root );
};

//class Evaluator {
//	const BlobTreeSoA &	_tree;
//public:
//	Evaluator( const BlobTreeSoA& tree_ );
//
//	/// Computes field values for several points simultaneously using SIMD instructions.
//	/// @param posX_ x coordinates for all points in SoA form
//	/// @param posY_ y coordinates for all points in SoA form
//	/// @param posZ_ z coordinates for all points in SoA form
//	/// @param values_ output field values for all points
//	///
//	void calcFieldValues(
//		const SIMDf& posX_, const SIMDf& posY_, const SIMDf& posZ_,
//		SIMDf &values_
//	) const;
//
//	//void get_known_edge_intersections(
//	//	const SIMDf& start_pos_x_, const SIMDf& start_pos_y_, const SIMDf& start_pos_z_,
//	//	const SIMDf& end_pos_x_, const SIMDf& end_pos_y_, const SIMDf& end_pos_z_,
//	//	SIMDf *hermite_edges_
//	//) const;
//};

/// Computes distance field values for many points simultaneously using SIMD instructions.
/// @param positions_ x, y and z coordinates for all points in SoA form
/// @param distances_ output distance field values for all points
///
void evaluate(
			  const BlobTreeSoA& tree,
			  const AABBf& bounding_box,	//!< region of interest
			  const UINT number_of_points,
			  const V3f_SoA& positions_,
			  Array_of_F32 &distances_,
			  Array_of_I32 &materials_
			  );

void debugDrawBoundingBoxes(
			  const BlobTreeSoA& tree,
			  const AABBf& bounding_box,
			  VX::ADebugView& dbg_view
			  );

void calculateAmbientOcclusion(
							   const BlobTreeSoA& tree,
							   const UINT number_of_points,
							   const V3f_SoA& positions,
							   const float normal_step_delta,	// [0.5..1)
							   const V3f_SoA& normals,	// assumed normalized
							   const UINT number_of_steps,	// [4..32]
							   Array_of_F32 &AO_factors_	// 0 == vertex is unoccluded, 1 == fully occluded
							   );

void calculateGradients(
						const BlobTreeSoA& sdf,
						const UINT number_of_points,
						const V3f_SoA& positions,
						const AABBf& bounding_box,
						const float epsilon,
						V3f_SoA &gradients_
						);

void projectVerticesOntoSurface(
					 const BlobTreeSoA& sdf,
					 const UINT number_of_points,
					 V3f_SoA & positions,
					 const float epsilon,
					 const int _iterations
						);
#endif
}//namespace SDF
