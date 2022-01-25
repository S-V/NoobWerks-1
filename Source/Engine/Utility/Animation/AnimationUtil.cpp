#include <Base/Base.h>
#pragma hdrstop

#include <Base/Base.h>
#include <Base/Template/Containers/Blob.h>
#include <Base/Template/Containers/BitSet/BitArray.h>
#include <Base/Text/String.h>
#include <Base/Math/BoundingVolumes/ViewFrustum.h>

#include <bx/bx.h>	// uint8_t

//#include <libnoise/noise.h>
//#pragma comment( lib, "libnoise.lib" )

#include <Base/Template/Containers/Array/TInplaceArray.h>
#include <Core/Client.h>
#include <Core/ConsoleVariable.h>
#include <Core/Serialization/Serialization.h>
#include <Core/Tasking/TaskSchedulerInterface.h>
#include <Core/Tasking/SlowTasks.h>
#include <Core/Util/ScopedTimer.h>
#include <GPU/Private/Backend/d3d11/d3d_common.h>
#include <Engine/WindowsDriver.h>	// Window stuff
#include <Developer/TypeDatabase/TypeDatabase.h>

#include <Utility/MeshLib/TriMesh.h>
#include <Core/Serialization/Text/TxTSerializers.h>

#include <Engine/Engine.h>

#include <Rendering/Public/Core/RenderPipeline.h>
#include <Rendering/Public/Core/Material.h>
#include <Rendering/Private/RenderSystemData.h>
#include <Rendering/Private/Modules/Animation/OzzAnimationBridge.h>
#include <Rendering/Private/ShaderInterop.h>

#include <Rendering/Private/Modules/Animation/OzzAnimationBridge.h>

#include <Utility/Animation/AnimationUtil.h>


#if MX_DEVELOPER

namespace
{
	using namespace Rendering;

	inline
	const ozz::math::Quaternion toOzzQuat( const Q4f& q )
	{
		return ozz::math::Quaternion( q.x, q.y, q.z, q.w );
	}
	
	inline
	const ozz::math::Transform ozz_transform_from_md5_transform( const Doom3::md5Joint& md5_joint )
	{
		ozz::math::Transform	ozz_transform;
		ozz_transform.translation = toOzzVec(md5_joint.position);
		ozz_transform.rotation = toOzzQuat(md5_joint.orientation);
		ozz_transform.scale = toOzzVec(CV3f(1));
		mxASSERT(ozz::math::IsNormalized(ozz_transform.rotation));
		return ozz_transform;
	}
	
	inline
	const ozz::animation::offline::RawSkeleton::Joint ozz_raw_skeleton_joint_from_md5_joint( const Doom3::md5Joint& md5_joint )
	{
		ozz::animation::offline::RawSkeleton::Joint	ozz_raw_skeleton_joint;
		ozz_raw_skeleton_joint.name = md5_joint.name.c_str();
		ozz_raw_skeleton_joint.transform = ozz_transform_from_md5_transform( md5_joint );
		return ozz_raw_skeleton_joint;
	}
}



