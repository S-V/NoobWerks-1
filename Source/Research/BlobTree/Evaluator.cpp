// Signed Distance Fields primitives and sampling.
// TODO: Use interval arithmetic to conservatively test entire set of edges against node bboxes
#include "Base/Base.h"
#pragma hdrstop
//#include <algorithm>
//#include <Meshok/Volumes.h>
#include <BlobTree/Evaluator.h>


namespace SDF
{

#if 0

/// 
#define INFINITE_DISTANCE	(FLT_MAX)

#define EMPTY_MATERIAL_ID	(0)

/// Empty space/'Air' material must be zero!
#define EMPTY_MATERIAL_VEC	g_MM_Zero
//#define SOLID_MATERIAL_VEC	g_MM_One
#define DEFAULT_MATERIAL_VEC	_mm_castsi128_ps( REPLICATE_I32( 1 ) )

//#if defined(__SSE4_1__)
//	#define V4_SELECT( MASK, A, B )		(_mm_blend_ps( (A), (B), (MASK) ))
//#else
//	#define V4_SELECT( MASK, A, B )		(_mm_or_ps( _mm_and_ps( (A), (MASK) ), _mm_andnot_ps( (B), (MASK) ) ))
//#endif
#define V4_SELECT( MASK, A, B )	_mm_or_ps( _mm_and_ps( MASK, A ), _mm_andnot_ps( MASK, B ) )

//__m128i
//#define REPLICATE_I32( X )	_mm_shuffle_epi32( _mm_castps_si128( _mm_load_ss( (float*)&X ) ), _MM_SHUFFLE(0, 0, 0, 0) )
#define REPLICATE_I32( X )	_mm_set_epi32( X, X, X, X )

// _mm_castsi128_ps for bitwise cast from __m128i to SIMDf
// _mm_castps_si128 for bitwise cast from SIMDf to __m128i

#define V4_ABS( X )	_mm_and_ps( X, _mm_castsi128_ps( _mm_set1_epi32(0x7fffFfff) ) )




#define V4_SATURATE( V )	V4_MIN( V4_MAX( V, g_MM_Zero ), g_MM_One )


mxFORCEINLINE SIMDf REDUCE_MIN( const SIMDf& v ) {
  SIMDf temp = _mm_min_ps( mmPERMUTE_PS( v, _MM_SHUFFLE(1,0,3,2) ),v );
  return _mm_min_ps( mmPERMUTE_PS( v, _MM_SHUFFLE(2,3,0,1) ), v );
}

mxFORCEINLINE SIMDf REDUCE_MAX( const SIMDf& v ) {
  SIMDf temp = _mm_max_ps( mmPERMUTE_PS( v, _MM_SHUFFLE(1,0,3,2) ),v );
  return _mm_max_ps( mmPERMUTE_PS( v, _MM_SHUFFLE(2,3,0,1) ), v );
}



static mxFORCEINLINE
SIMDf vec2_length( const SIMDf& Xs, const SIMDf& Ys )
{
	const SIMDf Xs_squared = V4_MUL( Xs, Xs );
	const SIMDf Ys_squared = V4_MUL( Ys, Ys );
	const SIMDf lengths_squared = V4_ADD( Xs_squared, Ys_squared );
	const SIMDf lengths = _mm_sqrt_ps( lengths_squared );
	return lengths;
}

static mxFORCEINLINE
SIMDf vec3_length( const SIMDf& Xs, const SIMDf& Ys, const SIMDf& Zs )
{
	const SIMDf Xs_squared = V4_MUL( Xs, Xs );
	const SIMDf Ys_squared = V4_MUL( Ys, Ys );
	const SIMDf Zs_squared = V4_MUL( Zs, Zs );
	const SIMDf lengths_squared = V4_ADD( V4_ADD( Xs_squared, Ys_squared ), Zs_squared );
	const SIMDf lengths = _mm_sqrt_ps( lengths_squared );
	return lengths;
}

BlobTreeCompiler::BlobTreeCompiler( AllocatorI & scratchpad )
: _nodes( scratchpad ), _leaves( scratchpad )
{
}

struct BlobTreeAnalysisResult {
	UINT	numOps;
	UINT	numPrims;
	UINT	numOpParams;
	UINT	numPrimParams;
public:
	BlobTreeAnalysisResult() {
		mxZERO_OUT(*this);
	}
};

static void analyzeBlobTree_recursive(
									  const Node* node,
									  BlobTreeAnalysisResult & result_
									  )
{
	const bool isOperator = (node->type < NT_FIRST_SHAPE_TYPE);
	if( isOperator ) {
		result_.numOps++;
		analyzeBlobTree_recursive( node->binary.left_operand, result_ );
		analyzeBlobTree_recursive( node->binary.right_operand, result_ );
	} else {
		result_.numPrims++;
		switch( node->type )
		{
		case NT_PLANE:
		case NT_SPHERE:
			result_.numPrimParams += 2;
			break;
		case NT_BOX:
			result_.numPrimParams += 2;
			break;
		case NT_INF_CYLINDER:
			result_.numPrimParams += 2;
			break;
		case NT_TORUS:
			result_.numPrimParams += 2;
			break;
		case NT_SINE_WAVE:
		case NT_GYROID:
			result_.numPrimParams += 1;
			break;
		mxDEFAULT_UNREACHABLE(;);
		}//switch
	}
}

static NodeIndex linearizeBlobTree_recursive(
										const BlobTreeAnalysisResult& tree_info,
										const Node* node,
										BlobTreeSoA &output_
										)
{
	const bool isOperator = (node->type < NT_FIRST_SHAPE_TYPE);
	if( isOperator )
	{
		const NodeIndex nodeOpIdx = output_.last_index++;
		output_.numOps++;

		const NodeIndex leftChildIdx = linearizeBlobTree_recursive( tree_info, node->binary.left_operand, output_ );
		const NodeIndex rightChildIdx = linearizeBlobTree_recursive( tree_info, node->binary.right_operand, output_ );
		const AABBf& left_child_bounds = output_.aabbs[ leftChildIdx ];
		const AABBf& right_child_bounds = output_.aabbs[ rightChildIdx ];
		
		output_.types[ nodeOpIdx ] = BlobTreeSoA::PackOp( node->type, 0, leftChildIdx, rightChildIdx );

		switch( node->type )
		{
		case NT_UNION:
			output_.aabbs[ nodeOpIdx ] = AABBf::getUnion( left_child_bounds, right_child_bounds );
			break;

		case NT_DIFFERENCE:
			output_.aabbs[ nodeOpIdx ] = left_child_bounds;
			break;

		case NT_INTERSECTION:
			output_.aabbs[ nodeOpIdx ] = AABBf::getIntersection( left_child_bounds, right_child_bounds );
			break;

		mxDEFAULT_UNREACHABLE(;);
		}

		return nodeOpIdx;
	}
	else
	{
		const NodeIndex primitive_index = output_.last_index++;
		output_.numPrims++;

		const U32 parameter_idx = output_.numPrimParams;

		output_.types[ primitive_index ] = BlobTreeSoA::PackPrimitive( node->type, parameter_idx );

		switch( node->type )
		{
		case NT_PLANE:
			{
				output_.aabbs[ primitive_index ] = AABBf::make( CV3f(-HUGE_VAL), CV3f(+HUGE_VAL) );
				output_.params[ parameter_idx + 0 ] = Vector4_Set( node->plane.normal_and_distance );
				output_.params[ parameter_idx + 1 ] = _mm_castsi128_ps(REPLICATE_I32( node->plane.material_id ));
				output_.numPrimParams += 2;
			} break;

		case NT_SPHERE:
			{
				output_.aabbs[ primitive_index ] = AABBf::fromSphere( node->sphere.center, node->sphere.radius );
				output_.params[ parameter_idx + 0 ] = Vector4_Set( node->sphere.center, node->sphere.radius );
				output_.params[ parameter_idx + 1 ] = _mm_castsi128_ps(REPLICATE_I32( node->sphere.material_id ));
				output_.numPrimParams += 2;
			} break;

		case NT_BOX:
			{
				output_.aabbs[ primitive_index ] = AABBf::make( node->box.center - node->box.extent, node->box.center + node->box.extent );

				//HACK:
				//output_.params[ parameter_idx + 0 ] = Vector4_Set( node->box.center, node->box.material_id );
				__m128i	tmp;
				tmp.m128i_i32[0] = FloatU32(node->box.center.x);
				tmp.m128i_i32[1] = FloatU32(node->box.center.y);
				tmp.m128i_i32[2] = FloatU32(node->box.center.z);
				tmp.m128i_i32[3] = node->box.material_id;
				output_.params[ parameter_idx + 0 ] = _mm_castsi128_ps(tmp);

				output_.params[ parameter_idx + 1 ] = Vector4_Set( node->box.extent, 0 );
				output_.numPrimParams += 2;
			} break;

		case NT_INF_CYLINDER:
			{
				const CV3f cylAabbMin(
					node->inf_cylinder.origin.x - node->inf_cylinder.radius,
					node->inf_cylinder.origin.y - node->inf_cylinder.radius,
					-HUGE_VAL
					);
				const CV3f cylAabbMax(
					node->inf_cylinder.origin.x + node->inf_cylinder.radius,
					node->inf_cylinder.origin.y + node->inf_cylinder.radius,
					+HUGE_VAL
					);
				output_.aabbs[ primitive_index ] = AABBf::make( cylAabbMin, cylAabbMax );
				output_.params[ parameter_idx + 0 ] = Vector4_Set( node->inf_cylinder.origin, node->inf_cylinder.radius );
				output_.params[ parameter_idx + 1 ] = _mm_castsi128_ps(REPLICATE_I32( node->inf_cylinder.material_id ));
				output_.numPrimParams += 2;
			} break;

		case NT_TORUS:
			{
				const float torus_outer_radius = node->torus.big_radius + node->torus.small_radius;

				const CV3f torusAabbMin(
					node->torus.center.x - torus_outer_radius,
					node->torus.center.y - torus_outer_radius,
					node->torus.center.z - node->torus.small_radius
					);
				const CV3f torusAabbMax(
					node->torus.center.x + torus_outer_radius,
					node->torus.center.y + torus_outer_radius,
					node->torus.center.z + node->torus.small_radius
					);
				output_.aabbs[ primitive_index ] = AABBf::make( torusAabbMin, torusAabbMax );

				//HACK:
				//output_.params[ parameter_idx + 0 ] = Vector4_Set( node->torus.center, node->torus.material_id );
				__m128i	tmp;
				tmp.m128i_i32[0] = FloatU32(node->torus.center.x);
				tmp.m128i_i32[1] = FloatU32(node->torus.center.y);
				tmp.m128i_i32[2] = FloatU32(node->torus.center.z);
				tmp.m128i_i32[3] = node->torus.material_id;
				output_.params[ parameter_idx + 0 ] = _mm_castsi128_ps(tmp);

				output_.params[ parameter_idx + 1 ] = Vector4_Set( node->torus.big_radius, node->torus.small_radius, 0, 0 );
				output_.numPrimParams += 2;
			} break;

		case NT_SINE_WAVE:
			{
				mxTODO("calc precise bounds");
				output_.aabbs[ primitive_index ] = AABBf::make( CV3f(-HUGE_VAL), CV3f(+HUGE_VAL) );
				output_.params[ parameter_idx ] = Vector4_Set( node->sine_wave.coord_scale, node->sine_wave.material_id );
				output_.numPrimParams += 1;
			} break;

		case NT_GYROID:
			{
				mxTODO("calc precise bounds");
				output_.aabbs[ primitive_index ] = AABBf::make( CV3f(-HUGE_VAL), CV3f(+HUGE_VAL) );
				output_.params[ parameter_idx ] = Vector4_Set( node->gyroid.coord_scale, node->gyroid.material_id );
				output_.numPrimParams += 1;
			} break;

			mxDEFAULT_UNREACHABLE(;);
		}//switch

		return primitive_index;
	}
}

ERet BlobTreeCompiler::compile(
	const Node* root,
	BlobTreeSoA &output_
	)
{
	BlobTreeAnalysisResult	treeInfo;
	analyzeBlobTree_recursive( root, treeInfo );

	const UINT numOpsAndPrims = treeInfo.numOps + treeInfo.numPrims;
	const UINT total_num_params = treeInfo.numOpParams + treeInfo.numPrimParams;

	const size_t size_of_types = sizeof(output_.types[0]) * numOpsAndPrims;
	const size_t size_of_aabbs = sizeof(output_.aabbs[0]) * numOpsAndPrims;
	const size_t size_of_params = sizeof(output_.params[0]) * total_num_params;

	const size_t aligned_offset_of_types = 0;
	const size_t aligned_offset_of_aabbs = SIMD_ALIGN( size_of_types );
	const size_t aligned_offset_of_params = aligned_offset_of_aabbs + SIMD_ALIGN( size_of_aabbs );

	const size_t total_mem_to_alloc = aligned_offset_of_params + SIMD_ALIGN( size_of_params );

	mxDO(output_.storage.setNum( total_mem_to_alloc ));

	void* mem_buf_start = output_.storage.raw();

	output_.types = (U32*) mxAddByteOffset( mem_buf_start, aligned_offset_of_types );
	output_.aabbs = (AABBf*) mxAddByteOffset( mem_buf_start, aligned_offset_of_aabbs );
	output_.params = (SIMDf*) mxAddByteOffset( mem_buf_start, aligned_offset_of_params );
	output_.numOps = 0;
	output_.numPrims = 0;
	output_.numOpParams = 0;
	output_.numPrimParams = 0;
	output_.last_index = 0;

	linearizeBlobTree_recursive( treeInfo, root, output_ );
	//updateBoundingBoxes_recursive( treeInfo, root, output_ );

	return ALL_OK;
}

#if 0
	// Check if all points are outside the operator's bounding box.
	const AABBf& bounds = tree.aabbs[ nodeIdx_ ];
	const SIMDf nodeMinX = V4_REPLICATE( bounds.mins.x );
	const SIMDf nodeMinY = V4_REPLICATE( bounds.mins.y );
	const SIMDf nodeMinZ = V4_REPLICATE( bounds.mins.z );
	const SIMDf nodeMaxX = V4_REPLICATE( bounds.maxs.x );
	const SIMDf nodeMaxY = V4_REPLICATE( bounds.maxs.y );
	const SIMDf nodeMaxZ = V4_REPLICATE( bounds.maxs.z );
	//const SIMDf nodeMinX = V4_REPLICATE( tree.nodes.aabbMinX[ nodeIdx_ ] );
	//const SIMDf nodeMinY = V4_REPLICATE( tree.nodes.aabbMinY[ nodeIdx_ ] );
	//const SIMDf nodeMinZ = V4_REPLICATE( tree.nodes.aabbMinZ[ nodeIdx_ ] );
	//const SIMDf nodeMaxX = V4_REPLICATE( tree.nodes.aabbMaxX[ nodeIdx_ ] );
	//const SIMDf nodeMaxY = V4_REPLICATE( tree.nodes.aabbMaxY[ nodeIdx_ ] );
	//const SIMDf nodeMaxZ = V4_REPLICATE( tree.nodes.aabbMaxZ[ nodeIdx_ ] );