ERet convert_Doom3_md5_to_ozz_raw_skeleton(
	const Doom3::md5Model& md5_model
	, ozz::animation::offline::RawSkeleton &raw_skeleton_
	)
{
	/// temporary data structure for converting md5 skeleton to ozz skeleton
	struct md5_JointLink
	{
		int			joint_index;
		DynamicArray< int >	kids;
		int	embedded_storage[16];

	public:
		md5_JointLink()
			: joint_index( -1 )
			, kids( MemoryHeaps::temporary() )
		{
			kids.initializeWithExternalStorageAndCount( embedded_storage, mxCOUNT_OF(embedded_storage) );
		}
	};

	md5_JointLink	temp_storage[64];
	DynamicArray< md5_JointLink >	joint_links( MemoryHeaps::temporary()  );
	joint_links.initializeWithExternalStorageAndCount( temp_storage, mxCOUNT_OF(temp_storage) );

	const int num_joints = md5_model.joints.num();
	mxDO(joint_links.setNum( num_joints ));

	for( int i = 0; i < num_joints; i++ )
	{
		const Doom3::md5Joint& current_md5_joint = md5_model.joints[ i ];

		joint_links[ i ].joint_index = i;

		const int parent_joint_index = current_md5_joint.parentNum;
		if( parent_joint_index != Doom3::MD5_NO_PARENT_JOINT )
		{
			joint_links[ parent_joint_index ].kids.add( i );
		}
	}

	/// Create ozz joints.
	struct Util
	{
		static void create_ozz_joints( const Doom3::md5Model& md5_model, const md5_JointLink* joint_links
									, const int current_joint_index
									, ozz::animation::offline::RawSkeleton::Joint &dst_joint_ )
		{
			const Doom3::md5Joint& current_md5_joint = md5_model.joints[ current_joint_index ];

			dst_joint_ = ozz_raw_skeleton_joint_from_md5_joint( current_md5_joint );

			//
			const md5_JointLink& src_joint_link = joint_links[ current_joint_index ];
			dst_joint_.children.resize( src_joint_link.kids.num() );

			for( int child_index = 0; child_index < src_joint_link.kids.num(); child_index++ )
			{
				create_ozz_joints( md5_model, joint_links, src_joint_link.kids[child_index], dst_joint_.children[child_index] );
			}
		}
	};

	// The root joint of md5 is at index 0.
	raw_skeleton_.roots.resize(1);
	Util::create_ozz_joints( md5_model, joint_links.raw(), 0, raw_skeleton_.roots[0] );

	//...and so on with the whole skeleton hierarchy...

	// Test for skeleton validity.
	// The main invalidity reason is the number of joints, which must be lower
	// than ozz::animation::Skeleton::kMaxJoints.
	if (!raw_skeleton_.Validate()) {
		return ERR_INVALID_PARAMETER;
	}


	const int num_joints_in_md5 = md5_model.NumJoints();
	const int num_joints_in_ozz = raw_skeleton_.num_joints();
	mxASSERT(num_joints_in_md5 == num_joints_in_ozz);

	return ALL_OK;
}

ERet convert_md5_model_to_ozz_skeleton(
	const Doom3::md5Model& md5_model
	, ozz::animation::Skeleton &skeleton_
	)
{
	ozz::io::MemoryStream	memory_file;

	{
		ozz::animation::offline::RawSkeleton	raw_skeleton;
		mxDO(convert_Doom3_md5_to_ozz_raw_skeleton( md5_model, raw_skeleton ));

		// Convert the RawSkeleton to a runtime Skeleton.

		// Creates a SkeletonBuilder instance.
		ozz::animation::offline::SkeletonBuilder builder;

		// Executes the builder on the previously prepared RawSkeleton, which returns
		// a new runtime skeleton instance.
		// This operation will fail and return NULL if the RawSkeleton isn't valid.
		ozz::animation::Skeleton* skeleton = builder(raw_skeleton);

		// ...use the skeleton as you want...
		{
			ozz::io::OArchive		output_archive(&memory_file);
			skeleton->Save(output_archive);
		}

		// In the end the skeleton needs to be deleted.
		ozz::memory::default_allocator()->Delete(skeleton);
	}

	{
		memory_file.Seek(0, ozz::io::Stream::kSet);

		ozz::io::IArchive		input_archive(&memory_file);
		skeleton_.Load( input_archive, OZZ_SKELETON_VERSION );

		mxENSURE(skeleton_.num_joints() && skeleton_.num_soa_joints(),
			ERR_FAILED_TO_PARSE_DATA,
			"failed to create skeleton - did you pass correct version?"
			);
	}

	return ALL_OK;
}

static
int find_joint_index_in_ozz_skeleton( const char* joint_name
									 , const ozz::animation::Skeleton& skeleton )
{
	const ozz::Range<const char* const> joint_names = skeleton.joint_names();
	const int num_joints = skeleton.num_joints();

	for( int i = 0; i < num_joints; i++ ) {
		if( 0 == std::strcmp( joint_names[i], joint_name ) ) {
			return i;
		}
	}

	return -1;
}

static
void fill_ozz_joint_track( ozz::animation::offline::RawAnimation::JointTrack &ozz_joint_track
						  , const Doom3::md5Joint& md5_joint
						  , const int key_frame_index
						  , const float key_frame_time )
{
	ozz::animation::offline::RawAnimation::TranslationKey &ozz_joint_track_translation_key = ozz_joint_track.translations[ key_frame_index ];
	ozz_joint_track_translation_key.time = key_frame_time;
	ozz_joint_track_translation_key.value = toOzzVec(md5_joint.ozz_position);

	ozz::animation::offline::RawAnimation::RotationKey &ozz_joint_track_rotation_key = ozz_joint_track.rotations[ key_frame_index ];
	ozz_joint_track_rotation_key.time = key_frame_time;
	ozz_joint_track_rotation_key.value = toOzzQuat(md5_joint.ozz_orientation);
}

ERet convert_animation_from_md5_to_raw_ozz(
	const Doom3::md5Animation& md5_animation
	, ozz::animation::offline::RawAnimation &raw_animation_
	, const ozz::animation::Skeleton& ozz_skeleton
	)
{
	mxENSURE(md5_animation.numJoints == ozz_skeleton.num_joints(), ERR_INVALID_PARAMETER,
		"md5 anim has %d joints, while ozz skeleton has %d joints", md5_animation.numJoints, ozz_skeleton.num_joints());

	raw_animation_.name = md5_animation.name.c_str();

	// All the animation keyframes times must be within range [0, duration].
	raw_animation_.duration = md5_animation.totalDurationInSeconds();

	// There should be as much tracks as there are joints in the skeleton that
	// this animation targets.
	raw_animation_.tracks.resize( md5_animation.numJoints );

	// Fills each track with keyframes.
	// Tracks should be ordered in the same order as joints in the
	// ozz::animation::Skeleton. Joint's names can be used to find joint's
	// index in the skeleton.

	const float key_frame_duration = md5_animation.frameDurationInSeconds();

	for(int joint_index_in_md5 = 0;
		joint_index_in_md5 < md5_animation.numJoints;
		joint_index_in_md5++)
	{
		const Doom3::md5JointAnimInfo& md5_joint_info = md5_animation.jointInfo[ joint_index_in_md5 ];

		//
		const int joint_index_in_ozz = find_joint_index_in_ozz_skeleton( md5_joint_info.name.c_str(), ozz_skeleton );

		mxENSURE( joint_index_in_ozz != -1, ERR_OBJECT_NOT_FOUND,
			"couldn't find joint '%s' in ozz skeleton", md5_joint_info.name.c_str() );

		//
		ozz::animation::offline::RawAnimation::JointTrack &ozz_joint_track = raw_animation_.tracks[ joint_index_in_ozz ];

		//
		ozz_joint_track.translations.resize( md5_animation.numFrames );
		ozz_joint_track.rotations.resize( md5_animation.numFrames );

		for(int key_frame_index = 0;
			key_frame_index < md5_animation.numFrames;
			key_frame_index++)
		{
			const Doom3::md5Skeleton& md5_frame_skeleton = md5_animation.frameSkeletons[ key_frame_index ];
			const Doom3::md5Joint& md5_joint = md5_frame_skeleton[ joint_index_in_md5 ];

			const float key_frame_time = key_frame_index * key_frame_duration;

			fill_ozz_joint_track( ozz_joint_track, md5_joint, key_frame_index, key_frame_time );
		}//
	}//

	// It's not mandatory to have the same number of keyframes for translation, rotations and scales.

	// For this example, don't fill scale with any key. The default value will be
	// identity, which is ozz::math::Float3(1.f, 1.f, 1.f) for scale.

	//...and so on with all other tracks...

	// Test for animation validity. These are the errors that could invalidate
	// an animation:
	//  1. Animation duration is less than 0.
	//  2. Keyframes' are not sorted in a strict ascending order.
	//  3. Keyframes' are not within [0, duration] range.
	if (!raw_animation_.Validate()) {
		return ERR_INVALID_PARAMETER;
	}

	return ALL_OK;
}