	//SIMDf	all_inside = g_MM_AllMask.m128;	// {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
	const SIMDf insideOnAxisX = V4_AND( V4_CMPLE( nodeMinX, posX_ ), V4_CMPLE( posX_, nodeMaxX ) );
	const SIMDf insideOnAxisY = V4_AND( V4_CMPLE( nodeMinY, posY_ ), V4_CMPLE( posY_, nodeMaxY ) );
	const SIMDf insideOnAxisZ = V4_AND( V4_CMPLE( nodeMinZ, posZ_ ), V4_CMPLE( posZ_, nodeMaxZ ) );
	const SIMDf insideBoxMask = V4_AND( V4_AND( insideOnAxisX, insideOnAxisY ), insideOnAxisZ );
#endif

// e.g. on 8-wide SIMD processor (e.g. AVX1) and 32-point packets
// 4 SIMD instructions are required to perform SDF evaluation on all points in packet
enum { MAX_PACKETS = MAX_POINTS / SIMD_WIDTH };

typedef mxALIGN_BY_CACHELINE __m128i	SIMDi_Array[MAX_PACKETS];
typedef mxALIGN_BY_CACHELINE SIMDf		SIMDf_Array[MAX_PACKETS];

typedef mxALIGN_BY_CACHELINE SoA_3D< SIMDf_Array >	V3_SIMDf_SoA;

//#define mxLOOP( VAR, NUM )		for( VAR = 0; VAR < NUM; VAR++ )


static mxFORCEINLINE
void initializePackets( const UINT number_of_packets, SIMDf_Array &distances_, SIMDf_Array &materials_ )
{
	const SIMDf infinite_distance = V4_REPLICATE(INFINITE_DISTANCE);
	// Initialize all the distances to FLT_MAX and materials to EMPTY_SPACE.
	mxUINT_LOOP_i( number_of_packets )
	{
		distances_[ i ] = infinite_distance;
		materials_[ i ] = EMPTY_MATERIAL_VEC;
	}
}

static
void evaluate_r(
			  const BlobTreeSoA& tree,
			  const UINT current_node,
			  const AABBf& bounding_box,
			  const UINT number_of_packets,
			  const V3_SIMDf_SoA& positions,
			  SIMDf_Array &distances_,
			  SIMDf_Array &materials_
			  )
{
	const AABBf& node_bounds = tree.aabbs[ current_node ];

	if( !node_bounds.intersects( bounding_box ) ) {
		return;
	}

	const U32 typeAndFlags = tree.types[ current_node ];

	const NodeType nodeType = NodeType( typeAndFlags & NODE_TYPE_MASK );

	// If at least one point is inside the node's AABB:
	if(1)// V4_MOVEMASK( insideBoxMask ) )
	{
		const bool isOperator = (nodeType < NT_FIRST_SHAPE_TYPE);
		if( isOperator )
		{
			//const bool isBinaryOp = true;
			U32	opParamIdx, leftChildIdx, rightChildIdx;
			BlobTreeSoA::UnpackOperator( typeAndFlags, opParamIdx, leftChildIdx, rightChildIdx );

			SIMDf_Array left_child_distances, left_child_materials;
			SIMDf_Array right_child_distances, right_child_materials;
			initializePackets( number_of_packets, left_child_distances, left_child_materials );
			initializePackets( number_of_packets, right_child_distances, right_child_materials );

			evaluate_r( tree, leftChildIdx, bounding_box, number_of_packets, positions, left_child_distances, left_child_materials );
			evaluate_r( tree, rightChildIdx, bounding_box, number_of_packets, positions, right_child_distances, right_child_materials );

			switch( nodeType )
			{
			case NT_UNION:
				mxUINT_LOOP_i( number_of_packets ) {
					const SIMDf distances = V4_MIN( left_child_distances[i], right_child_distances[i] );
					distances_[i] = distances;
					materials_[i] = V4_MAX( left_child_materials[i], right_child_materials[i] );
				}
				break;

			case NT_DIFFERENCE:
				mxUINT_LOOP_i( number_of_packets ) {
					const SIMDf distances = V4_MAX( left_child_distances[i], V4_NEGATE(right_child_distances[i]) );
					distances_[i] = distances;
					const SIMDf mask = V4_CMPLE( distances, g_MM_Zero );
					materials_[i] = V4_SELECT( mask, left_child_materials[i], EMPTY_MATERIAL_VEC );
				}
				break;

			case NT_INTERSECTION:
				mxUINT_LOOP_i( number_of_packets ) {
					const SIMDf distances = V4_MAX( left_child_distances[i], right_child_distances[i] );
					distances_[i] = distances;
					//materials_[i] = V4_MIN( left_child_materials[i], right_child_materials[i] );
					const SIMDf mask = V4_CMPLE( distances, g_MM_Zero );
					materials_[i] = V4_SELECT( mask, DEFAULT_MATERIAL_VEC, EMPTY_MATERIAL_VEC );
				}
				break;

				mxDEFAULT_UNREACHABLE(;);
			}
		}
		else
		{
			// This is a leaf primitive.
			U32	first_parameter_index;
			BlobTreeSoA::unpackPrimitive( typeAndFlags, first_parameter_index );

			switch( nodeType )
			{
			case NT_PLANE:
				{
					const SIMDf plane = tree.params[ first_parameter_index ];

					const SIMDf plane_normal_x	= mmSPLAT_X( plane );
					const SIMDf plane_normal_y	= mmSPLAT_Y( plane );
					const SIMDf plane_normal_z	= mmSPLAT_Z( plane );
					const SIMDf plane_d			= mmSPLAT_W( plane );

					const SIMDf primitive_material = tree.params[ first_parameter_index + 1 ];

					mxUINT_LOOP_i( number_of_packets )
					{
						const SIMDf tmp0 = V4_ADD( V4_MUL( plane_normal_x, positions.xs[i] ), V4_MUL( plane_normal_y, positions.ys[i] ) );
						const SIMDf tmp1 = V4_MAD( plane_normal_z, positions.zs[i], plane_d );

						const SIMDf distances = V4_ADD( tmp0, tmp1 );	// distances from the points to the plane
						distances_[i] = distances;

						const SIMDf mask = V4_CMPLE( distances, g_MM_Zero );
						materials_[i] = V4_SELECT( mask, primitive_material, materials_[i] );
					}
				} break;

			case NT_SPHERE:
				{
					const SIMDf sphere = tree.params[ first_parameter_index ];

					const SIMDf sphere_center_x	= mmSPLAT_X( sphere );
					const SIMDf sphere_center_y	= mmSPLAT_Y( sphere );
					const SIMDf sphere_center_z	= mmSPLAT_Z( sphere );
					const SIMDf sphere_radius	= mmSPLAT_W( sphere );

					const SIMDf primitive_material = tree.params[ first_parameter_index + 1 ];

					mxUINT_LOOP_i( number_of_packets )
					{
						const SIMDf local_pos_x = V4_SUB( positions.xs[i], sphere_center_x );
						const SIMDf local_pos_y = V4_SUB( positions.ys[i], sphere_center_y );
						const SIMDf local_pos_z = V4_SUB( positions.zs[i], sphere_center_z );

						const SIMDf lengths = vec3_length( local_pos_x, local_pos_y, local_pos_z );

						const SIMDf distances = V4_SUB( lengths, sphere_radius );
						distances_[i] = distances;

						const SIMDf mask = V4_CMPLE( distances, g_MM_Zero );
						materials_[i] = V4_SELECT( mask, primitive_material, materials_[i] );
					}
				} break;

			case NT_BOX:
				{
					const SIMDf param0 = tree.params[ first_parameter_index ];
					const SIMDf box_center_x = mmSPLAT_X( param0 );
					const SIMDf box_center_y = mmSPLAT_Y( param0 );
					const SIMDf box_center_z = mmSPLAT_Z( param0 );

					const SIMDf param1 = tree.params[ first_parameter_index+1 ];
					const SIMDf box_extent_xs = mmSPLAT_X( param1 );
					const SIMDf box_extent_ys = mmSPLAT_Y( param1 );
					const SIMDf box_extent_zs = mmSPLAT_Z( param1 );

					const SIMDf primitive_material = mmSPLAT_W( param0 );
					/*
					// Box: correct distance to corners
					float fBox(vec3 p, vec3 b) {
					vec3 d = abs(p) - b;
					return length(max(d, vec3(0))) + vmax(min(d, vec3(0)));	// where: float vmax(vec3 v) -> max(max(v.x, v.y), v.z)
					}
					*/
					mxUINT_LOOP_i( number_of_packets )
					{
						const SIMDf local_pos_x = V4_SUB( positions.xs[i], box_center_x );
						const SIMDf local_pos_y = V4_SUB( positions.ys[i], box_center_y );
						const SIMDf local_pos_z = V4_SUB( positions.zs[i], box_center_z );

						const SIMDf abs_xs = V4_ABS( local_pos_x );
						const SIMDf abs_ys = V4_ABS( local_pos_y );
						const SIMDf abs_zs = V4_ABS( local_pos_z );

						const SIMDf d_xs = V4_SUB( abs_xs, box_extent_xs );
						const SIMDf d_ys = V4_SUB( abs_ys, box_extent_ys );
						const SIMDf d_zs = V4_SUB( abs_zs, box_extent_zs );

						//
						const SIMDf max_xs = V4_MAX( d_xs, g_MM_Zero );
						const SIMDf max_ys = V4_MAX( d_ys, g_MM_Zero );
						const SIMDf max_zs = V4_MAX( d_zs, g_MM_Zero );

						const SIMDf lengths = vec3_length( max_xs, max_ys, max_zs );

						//
						const SIMDf min_xs = V4_MIN( d_xs, g_MM_Zero );
						const SIMDf min_ys = V4_MIN( d_ys, g_MM_Zero );
						const SIMDf min_zs = V4_MIN( d_zs, g_MM_Zero );

						const SIMDf vmax = V4_MAX( min_xs, V4_MAX( min_ys, min_zs ) );

						//
						const SIMDf distances = V4_ADD( lengths, vmax );
						distances_[i] = distances;

						const SIMDf mask = V4_CMPLE( distances, g_MM_Zero );
						materials_[i] = V4_SELECT( mask, primitive_material, materials_[i] );
					}
				} break;

			case NT_INF_CYLINDER:
				{
					const SIMDf cyl_params = tree.params[ first_parameter_index ];
					const SIMDf cyl_center_x	= mmSPLAT_X( cyl_params );
					const SIMDf cyl_center_y	= mmSPLAT_Y( cyl_params );
					const SIMDf cyl_center_z	= mmSPLAT_Z( cyl_params );
					const SIMDf cyl_radius		= mmSPLAT_W( cyl_params );

					const SIMDf primitive_material = tree.params[ first_parameter_index + 1 ];
					//
					mxUINT_LOOP_i( number_of_packets )
					{
						const SIMDf local_pos_x = V4_SUB( positions.xs[i], cyl_center_x );
						const SIMDf local_pos_y = V4_SUB( positions.ys[i], cyl_center_y );

						const SIMDf tmp = vec2_length( local_pos_x, local_pos_y );
						//
						const SIMDf distances = V4_SUB( tmp, cyl_radius );
						distances_[i] = distances;

						const SIMDf mask = V4_CMPLE( distances, g_MM_Zero );
						materials_[i] = V4_SELECT( mask, primitive_material, materials_[i] );
					}
				} break;

			case NT_TORUS:
				{
					const SIMDf param0 = tree.params[ first_parameter_index ];
					const SIMDf torus_center_x = mmSPLAT_X( param0 );
					const SIMDf torus_center_y = mmSPLAT_Y( param0 );
					const SIMDf torus_center_z = mmSPLAT_Z( param0 );
					const SIMDf primitive_material = mmSPLAT_W( param0 );

					const SIMDf param1 = tree.params[ first_parameter_index+1 ];
					const SIMDf torus_outer_radius = mmSPLAT_X( param1 );	// the radius from the center to the outer rim
					const SIMDf torus_inner_radius = mmSPLAT_Y( param1 );	// the radius of the revolving circle

					//
					mxUINT_LOOP_i( number_of_packets )
					{
						const SIMDf local_pos_x = V4_SUB( positions.xs[i], torus_center_x );
						const SIMDf local_pos_y = V4_SUB( positions.ys[i], torus_center_y );
						const SIMDf local_pos_z = V4_SUB( positions.zs[i], torus_center_z );
						/*
						// Torus in the XZ-plane
						float fTorus(vec3 p, float smallRadius, float largeRadius) {
							return length(vec2(length(p.xz) - largeRadius, p.y)) - smallRadius;
						}
						*/
						const SIMDf tmp0 = vec2_length( local_pos_x, local_pos_y );
						const SIMDf tmp1 = V4_SUB( tmp0, torus_outer_radius );
						const SIMDf tmp2 = vec2_length( tmp1, local_pos_z );
						//
						const SIMDf distances = V4_SUB( tmp2, torus_inner_radius );
						distances_[i] = distances;

						const SIMDf mask = V4_CMPLE( distances, g_MM_Zero );
						materials_[i] = V4_SELECT( mask, primitive_material, materials_[i] );
					}
				} break;

			case NT_SINE_WAVE:
				{
					const SIMDf param0 = tree.params[ first_parameter_index ];
					const SIMDf x_scale = mmSPLAT_X( param0 );
					const SIMDf y_scale = mmSPLAT_Y( param0 );
					const SIMDf z_scale = mmSPLAT_Z( param0 );
					const SIMDf primitive_material = mmSPLAT_W( param0 );

					mxUINT_LOOP_i( number_of_packets )
					{
						const SIMDf x = V4_MUL( positions.xs[i], x_scale );
						const SIMDf y = V4_MUL( positions.ys[i], y_scale );
						const SIMDf z = V4_MUL( positions.zs[i], z_scale );

						SIMDf &distances = distances_[i];
						SIMDf &materials = materials_[i];
						mxUINT_LOOP_j( SIMD_WIDTH )
						{
							float height = sinf( sqrtf( squaref(x.m128_f32[j]) + squaref(y.m128_f32[j]) ) );

							distances.m128_f32[j] = z.m128_f32[j] - height;
							materials.m128_i32[j] = distances.m128_f32[j] <= 0 ? 1 : 0;
						}
					}
				} break;

			case NT_GYROID:
				{
					const SIMDf param0 = tree.params[ first_parameter_index ];
					const SIMDf x_scale = mmSPLAT_X( param0 );
					const SIMDf y_scale = mmSPLAT_Y( param0 );
					const SIMDf z_scale = mmSPLAT_Z( param0 );
					const SIMDf primitive_material = mmSPLAT_W( param0 );

					mxUINT_LOOP_i( number_of_packets )
					{
						const SIMDf x = V4_MUL( positions.xs[i], x_scale );
						const SIMDf y = V4_MUL( positions.ys[i], y_scale );
						const SIMDf z = V4_MUL( positions.zs[i], z_scale );

						SIMDf &distances = distances_[i];
						SIMDf &materials = materials_[i];
						mxUINT_LOOP_j( SIMD_WIDTH )
						{
							float sx, cx, sy, cy, sz, cz;
							mmSinCos( x.m128_f32[j], sx, cx );
							mmSinCos( y.m128_f32[j], sy, cy );
							mmSinCos( z.m128_f32[j], sz, cz );

							distances.m128_f32[j] = 2.0 * (cx * sy + cy * sz + cz * sx);
							materials.m128_i32[j] = distances.m128_f32[j] <= 0 ? 1 : 0;
						}
					}
				} break;

				mxDEFAULT_UNREACHABLE(;);
			}//switch
		}//If this is a primitive.
	}
}

void evaluate(
			  const BlobTreeSoA& tree,
			  const AABBf& bounding_box,
			  const UINT number_of_points,
			  const V3f_SoA& positions,
			  Array_of_F32 &distances_,
			  Array_of_I32 &materials_
			  )
{
	const V3_SIMDf_SoA& simd_positions = *reinterpret_cast< const V3_SIMDf_SoA* >( &positions );
	SIMDf_Array &simd_distances = *reinterpret_cast< SIMDf_Array* >( &distances_ );
	SIMDf_Array &simd_materials = *reinterpret_cast< SIMDf_Array* >( &materials_ );

	// Initialize all the distances to FLT_MAX and materials to EMPTY_SPACE.
	const UINT number_of_packets = getNumBatches( number_of_points, SIMD_WIDTH );
	initializePackets( number_of_packets, simd_distances, simd_materials );

	evaluate_r( tree, 0, bounding_box, number_of_packets, simd_positions, simd_distances, simd_materials );
}

static void debugDraw_r(
			  const BlobTreeSoA& tree,
			  const UINT current_node,
			  const AABBf& bounding_box,
			  VX::ADebugView& dbg_view
			  )
{
	const AABBf& node_bounds = tree.aabbs[ current_node ];

	if( !node_bounds.intersects( bounding_box ) ) {
		return;
	}

	const U32 typeAndFlags = tree.types[ current_node ];

	const NodeType nodeType = NodeType( typeAndFlags & NODE_TYPE_MASK );

	const bool isOperator = (nodeType < NT_FIRST_SHAPE_TYPE);
	if( isOperator )
	{
		//const bool isBinaryOp = true;
		U32	opParamIdx, leftChildIdx, rightChildIdx;
		BlobTreeSoA::UnpackOperator( typeAndFlags, opParamIdx, leftChildIdx, rightChildIdx );

		debugDraw_r( tree, leftChildIdx, bounding_box, dbg_view );
		debugDraw_r( tree, rightChildIdx, bounding_box, dbg_view );
	}
	else
	{
		// This is a leaf primitive.

		RGBAf	prim_color;
		switch( nodeType )
		{
		case NT_PLANE:		prim_color = RGBAf::BLUE; break;
		case NT_SPHERE:		prim_color = RGBAf::YELLOW; break;
		case NT_BOX:		prim_color = RGBAf::GREEN; break;
		case NT_CYLINDER:	prim_color = RGBAf::GRAY; break;
		case NT_TORUS:		prim_color = RGBAf::RED; break;
		default:			prim_color = RGBAf::WHITE; break;
		}

		dbg_view.addBox( node_bounds, prim_color );
	}
}

void debugDrawBoundingBoxes(
			  const BlobTreeSoA& tree,
			  const AABBf& bounding_box,
			  VX::ADebugView& dbg_view
			  )
{
	debugDraw_r( tree, 0, bounding_box, dbg_view );
}

void calculateAmbientOcclusion(
							   const BlobTreeSoA& tree,
							   const UINT number_of_points,
							   const V3f_SoA& positions,
							   const float normal_step_delta_,
							   const V3f_SoA& normals_,
							   const UINT number_of_steps_,
							   Array_of_F32 &AO_factors_
							   )
{
	const UINT number_of_packets = getNumBatches( number_of_points, SIMD_WIDTH );

	const V3_SIMDf_SoA& simd_positions = *reinterpret_cast< const V3_SIMDf_SoA* >( &positions );
	const V3_SIMDf_SoA& simd_normals = *reinterpret_cast< const V3_SIMDf_SoA* >( &normals_ );

	SIMDf_Array &AO_factors = *reinterpret_cast< SIMDf_Array* >( &AO_factors_ );

	mxUINT_LOOP_i( number_of_packets ) {
		AO_factors[ i ] = _mm_setzero_ps();
	}

	for( UINT iStep = 1; iStep <= number_of_steps_; iStep++ )
	{
		const float contribution = (1.0f / (1 << iStep));
		const SIMDf contribution_v = V4_REPLICATE( contribution );
		const float distance_along_normal = iStep * normal_step_delta_;
		const SIMDf distance_along_normal_v = V4_REPLICATE( distance_along_normal );

		//
		SoA_3D< SIMDf >	aabb_mins = { g_MM_FltMax.m128, g_MM_FltMax.m128, g_MM_FltMax.m128 };
		SoA_3D< SIMDf >	aabb_maxs = { g_MM_FltMin.m128, g_MM_FltMin.m128, g_MM_FltMin.m128 };

		//
		V3_SIMDf_SoA	sample_positions;

		mxUINT_LOOP_i( number_of_packets )
		{
			const SIMDf tempX = V4_MAD( simd_normals.xs[i], distance_along_normal_v, simd_positions.xs[i] );
			const SIMDf tempY = V4_MAD( simd_normals.ys[i], distance_along_normal_v, simd_positions.ys[i] );
			const SIMDf tempZ = V4_MAD( simd_normals.zs[i], distance_along_normal_v, simd_positions.zs[i] );

			sample_positions.xs[i] = tempX;
			sample_positions.ys[i] = tempY;
			sample_positions.zs[i] = tempZ;

			aabb_mins.xs = V4_MIN( aabb_mins.xs, tempX );
			aabb_mins.ys = V4_MIN( aabb_mins.ys, tempY );
			aabb_mins.zs = V4_MIN( aabb_mins.zs, tempZ );

			aabb_maxs.xs = V4_MAX( aabb_maxs.xs, tempX );
			aabb_maxs.ys = V4_MAX( aabb_maxs.ys, tempY );
			aabb_maxs.zs = V4_MAX( aabb_maxs.zs, tempZ );
		}

		//
		AABBf	packet_bounds;

		packet_bounds.min_corner.x = _mm_cvtss_f32( REDUCE_MIN( aabb_mins.xs ) );
		packet_bounds.min_corner.y = _mm_cvtss_f32( REDUCE_MIN( aabb_mins.ys ) );
		packet_bounds.min_corner.z = _mm_cvtss_f32( REDUCE_MIN( aabb_mins.zs ) );

		packet_bounds.max_corner.x = _mm_cvtss_f32( REDUCE_MAX( aabb_maxs.xs ) );
		packet_bounds.max_corner.y = _mm_cvtss_f32( REDUCE_MAX( aabb_maxs.ys ) );
		packet_bounds.max_corner.z = _mm_cvtss_f32( REDUCE_MAX( aabb_maxs.zs ) );

//packet_bounds.min_corner = CV3f( -FLT_MAX );
//packet_bounds.max_corner = CV3f( +FLT_MAX );
		//
		SIMDf_Array	sample_distances;
		SIMDf_Array	sample_materials;

		const SIMDf infinite_distance = g_MM_FltMax.m128;
		mxUINT_LOOP_i( number_of_packets ) {
			sample_distances[i] = infinite_distance;
		}

		evaluate_r( tree, 0, packet_bounds, number_of_packets, sample_positions, sample_distances, sample_materials );

		//
		mxUINT_LOOP_i( number_of_packets )
		{
			const SIMDf temp0 = V4_MUL( V4_SUB( distance_along_normal_v, sample_distances[i] ), contribution_v );
			const SIMDf temp1 = V4_MAX( temp0, g_MM_Zero );
			////SIMDf temp = V4_SATURATE( AO_factors[i] );
			AO_factors[i] = V4_ADD( AO_factors[i], temp1 );
		}
	}
}

void calculateGradients(
						const BlobTreeSoA& sdf_,
						const UINT number_of_points,
						const V3f_SoA& positions,
						const AABBf& bounding_box,
						const float epsilon_,
						V3f_SoA &gradients_
						)
{
	const UINT number_of_packets = getNumBatches( number_of_points, SIMD_WIDTH );

	const V3_SIMDf_SoA& simd_positions = *reinterpret_cast< const V3_SIMDf_SoA* >( &positions );
	V3_SIMDf_SoA &destination_gradients = *reinterpret_cast< V3_SIMDf_SoA* >( &gradients_ );

	//
	const SIMDf epsilon_v = V4_REPLICATE(epsilon_);

	Array_of_I32	unused_materials;

	//
	Array_of_F32	distances;
	evaluate( sdf_, bounding_box, number_of_points, positions, distances, unused_materials );

	//
	Array_of_F32	distances_pos_x;
	{
		V3_SIMDf_SoA	sample_positions;

		mxUINT_LOOP_i( number_of_packets )
		{
			sample_positions.xs[i] = V4_ADD( simd_positions.xs[i], epsilon_v );
			sample_positions.ys[i] = simd_positions.ys[i];
			sample_positions.zs[i] = simd_positions.zs[i];
		}

		evaluate( sdf_, bounding_box.expanded(CV3f(epsilon_, 0, 0)), number_of_points, (V3f_SoA&)sample_positions, distances_pos_x, unused_materials );
	}

	//
	Array_of_F32	distances_pos_y;
	{
		V3_SIMDf_SoA	sample_positions;

		mxUINT_LOOP_i( number_of_packets )
		{
			sample_positions.xs[i] = simd_positions.xs[i];
			sample_positions.ys[i] = V4_ADD( simd_positions.ys[i], epsilon_v );
			sample_positions.zs[i] = simd_positions.zs[i];
		}

		evaluate( sdf_, bounding_box.expanded(CV3f(0, epsilon_, 0)), number_of_points, (V3f_SoA&)sample_positions, distances_pos_y, unused_materials );
	}

	//
	Array_of_F32	distances_pos_z;
	{
		V3_SIMDf_SoA	sample_positions;

		mxUINT_LOOP_i( number_of_packets )
		{
			sample_positions.xs[i] = simd_positions.xs[i];
			sample_positions.ys[i] = simd_positions.ys[i];
			sample_positions.zs[i] = V4_ADD( simd_positions.zs[i], epsilon_v );
		}

		evaluate( sdf_, bounding_box.expanded(CV3f(0, 0, epsilon_)), number_of_points, (V3f_SoA&)sample_positions, distances_pos_z, unused_materials );
	}

	//
	mxUINT_LOOP_i( number_of_packets )
	{
		const SIMDf grad_x = V4_SUB( ((SIMDf_Array&)distances_pos_x)[i], ((SIMDf_Array&)distances)[i] );
		const SIMDf grad_y = V4_SUB( ((SIMDf_Array&)distances_pos_y)[i], ((SIMDf_Array&)distances)[i] );
		const SIMDf grad_z = V4_SUB( ((SIMDf_Array&)distances_pos_z)[i], ((SIMDf_Array&)distances)[i] );

		destination_gradients.xs[i] = grad_x;
		destination_gradients.ys[i] = grad_y;
		destination_gradients.zs[i] = grad_z;
		//const SIMDf lengths = vec3_length( grad_x, grad_y, grad_z );
		////
		//int a = 0;
	}
}


float dist_to_sphere(const Sphere16& sphere, const V3f& p) {
	return mmSqrt( (p-sphere.center).lengthSquared() ) - sphere.radius;
}

void projectVerticesOntoSurface(
					 const BlobTreeSoA& sdf_,
					 const UINT number_of_points,
					 V3f_SoA & positions,
					 const float epsilon_,
					 const int _iterations
						)
{
	const UINT number_of_packets = getNumBatches( number_of_points, SIMD_WIDTH );

	V3_SIMDf_SoA & simd_positions = *reinterpret_cast< V3_SIMDf_SoA* >( &positions );

//positions.xs[0] = -100;
//positions.ys[0] = 0;
//positions.zs[0] = 0;

//{
//	const float* pos_x = (float*) positions.xs;
//	const float* pos_y = (float*) positions.ys;
//	const float* pos_z = (float*) positions.zs;

//	mxUINT_LOOP_i( number_of_points )
//	{
//		VX::g_DbgView->addPoint( CV3f( pos_x[ i ], pos_y[ i ], pos_z[ i ] ), RGBAf::RED );
//	}
//}

	//
	const SIMDf epsilon_v = V4_REPLICATE(epsilon_);

	for( int iteration = 0; iteration < _iterations; iteration++ )
	{
		//
		SoA_3D< SIMDf >	aabb_mins = { g_MM_FltMax.m128, g_MM_FltMax.m128, g_MM_FltMax.m128 };
		SoA_3D< SIMDf >	aabb_maxs = { g_MM_FltMin.m128, g_MM_FltMin.m128, g_MM_FltMin.m128 };

		mxUINT_LOOP_i( number_of_packets )
		{
			const SIMDf tempX = simd_positions.xs[i];
			const SIMDf tempY = simd_positions.ys[i];
			const SIMDf tempZ = simd_positions.zs[i];

			aabb_mins.xs = V4_MIN( aabb_mins.xs, tempX );
			aabb_mins.ys = V4_MIN( aabb_mins.ys, tempY );
			aabb_mins.zs = V4_MIN( aabb_mins.zs, tempZ );

			aabb_maxs.xs = V4_MAX( aabb_maxs.xs, tempX );
			aabb_maxs.ys = V4_MAX( aabb_maxs.ys, tempY );
			aabb_maxs.zs = V4_MAX( aabb_maxs.zs, tempZ );
		}

		//
		AABBf	packet_bounds;

		packet_bounds.min_corner.x = _mm_cvtss_f32( REDUCE_MIN( aabb_mins.xs ) );
		packet_bounds.min_corner.y = _mm_cvtss_f32( REDUCE_MIN( aabb_mins.ys ) );
		packet_bounds.min_corner.z = _mm_cvtss_f32( REDUCE_MIN( aabb_mins.zs ) );

		packet_bounds.max_corner.x = _mm_cvtss_f32( REDUCE_MAX( aabb_maxs.xs ) );
		packet_bounds.max_corner.y = _mm_cvtss_f32( REDUCE_MAX( aabb_maxs.ys ) );
		packet_bounds.max_corner.z = _mm_cvtss_f32( REDUCE_MAX( aabb_maxs.zs ) );

packet_bounds.min_corner = CV3f( -FLT_MAX );
packet_bounds.max_corner = CV3f( +FLT_MAX );

		//
		SIMDf_Array	central_distances;
		SIMDf_Array	unused_materials;

		const SIMDf infinite_distance = g_MM_FltMax.m128;
		mxUINT_LOOP_i( number_of_packets ) {
			central_distances[i] = infinite_distance;
		}

		evaluate_r( sdf_, 0, packet_bounds, number_of_packets, simd_positions, central_distances, unused_materials );



		//
		Array_of_F32	distances_neg_dx;
		{
			V3_SIMDf_SoA	sample_positions;
			mxUINT_LOOP_i( number_of_packets ) {
				sample_positions.xs[i] = V4_SUB( simd_positions.xs[i], epsilon_v );
				sample_positions.ys[i] = simd_positions.ys[i];
				sample_positions.zs[i] = simd_positions.zs[i];
			}
			evaluate( sdf_, packet_bounds.expanded(CV3f(-epsilon_, 0, 0)), number_of_points, (V3f_SoA&)sample_positions, distances_neg_dx, (Array_of_I32&)unused_materials );
		}

		//
		Array_of_F32	distances_neg_dy;
		{
			V3_SIMDf_SoA	sample_positions;
			mxUINT_LOOP_i( number_of_packets ) {
				sample_positions.xs[i] = simd_positions.xs[i];
				sample_positions.ys[i] = V4_SUB( simd_positions.ys[i], epsilon_v );
				sample_positions.zs[i] = simd_positions.zs[i];
			}
			evaluate( sdf_, packet_bounds.expanded(CV3f(0, -epsilon_, 0)), number_of_points, (V3f_SoA&)sample_positions, distances_neg_dy, (Array_of_I32&)unused_materials );
		}

		//
		Array_of_F32	distances_neg_dz;
		{
			V3_SIMDf_SoA	sample_positions;
			mxUINT_LOOP_i( number_of_packets ) {
				sample_positions.xs[i] = simd_positions.xs[i];
				sample_positions.ys[i] = simd_positions.ys[i];
				sample_positions.zs[i] = V4_SUB( simd_positions.zs[i], epsilon_v );
			}
			evaluate( sdf_, packet_bounds.expanded(CV3f(0, 0, -epsilon_)), number_of_points, (V3f_SoA&)sample_positions, distances_neg_dz, (Array_of_I32&)unused_materials );
		}




		//
		Array_of_F32	distances_pos_dx;
		{
			V3_SIMDf_SoA	sample_positions;
			mxUINT_LOOP_i( number_of_packets ) {
				sample_positions.xs[i] = V4_ADD( simd_positions.xs[i], epsilon_v );
				sample_positions.ys[i] = simd_positions.ys[i];
				sample_positions.zs[i] = simd_positions.zs[i];
			}
			evaluate( sdf_, packet_bounds.expanded(CV3f(+epsilon_, 0, 0)), number_of_points, (V3f_SoA&)sample_positions, distances_pos_dx, (Array_of_I32&)unused_materials );
		}

		//
		Array_of_F32	distances_pos_dy;
		{
			V3_SIMDf_SoA	sample_positions;
			mxUINT_LOOP_i( number_of_packets ) {
				sample_positions.xs[i] = simd_positions.xs[i];
				sample_positions.ys[i] = V4_ADD( simd_positions.ys[i], epsilon_v );
				sample_positions.zs[i] = simd_positions.zs[i];
			}
			evaluate( sdf_, packet_bounds.expanded(CV3f(0, +epsilon_, 0)), number_of_points, (V3f_SoA&)sample_positions, distances_pos_dy, (Array_of_I32&)unused_materials );
		}

		//
		Array_of_F32	distances_pos_dz;
		{
			V3_SIMDf_SoA	sample_positions;
			mxUINT_LOOP_i( number_of_packets ) {
				sample_positions.xs[i] = simd_positions.xs[i];
				sample_positions.ys[i] = simd_positions.ys[i];
				sample_positions.zs[i] = V4_ADD( simd_positions.zs[i], epsilon_v );
			}
			evaluate( sdf_, packet_bounds.expanded(CV3f(0, 0, +epsilon_)), number_of_points, (V3f_SoA&)sample_positions, distances_pos_dz, (Array_of_I32&)unused_materials );
		}


//if( iteration == 0)
//{
//	const float* pos_x = (float*) positions.xs;
//	const float* pos_y = (float*) positions.ys;
//	const float* pos_z = (float*) positions.zs;
//
//	const float* dists = (float*) central_distances;
//	const float* dists_pos_x = (float*) distances_pos_dx;
//	const float* dists_pos_y = (float*) distances_pos_dy;
//	const float* dists_pos_z = (float*) distances_pos_dz;
//
//	mxUINT_LOOP_i( number_of_points )
//	{
//		const float grad_x = dists_pos_x[i] - dists[i];
//		const float grad_y = dists_pos_y[i] - dists[i];
//		const float grad_z = dists_pos_z[i] - dists[i];
//
//		CV3f	normal(grad_x, grad_y, grad_z);
//		float len = normal.normalize();
//
//		if( len == 0 ) {
//			VX::g_DbgView->addPoint( CV3f( pos_x[ i ], pos_y[ i ], pos_z[ i ] ), RGBAf::RED );
//		}
//
//		VX::g_DbgView->addPointWithNormal( CV3f( pos_x[ i ], pos_y[ i ], pos_z[ i ] ), normal );
//	}
//}

		//
		mxUINT_LOOP_i( number_of_packets )
		{
const SIMDf oldPosX = simd_positions.xs[i];
const SIMDf oldPosY = simd_positions.ys[i];
const SIMDf oldPosZ = simd_positions.zs[i];

			//const SIMDf_Array& distances_v = (SIMDf_Array&) central_distances;
			const SIMDf central_dist = central_distances[i];
 
			//const SIMDf grad_x = V4_SUB( ((SIMDf_Array&)distances_pos_dx)[i], central_dist );
			//const SIMDf grad_y = V4_SUB( ((SIMDf_Array&)distances_pos_dy)[i], central_dist );
			//const SIMDf grad_z = V4_SUB( ((SIMDf_Array&)distances_pos_dz)[i], central_dist );

			const SIMDf grad_x = V4_SUB( ((SIMDf_Array&)distances_pos_dx)[i], ((SIMDf_Array&)distances_neg_dx)[i] );
			const SIMDf grad_y = V4_SUB( ((SIMDf_Array&)distances_pos_dy)[i], ((SIMDf_Array&)distances_neg_dy)[i] );
			const SIMDf grad_z = V4_SUB( ((SIMDf_Array&)distances_pos_dz)[i], ((SIMDf_Array&)distances_neg_dz)[i] );

			//const SIMDf grad_x = ((SIMDf_Array&)distances_pos_dx)[i];
			//const SIMDf grad_y = ((SIMDf_Array&)distances_pos_dy)[i];
			//const SIMDf grad_z = ((SIMDf_Array&)distances_pos_dz)[i];

			const SIMDf grad_length = vec3_length( grad_x, grad_y, grad_z );	// can be zero near sharp corners!

			// Test for a divide by zero (Must be FP to detect -0.0)
			const SIMDf zero_mask = _mm_cmpneq_ps( _mm_setzero_ps(), grad_length );

			//const SIMDf grad_inv_length = _mm_rcp_ps( grad_length );
			const SIMDf temp = _mm_div_ps( g_MM_One, grad_length );

			// Any that are infinity, set to zero
			const SIMDf grad_inv_length = _mm_and_ps( temp, zero_mask );

bool ok = !IsNAN(grad_inv_length.m128_f32[0]);
mxASSERT( !IsNAN(grad_inv_length.m128_f32[0]) );
if( !ok ) {
	VX::g_DbgView->addPoint( CV3f( oldPosX.m128_f32[0], oldPosY.m128_f32[0], oldPosZ.m128_f32[0] ), RGBAf::RED );
}



			const SIMDf normal_x = V4_MUL( grad_x, grad_inv_length );
			const SIMDf normal_y = V4_MUL( grad_y, grad_inv_length );
			const SIMDf normal_z = V4_MUL( grad_z, grad_inv_length );

const SIMDf check = vec3_length( normal_x, normal_y, normal_z );

			//
			//const SIMDf neg_dist = V4_NEGATE( central_dist );
const SIMDf neg_dist = V4_MUL( V4_NEGATE( central_dist ), V4_REPLICATE(0.7f) );



			simd_positions.xs[i] = V4_MAD( normal_x, neg_dist, simd_positions.xs[i] );
			simd_positions.ys[i] = V4_MAD( normal_y, neg_dist, simd_positions.ys[i] );
			simd_positions.zs[i] = V4_MAD( normal_z, neg_dist, simd_positions.zs[i] );


const SIMDf newPosX = simd_positions.xs[i];
const SIMDf newPosY = simd_positions.ys[i];
const SIMDf newPosZ = simd_positions.zs[i];

	
Sphere16 bsphere = {CV3f(0), sdf_.user_param*0.9f};

const float oldDist0 = dist_to_sphere(bsphere, CV3f( oldPosX.m128_f32[0], oldPosY.m128_f32[0], oldPosZ.m128_f32[0] ));
const float oldDist1 = dist_to_sphere(bsphere, CV3f( oldPosX.m128_f32[1], oldPosY.m128_f32[1], oldPosZ.m128_f32[1] ));
const float oldDist2 = dist_to_sphere(bsphere, CV3f( oldPosX.m128_f32[2], oldPosY.m128_f32[2], oldPosZ.m128_f32[2] ));
const float oldDist3 = dist_to_sphere(bsphere, CV3f( oldPosX.m128_f32[3], oldPosY.m128_f32[3], oldPosZ.m128_f32[3] ));

const float newDist0 = dist_to_sphere(bsphere, CV3f( newPosX.m128_f32[0], newPosY.m128_f32[0], newPosZ.m128_f32[0] ));
const float newDist1 = dist_to_sphere(bsphere, CV3f( newPosX.m128_f32[1], newPosY.m128_f32[1], newPosZ.m128_f32[1] ));
const float newDist2 = dist_to_sphere(bsphere, CV3f( newPosX.m128_f32[2], newPosY.m128_f32[2], newPosZ.m128_f32[2] ));
const float newDist3 = dist_to_sphere(bsphere, CV3f( newPosX.m128_f32[3], newPosY.m128_f32[3], newPosZ.m128_f32[3] ));

int a = 0;

			//mxUINT_LOOP_j( number_of_packets )
//VX::g_DbgView->addPointWithNormal( CV3f( positions.xs[ i ], positions.ys[ i ], positions.zs[ i ] ), normal );

			//positions.xs[i] = V4_SUB( positions.xs[i], grad_x );
			//positions.ys[i] = V4_SUB( positions.ys[i], grad_y );
			//positions.zs[i] = V4_SUB( positions.zs[i], grad_z );

			//positions.xs[i] = V4_SUB( positions.xs[i], grad_x );
			//positions.ys[i] = V4_SUB( positions.ys[i], grad_y );
			//positions.zs[i] = V4_SUB( positions.zs[i], grad_z );
		}



		//
		SIMDf_Array	distances2;
		mxUINT_LOOP_i( number_of_packets ) {
			distances2[i] = infinite_distance;
		}

		evaluate_r( sdf_, 0, packet_bounds, number_of_packets, simd_positions, distances2, unused_materials );

		for( int i = 0; i < number_of_points; i++ )
		{
			float orig_dist = ((float*)central_distances)[i];
			float new_dist = ((float*)distances2)[i];

			mxASSERT(fabs(new_dist) <= fabs(orig_dist));
		}

	}//for each iter
}

#if 0
//Microsoft (R) C/C++ Optimizing Compiler Version 19.00.23506 for x86
#include <immintrin.h>
#include <intrin.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct V3f
{
	float	x;
	float	y;
	float	z;
};

// Define 128-bit wide 16-byte aligned hardware register type.
typedef __m128	Vector4;

// Define a bunch of macros for readability.
#define SPLAT_X( X )	_mm_shuffle_ps( X, X, _MM_SHUFFLE(0,0,0,0) )
#define SPLAT_Y( X )	_mm_shuffle_ps( X, X, _MM_SHUFFLE(1,1,1,1) )
#define SPLAT_Z( X )	_mm_shuffle_ps( X, X, _MM_SHUFFLE(2,2,2,2) )
#define SPLAT_W( X )	_mm_shuffle_ps( X, X, _MM_SHUFFLE(3,3,3,3) )

#define V4_ADD( A, B )	(_mm_add_ps( (A), (B) ))
#define V4_SUB( A, B )	(_mm_sub_ps( (A), (B) ))
#define V4_MUL( A, B )	(_mm_mul_ps( (A), (B) ))
#define V4_DIV( A, B )	(_mm_div_ps( (A), (B) ))

#define V4_MAD( A, B, C )	(_mm_add_ps( _mm_mul_ps((A),(B)), (C) ))

///----------------------------------------------------------------------
///	Packed matrix for affine 3D transformations:
///	the fourth row is (0,0,0,1),
///	the translation is computed from r[0].w, r[1].w, r[2].w.
///	The first three elements of each row/column form a rotation matrix
///	and the last element of every row is the translation over one axis.
///
///	Mat3x3	R;	// rotation component
///	Vec3	T;	// translation component
///
///	R[0][0], R[0][1], R[0][2], Tx[0]
///	R[1][0], R[1][1], R[1][2], Ty[1]
///	R[2][0], R[2][1], R[2][2], Tz[2]
///	((0, 0, 0, 1) - the forth row is not stored)
///----------------------------------------------------------------------
///
struct alignas(16) Matrix3x4f
{
	union
    {
		Vector4	r[3];	// laid out as 3 rows in memory
		float	m[3][4];
	};
	// (r[0].w, r[1].w, r[2].w) contains the translation.
	// (r[0].w, r[1].w, r[2].w, 1) are the forth column and
	// (0, 0, 0, 1) are the forth row of the full 4x4 matrix.
};

/*
=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    SCALAR VERSION
=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
*/
void transform_points_scalar(
    const Matrix3x4f& transform_,
    const V3f* __restrict input_points_,
    const int number_of_points,
    V3f *__restrict transformed_points_
    )
{
    for( int i = 0; i < number_of_points; i++ )
    {
        const V3f& point = input_points_[i];
        
        transformed_points_[i].x =
            transform_.m[0][0] * point.x +
            transform_.m[0][1] * point.y +
            transform_.m[0][2] * point.z +
            transform_.m[0][3];
        
        transformed_points_[i].y =
            transform_.m[1][0] * point.x +
            transform_.m[1][1] * point.y +
            transform_.m[1][2] * point.z +
            transform_.m[1][3];
        
        transformed_points_[i].z =
            transform_.m[2][0] * point.x +
            transform_.m[2][1] * point.y +
            transform_.m[2][2] * point.z +
            transform_.m[2][3];
    }
}

/*
=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    SIMD VERSION
=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
*/

struct Positions_SoA
{
    Vector4 *	xs;
    Vector4 *	ys;
    Vector4 *	zs;
};

void transform_points_SSE2(
    const Matrix3x4f& transform_,
    const Positions_SoA& inputs_,
    const int number_of_packets,
    Positions_SoA &outputs_
    )
{
    //FIXME: woudn't this cause too much register pressure and memory loads?
    // For AVX/AVX2 we'll have to splat 8/16 values.
    const Vector4 m00 = SPLAT_X( transform_.r[0] );
    const Vector4 m01 = SPLAT_Y( transform_.r[0] );
    const Vector4 m02 = SPLAT_Z( transform_.r[0] );
    const Vector4 m03 = SPLAT_W( transform_.r[0] );	// translation X

    const Vector4 m10 = SPLAT_X( transform_.r[1] );
    const Vector4 m11 = SPLAT_Y( transform_.r[1] );
    const Vector4 m12 = SPLAT_Z( transform_.r[1] );
    const Vector4 m13 = SPLAT_W( transform_.r[1] );	// translation Y

    const Vector4 m20 = SPLAT_X( transform_.r[2] );
    const Vector4 m21 = SPLAT_Y( transform_.r[2] );
    const Vector4 m22 = SPLAT_Z( transform_.r[2] );
    const Vector4 m23 = SPLAT_W( transform_.r[2] );	// translation Z

    Vector4 * __restrict   xs = inputs_.xs;
    Vector4 * __restrict   ys = inputs_.ys;
    Vector4 * __restrict   zs = inputs_.zs;
    Vector4 * __restrict   outxs = outputs_.xs;
    Vector4 * __restrict   outys = outputs_.ys;
    Vector4 * __restrict   outzs = outputs_.zs;

    for( int i = 0; i < number_of_packets; i++ )
    {
        outxs[i] = V4_ADD(
            V4_MAD(
				m00, xs[i],
                V4_MUL( m01, ys[i] )
			),
            V4_MAD( m02, zs[i], m03 )
        );

        outys[i] = V4_ADD(
            V4_MAD(
				m10, xs[i],
                V4_MUL( m11, ys[i] )
            ),
            V4_MAD( m12, zs[i], m13 )
        );

        outzs[i] = V4_ADD(
            V4_MAD(
				m20, xs[i],
                V4_MUL( m21, ys[i] )
            ),
            V4_MAD( m22, zs[i], m23 )
        );
    }
}

// utilities

static float get_random_float()
{
    return (rand() - RAND_MAX/2) / (float)RAND_MAX;
}

const V3f get_random_point()
{
    return { get_random_float(), get_random_float(), get_random_float() };
}

const Matrix3x4f get_random_matrix()
{
    Matrix3x4f result;
    for( int row = 0; row < 3; row++ )
        for( int col = 0; col < 4; col++ )
            result.m[row][col] = get_random_float();
    return result;
}

void print_matrix( const Matrix3x4f& transform_ )
{
    for( int row = 0; row < 3; row++ )
        for( int col = 0; col < 4; col++ )
            printf( "[%d][%d] = %f\n", row, col, transform_.m[row][col] );
}

int main(int /*argc*/, char ** /*argv*/)
{
    srand(6666); // for determinism

    //printf("Random matrix:\n");
    const Matrix3x4f transform = get_random_matrix();
    //print_matrix( transform );
    
    //printf("Random points:\n");
    enum { NUM_POINTS = 6100 };
    V3f positions[NUM_POINTS];
    for( int i = 0; i < NUM_POINTS; i++ )
    {
        positions[i] = get_random_point();
        //printf( "[%d] = (%f, %f, %f )\n", i, positions[i].x, positions[i].y, positions[i].z );
    }
    

    V3f	transformed_positions[NUM_POINTS];
    unsigned long long elapsed_time_scalar;
    {
        unsigned long long start_time = __rdtsc();

        transform_points_scalar(
            transform,
            positions,
            NUM_POINTS,
            transformed_positions
        );

        elapsed_time_scalar = __rdtsc() - start_time;
        printf( "transform_points_scalar(): %I64u ticks (%d points)\n", elapsed_time_scalar, NUM_POINTS );
    }
    

    enum {
        SIMD_WIDTH = 4,
        NUM_SIMD_PACKETS = (NUM_POINTS / SIMD_WIDTH) + !!(NUM_POINTS % SIMD_WIDTH)
    };

    // positions in SoA format:
    Vector4	positions_x[ NUM_SIMD_PACKETS ];
    Vector4	positions_y[ NUM_SIMD_PACKETS ];
    Vector4	positions_z[ NUM_SIMD_PACKETS ];
    for( int i = 0; i < NUM_POINTS; i++ )
    {
        ((float*)positions_x)[i] = positions[i].x;
        ((float*)positions_y)[i] = positions[i].y;
        ((float*)positions_z)[i] = positions[i].z;
    }

    Vector4	transformed_positions_x[ NUM_SIMD_PACKETS ];
    Vector4	transformed_positions_y[ NUM_SIMD_PACKETS ];
    Vector4	transformed_positions_z[ NUM_SIMD_PACKETS ];
    Positions_SoA transformed_positions_SoA = { transformed_positions_x, transformed_positions_y, transformed_positions_z };

    unsigned long long start_time = __rdtsc();

    transform_points_SSE2(
        transform,
        { positions_x, positions_y, positions_z },
        NUM_SIMD_PACKETS,
        transformed_positions_SoA
    );

    unsigned long long elapsed_time_simd = __rdtsc() - start_time;
    printf( "transform_points_SSE2(): %I64u ticks (%d packets)\n", elapsed_time_simd, NUM_SIMD_PACKETS );

    int num_failed_tests = 0;
    for( int i = 0; i < NUM_POINTS; i++ )
    {
        float x = ((float*)transformed_positions_x)[i];
        float y = ((float*)transformed_positions_y)[i];
        float z = ((float*)transformed_positions_z)[i];
        bool equal =
            x == transformed_positions[i].x &&
            y == transformed_positions[i].y &&
            z == transformed_positions[i].z;
        if( !equal ) {
            num_failed_tests++;
//            printf( "Test failed for point [%d]:\n", i );
            //printf( "\tsrc: (%f, %f, %f )\n", positions[i].x, positions[i].y, positions[i].z );
//            printf( "\tref: (%f, %f, %f )\n", transformed_positions[i].x, transformed_positions[i].y, transformed_positions[i].z );
//            printf( "\tgot: (%f, %f, %f )\n", x, y, z );
        }
    }
    
    printf( "\n====================================\n" );
    printf( "SSE version is %g times faster than the scalar one. %d out of %d failed.\n", (double)elapsed_time_scalar/elapsed_time_simd, num_failed_tests, NUM_POINTS );

    return 0;
}
#endif
#endif

}//namespace SDF