ERet convert_animation_from_md5_to_ozz(
	ozz::animation::Animation &ozz_animation_
	, const Doom3::md5Animation& md5_animation_
	, const ozz::animation::Skeleton& ozz_skeleton
	)
{
	ozz::io::MemoryStream	memory_file;

	{
		ozz::animation::offline::RawAnimation	raw_animation;
		mxDO(convert_animation_from_md5_to_raw_ozz( md5_animation_, raw_animation, ozz_skeleton ));

		//
		ozz::animation::offline::RawAnimation	optimized_raw_animation;

		if(1)
		{
			const ozz::animation::offline::AnimationOptimizer	animation_optimizer;
			mxENSURE( animation_optimizer( raw_animation, ozz_skeleton, &optimized_raw_animation )
				, ERR_UNKNOWN_ERROR, "ozz: failed to optimize raw animation" );
		}
		else
		{
			optimized_raw_animation = raw_animation;
		}

		// Convert the RawAnimation to a runtime Animation.
		// 
		// Creates a AnimationBuilder instance.
		const ozz::animation::offline::AnimationBuilder builder;

		// Executes the builder on the previously prepared RawAnimation, which returns
		// a new runtime animation instance.
		// This operation will fail and return NULL if the RawAnimation isn't valid.
		ozz::animation::Animation* runtime_animation = builder( optimized_raw_animation );
		mxENSURE( runtime_animation, ERR_UNKNOWN_ERROR, "ozz: failed to create runtime animation" );

		// ...use the runtime_animation as you want...
		{
			ozz::io::OArchive		output_archive( &memory_file );
			runtime_animation->Save( output_archive );
		}

		// In the end the runtime_animation needs to be deleted.
		ozz::memory::default_allocator()->Delete( runtime_animation );
	}

	{
		memory_file.Seek(0, ozz::io::Stream::kSet);

		ozz::io::IArchive		input_archive( &memory_file );
		ozz_animation_.Load( input_archive, OZZ_ANIMATION_VERSION );

		mxENSURE(ozz_animation_.num_tracks() && ozz_animation_.num_soa_tracks(), ERR_FAILED_TO_PARSE_DATA, "failed to create animation - did you pass correct version?");
	}

	return ALL_OK;
}

static
ERet buildSkinnedMeshBuffer( const Doom3::md5Mesh& md5_mesh
							, TbSkinnedMeshData::Submesh &dst_mesh
							, const Doom3::md5Model& md5_model
							, const ozz::animation::Skeleton& ozz_skeleton	// for determining joint indices
							)
{
	const int num_vertices = md5_mesh.vertices.num();

	const int num_indices = md5_mesh.triangles.num() * 3;
	mxENSURE(num_indices < MAX_UINT16, ERR_BUFFER_TOO_SMALL, "only 16-bit indices supported (got %d)", num_indices);

	{
		const char* md5_shader_name = md5_mesh.shader.c_str();

		dst_mesh.material = make_AssetID_from_raw_string( md5_shader_name );

		dst_mesh.can_skip_rendering = ( strstr(md5_shader_name, "damage") || strstr(md5_shader_name, "skeleton") );
	}

	mxDO(dst_mesh.vertices.setNum(num_vertices));
	mxDO(dst_mesh.indices.setNum(num_indices));

	for( int iVertex = 0; iVertex < num_vertices; iVertex++ )
	{
		const Doom3::md5Vertex& md5_vertex = md5_mesh.vertices[iVertex];

		if( md5_vertex.numWeights > Vertex_Skinned::MAX_WEIGHTS )
		{
//			ptWARN("vertex %d has %d (>4) weights", iVertex, md5_vertex.numWeights);
		}

		// some models do have >4 joint weights, so it is necessary to sort and renormalize

		struct SortedWeight
		{
			float	joint_influence;
			int		weight_index_in_mesh;
		};

		SortedWeight	sorted_weights[Vertex_Skinned::MAX_BONES];

		float total_weight = 0;

		for( int iWeight = 0; iWeight < md5_vertex.numWeights; iWeight++ )
		{
			const int weight_index_in_mesh = md5_vertex.startWeight + iWeight;
			const Doom3::md5Weight& md5_weight = md5_mesh.weights[ weight_index_in_mesh ];

			SortedWeight &sorted_weight = sorted_weights[ iWeight ];
			sorted_weight.joint_influence = md5_weight.bias;
			sorted_weight.weight_index_in_mesh = weight_index_in_mesh;

			total_weight += md5_weight.bias;
		}

		mxENSURE(
			total_weight > 0.999f && total_weight < 1.001f
			, ERR_INVALID_PARAMETER
			, "sum of weights must be 1 (got %.3f)", total_weight
			);

		// sort the weights in decreasing order (using bubble sort) and take the four largest
		for( int i = 0; i < md5_vertex.numWeights-1; i++ )
		{
			for( int j = i+1; j < md5_vertex.numWeights; j++ )
			{
				if( sorted_weights[i].joint_influence < sorted_weights[j].joint_influence )
				{
					TSwap( sorted_weights[i], sorted_weights[j] );
				}
			}
		}

		const int num_used_weights = smallest( md5_vertex.numWeights, Vertex_Skinned::MAX_WEIGHTS );

		float total_used_weight = 0;
		for( int i = 0; i < num_used_weights; i++ )
		{
			total_used_weight += sorted_weights[i].joint_influence;
		}

		//
		const float residualWeight = total_weight - total_used_weight;

		//
		float	used_blend_weights[Vertex_Skinned::MAX_WEIGHTS] = { 0 };
		int		used_joint_indices[Vertex_Skinned::MAX_WEIGHTS] = { 0 };

		for( int i = 0; i < num_used_weights; i++ )
		{
			const float renormalized_weight = sorted_weights[i].joint_influence / total_used_weight;
			used_blend_weights[i] = renormalized_weight;

			const Doom3::md5Weight& md5_weight = md5_mesh.weights[ sorted_weights[i].weight_index_in_mesh ];
			used_joint_indices[i] = md5_weight.jointIndex;
		}

/*
		// Sort the weights and indices for hardware skinning
		for ( int i = 0; i < num_used_weights-1; ++i ) {
			for ( int j = i + 1; j < num_used_weights; ++j ) {
				if ( used_blend_weights[i] < used_blend_weights[j] ) {
					TSwap( used_blend_weights[i], used_blend_weights[j] );
					TSwap( used_joint_indices[i], used_joint_indices[j] );
				}
			}
		}

		// Give any left over to the biggest weight
		const float sum_of_used_weights = used_blend_weights[0] + used_blend_weights[1] + used_blend_weights[2] + used_blend_weights[3];
		used_blend_weights[0] += largest( 1.0f - sum_of_used_weights, 0 );
*/

		//
		Vertex_Skinned &dst_vertex = dst_mesh.vertices[iVertex];

		dst_vertex.position = CV3f(0);
		dst_vertex.texCoord = V2_To_Half2(md5_vertex.uv);

		const V3f& md5_vertex_normal_in_bind_pose = md5_mesh.bindPoseNormals[iVertex];
		const V3f& md5_vertex_tangent_in_bind_pose = md5_mesh.bindPoseTangents[iVertex];

		// NOTE: negate the normal, because Doom3's triangle winding is different
		dst_vertex.normal = PackNormal( -md5_vertex_normal_in_bind_pose );
		dst_vertex.tangent = PackNormal( -md5_vertex_tangent_in_bind_pose );

		dst_vertex.indices.v = 0;
		dst_vertex.weights.v = 0;

		// Compute the mesh's vertex positions in object-local space
		// based on the model's joints and the mesh's weights array.

		for( int i = 0; i < num_used_weights; i++ )
		{
			const Doom3::md5Weight& md5_weight = md5_mesh.weights[ sorted_weights[i].weight_index_in_mesh ];
			const Doom3::md5Joint& md5_joint = md5_model.joints[ md5_weight.jointIndex ];

			// Convert the weight position from Joint local space to object space
 			const V3f weight_pos = md5_joint.position + md5_joint.orientation.rotateVector( md5_weight.position );

			dst_vertex.position += weight_pos * used_blend_weights[i];

			const int joint_index_in_ozz = find_joint_index_in_ozz_skeleton( md5_joint.name.c_str(), ozz_skeleton );
			mxENSURE( joint_index_in_ozz != -1, ERR_OBJECT_NOT_FOUND,
				"couldn't find joint '%s' in ozz skeleton", md5_joint.name.c_str() );

			dst_vertex.indices.c[i] = joint_index_in_ozz;
			dst_vertex.weights.c[i] = _NormalizedFloatToUInt8( used_blend_weights[i] );
		}

		////
		//if( md5_vertex.numWeights > Vertex_Skinned::MAX_WEIGHTS )
		//{
		//	String256	temp;

		//	for( int i = 0; i < md5_vertex.numWeights; i++ ) {
		//		const int weight_index_in_mesh = md5_vertex.startWeight + i;
		//		const Doom3::md5Weight& md5_weight = md5_mesh.weights[ weight_index_in_mesh ];
		//		Str::SAppendF(temp, "[%d]=%.3f, ", md5_weight.jointIndex, md5_weight.bias);
		//	}

		//	ptPRINT("--- [%d] Before: %s", iVertex, temp.c_str());

		//	//
		//	temp.empty();

		//	for( int i = 0; i < num_used_weights; i++ ) {
		//		Str::SAppendF(temp, "[%d]=%.3f, ", used_joint_indices[i], used_blend_weights[i]);
		//	}

		//	ptPRINT("--- [%d] After: %s", iVertex, temp.c_str());
		//}

	}//for each vertex

	//
	UInt3* src_tris = (UInt3*) md5_mesh.triangles.raw();

	Meshok::CopyIndices(
		dst_mesh.indices.raw()
		, false	// dstIndices32bit?
		, TSpan<UInt3>(src_tris, md5_mesh.triangles.num())
		);

	return ALL_OK;
}

ERet buildSkinnedMeshData( const Doom3::md5Model& md5_model
						  , TbSkinnedMeshData &skinned_mesh_data_
						  , const ozz::animation::Skeleton& ozz_skeleton	// for determining joint indices
						  )
{
	const int numJoints = md5_model.NumJoints();
	mxENSURE(numJoints < 255, ERR_BUFFER_TOO_SMALL, "num joints (%d) is greater than 255", numJoints);

	const int numSubmeshes = md5_model.meshes.num();
	mxDO(skinned_mesh_data_.submeshes.setNum(numSubmeshes));

	for( int iSubmesh = 0; iSubmesh < numSubmeshes; iSubmesh++ )
	{
		const Doom3::md5Mesh& md5_mesh = md5_model.meshes[iSubmesh];

		TbSkinnedMeshData::Submesh &dst_mesh = skinned_mesh_data_.submeshes[iSubmesh];

		mxDO(buildSkinnedMeshBuffer( md5_mesh, dst_mesh, md5_model, ozz_skeleton ));
	}

	//
	mxDO(skinned_mesh_data_.inverseBindPoseMatrices.setNum( md5_model.NumJoints() ));

	for( int iJoint = 0; iJoint < md5_model.NumJoints(); iJoint++ )
	{
		const Doom3::md5Joint& md5_joint = md5_model.joints[ iJoint ];

		const int joint_index_in_ozz = find_joint_index_in_ozz_skeleton( md5_joint.name.c_str(), ozz_skeleton );
		mxENSURE( joint_index_in_ozz != -1, ERR_OBJECT_NOT_FOUND,
			"couldn't find joint '%s' in ozz skeleton", md5_joint.name.c_str() );

		skinned_mesh_data_.inverseBindPoseMatrices[joint_index_in_ozz] = md5_model.inverseBindPoseMatrices[iJoint];
	}

	ptPRINT("Built skinned mesh: %d submeshes", numSubmeshes);

	return ALL_OK;
}

void dbg_drawSkinnedMeshData_inBindPose( const TbSkinnedMeshData& skinned_mesh_data
							 , TbPrimitiveBatcher & debugRenderer
							 , const RGBAf& color )
{
	for( UINT iSubMesh = 0; iSubMesh < skinned_mesh_data.submeshes.num(); ++iSubMesh )
	{
		const TbSkinnedMeshData::Submesh& submesh = skinned_mesh_data.submeshes[iSubMesh];

		const Vertex_Skinned* verts = submesh.vertices.raw();
		const U16* tris = submesh.indices.raw();

		const UINT numTris = submesh.indices.num() / 3;

		for( UINT iTri = 0; iTri < numTris; iTri++ )
		{
			const Vertex_Skinned& vert0 = verts[ tris[ iTri*3 + 0 ] ];
			const Vertex_Skinned& vert1 = verts[ tris[ iTri*3 + 1 ] ];
			const Vertex_Skinned& vert2 = verts[ tris[ iTri*3 + 2 ] ];

			debugRenderer.DrawLine(vert0.position, vert1.position, color, color);
			debugRenderer.DrawLine(vert1.position, vert2.position, color, color);
			debugRenderer.DrawLine(vert0.position, vert2.position, color, color);
		}
	}
}

ozz::math::Float4x4 ozz_makeZeroMatrix()
{
	const ozz::math::Float4x4	result = {
		Vector4_Zero(),
		Vector4_Zero(),
		Vector4_Zero(),
		Vector4_Zero(),
	};
	return result;
}

ozz::math::Float4x4 ozz_multiplyMatrixByScalar( const ozz::math::Float4x4& m, const float s )
{
	const ozz::math::Float4x4	result = {
		Vector4_Scale( m.cols[0], s ),
		Vector4_Scale( m.cols[1], s ),
		Vector4_Scale( m.cols[2], s ),
		Vector4_Scale( m.cols[3], s ),
	};
	return result;
}

void dbg_drawSkinnedMeshData_Animated( const TbSkinnedMeshData& skinned_mesh_data
									  , const ozz::animation::Skeleton& skeleton
									  , ozz::Range<const ozz::math::Float4x4> bone_matrices	// object-space joint transforms
									  , TbPrimitiveBatcher & debugRenderer
									  , const M44f& world_transform
									  , const RGBAf& color )
{
	const int num_joints = skeleton.num_joints();

	ozz::math::Float4x4	matrix_palette[Vertex_Skinned::MAX_BONES];
	MemZero(matrix_palette, sizeof(matrix_palette));

	for( int i = 0; i < num_joints; i++ )
	{
		const M44f& inverse_bind_pose_matrix = skinned_mesh_data.inverseBindPoseMatrices[i];

		const ozz::math::Float4x4	inverse_bind_pose_matrix_ozz = {
			inverse_bind_pose_matrix.r0,
			inverse_bind_pose_matrix.r1,
			inverse_bind_pose_matrix.r2,
			inverse_bind_pose_matrix.r3,
		};

		matrix_palette[i] = bone_matrices[i] * inverse_bind_pose_matrix_ozz;
	}

	//
	AllocatorI &	scratchpad = MemoryHeaps::temporary();

	DynamicArray< V3f >	transformed_vertices(scratchpad);

	//
	for( UINT iSubMesh = 0; iSubMesh < skinned_mesh_data.submeshes.num(); ++iSubMesh )
	{
		const TbSkinnedMeshData::Submesh& submesh = skinned_mesh_data.submeshes[iSubMesh];

		const Vertex_Skinned* verts = submesh.vertices.raw();

		{
			const int numVerts = submesh.vertices.num();
			transformed_vertices.setNum(numVerts);

			for( int iVert = 0; iVert < numVerts; iVert++ )
			{
				const Vertex_Skinned& src_vert = verts[iVert];

				const ozz::math::SimdFloat4 simd_xyz1 =
					ozz::math::simd_float4::Load( src_vert.position.x, src_vert.position.y, src_vert.position.z, 1 );

				ozz::math::Float4x4 final_transform = ozz_makeZeroMatrix();

				float sum = 0;

				for( int k = 0; k < mxCOUNT_OF(src_vert.weights.c); k++ )
				{
					const U8 joint_index = src_vert.indices.c[k];
					const U8 blend_weight8 = src_vert.weights.c[k];

					const float blend_weight = _UInt8ToNormalizedFloat( blend_weight8 );

					const ozz::math::Float4x4 scaled_bone_matrix = ozz_multiplyMatrixByScalar( matrix_palette[joint_index], blend_weight );

					final_transform = final_transform + scaled_bone_matrix;

					sum += blend_weight;
				}

				//mxASSERT( sum > 0.999f && sum < 1.001f );

				const ozz::math::SimdFloat4 simd_xyz1_transformed = ozz::math::TransformPoint( final_transform, simd_xyz1 );

				ozz::math::Store3PtrU(simd_xyz1_transformed, transformed_vertices[iVert].a);

				transformed_vertices[iVert] = M44_TransformPoint( world_transform, transformed_vertices[iVert] );
			}
		}

		//
		const U16* tris = submesh.indices.raw();
		const UINT numTris = submesh.indices.num() / 3;

		for( UINT iTri = 0; iTri < numTris; iTri++ )
		{
			const V3f& vert0 = transformed_vertices[ tris[ iTri*3 + 0 ] ];
			const V3f& vert1 = transformed_vertices[ tris[ iTri*3 + 1 ] ];
			const V3f& vert2 = transformed_vertices[ tris[ iTri*3 + 2 ] ];

			debugRenderer.DrawLine(vert0, vert1, color, color);
			debugRenderer.DrawLine(vert1, vert2, color, color);
			debugRenderer.DrawLine(vert0, vert2, color, color);
		}
	}
}

#endif // MX_DEVELOPER
