#include <Base/Base.h>
#include <Base/Math/Matrix/TMatrix3x3.h>
#include <Base/Math/Matrix/TMatrix4x4.h>
#include <Base/Template/Containers/Array/TInplaceArray.h>

#include <Developer/ThirdPartyGames/RedFaction/RedFaction_Common.h>
#include <Developer/ThirdPartyGames/RedFaction/RedFaction_AnimationLoader.h>
#include <Developer/ThirdPartyGames/RedFaction/RedFaction_Experimental.h>



struct TmpJoint
{
	Q4f		relative_orientation;	// joint's orientation_relative_to_parent quaternion relative to the parent joint
	V3f		relative_position;		// joint's position relative to the parent joint
	int		parent;			// index of the parent joint; this is the root joint if (-1)
int		bone_index;

M44f		local_mat_rel_to_parent;
M44f		obj_space_joint_mat;
};


TestSkinnedModel::TestSkinnedModel( AllocatorI & allocator )
	: _vertices( allocator )
	, _triangles( allocator )

	, _transformed_vertices( allocator )

	, _submeshes( allocator )

	, _bones( allocator )
	, _animated_joints( allocator )
	, _bone_to_joint_index( allocator )
{
}

ERet TestSkinnedModel::computeBindPoseJoints()
{
	//
	AllocatorI &	scratchpad = MemoryHeaps::temporary();

	//
	const UINT	num_bones = _bones.num();

	//
	struct JointNode
	{
		int						bone_index;
		TInplaceArray< U8, 8 >	kids;

	public:
		JointNode() : bone_index( -1 ) {}
	};

	//
	DynamicArray< JointNode >	bone_hierarchy( scratchpad );
	mxDO(bone_hierarchy.setNum( num_bones ));

	//
	UINT	root_bone_index = ~0;
	for( UINT iBone = 0; iBone < num_bones; iBone++ )
	{
		const Bone&	bone = _bones._data[ iBone ];
		//
		JointNode &dst_node = bone_hierarchy._data[ iBone ];
		//
		dst_node.bone_index = iBone;
		//
		if( bone.parent != PARENT_NONE )
		{
			// This is a child joint.
			JointNode &parent_node = bone_hierarchy._data[ bone.parent ];
			parent_node.kids.add( iBone );
		}
		else
		{
			// This is the root joint.
			mxASSERT(root_bone_index == ~0);
			root_bone_index = iBone;
		}
	}//For each bone.

	//
	struct FillJointsArrayUtil
	{
		static void fillJoint(
			AnimatedJoint *new_joint_
			, const int parent_joint_index
			, const JointNode& bone_node
			, const TSpan< const Bone >& bones_array
			)
		{
			const Bone& bone = bones_array[ bone_node.bone_index ];
			//
			new_joint_->orientation_relative_to_parent = bone.orientation;
			new_joint_->position_relative_to_parent = bone.position;
			new_joint_->parent = parent_joint_index;
			new_joint_->bone_index = bone_node.bone_index;
			new_joint_->name = bone.name;
			new_joint_->local_mat_rel_to_parent = bone.obj_space_joint_mat;
		}

		static void writeJoints_BreadthFirst_Recursive(
			DynamicArray< AnimatedJoint > &dst_joints_array_
			, const int current_node_index
			, const int parent_joint_index
			, const TSpan< const JointNode >& bone_hierarchy
			, const TSpan< const Bone >& bones_array
			)
		{
			const JointNode& bone_node = bone_hierarchy[ current_node_index ];

			const UINT	first_child_joint_index = dst_joints_array_.num();

			//
			for( UINT iKid = 0; iKid < bone_node.kids.num(); iKid++ )
			{
				const UINT child_node_index = bone_node.kids[ iKid ];
				const JointNode& child_bone_node = bone_hierarchy[ child_node_index ];

				const int	new_child_joint_index = dst_joints_array_.num();

				AnimatedJoint	new_child_joint;
				fillJoint(
					&new_child_joint
					, parent_joint_index
					, child_bone_node
					, bones_array
					);

				dst_joints_array_.add( new_child_joint );
			}

			//
			for( UINT iKid = 0; iKid < bone_node.kids.num(); iKid++ )
			{
				const UINT child_node_index = bone_node.kids[ iKid ];

				writeJoints_BreadthFirst_Recursive(
					dst_joints_array_
					, child_node_index
					, first_child_joint_index + iKid
					, bone_hierarchy
					, bones_array
					);
			}
		}

		static void writeJoints_DepthFirst_Recursive(
			DynamicArray< AnimatedJoint > &dst_joints_array_
			, const int current_node_index
			, const int parent_joint_index
			, const TSpan< const JointNode >& bone_hierarchy
			, const TSpan< const Bone >& bones_array
			)
		{
			const JointNode& bone_node = bone_hierarchy[ current_node_index ];

			//
			const UINT	new_joint_index = dst_joints_array_.num();

			AnimatedJoint	new_joint;
			fillJoint(
				&new_joint
				, parent_joint_index
				, bone_node
				, Arrays::GetSpan(bones_array)
				);
			(dst_joints_array_.add( new_joint ));

			//
			for( UINT iKid = 0; iKid < bone_node.kids.num(); iKid++ )
			{
				const UINT child_node_index = bone_node.kids[ iKid ];

				writeJoints_DepthFirst_Recursive(
					dst_joints_array_
					, child_node_index
					, new_joint_index
					, bone_hierarchy
					, bones_array
					);
			}
		}
	};

	// Build the skeleton joints' hierarchy.

	mxDO(_animated_joints.ReserveExactly( num_bones ));

	//
	FillJointsArrayUtil::writeJoints_DepthFirst_Recursive(
		_animated_joints
		, root_bone_index
		, PARENT_NONE
		, Arrays::GetSpan(bone_hierarchy)
		, Arrays::GetSpan(_bones)
		);

	//
	DBGOUT("\n");

	mxDO(_bone_to_joint_index.setNum( num_bones ));

	//
	for( UINT joint_index = 0; joint_index < num_bones; joint_index++ )
	{
		const AnimatedJoint& joint = _animated_joints._data[ joint_index ];

		LogStream(LL_Trace)
			,"Joint[",joint_index,"]: ",joint.name
			,", orientation = ",joint.orientation_relative_to_parent
			,", position = ",joint.position_relative_to_parent
			,", parent = ",joint.parent
			,", bone_index = ",joint.bone_index
			,", Q: ",joint.orientation_relative_to_parent.humanReadableString()
			;

		_bone_to_joint_index[ joint.bone_index ] = joint_index;

	}//For each joint.

	DBGOUT("\n");



	DynamicArray< TmpJoint >	tmp_joints( scratchpad );
	mxDO(tmp_joints.setNum( num_bones ));

	//
	for( UINT joint_index = 0; joint_index < num_bones; joint_index++ )
	{
		AnimatedJoint& joint = _animated_joints._data[ joint_index ];

		const Bone& bone = _bones[ joint.bone_index ];

		//
		if( joint.parent >= 0 )
		{
			const AnimatedJoint& parent_joint = _animated_joints[ joint.parent ];
			const Bone& parent_bone = _bones[ parent_joint.bone_index ];

			//
			const Q4f	inverse_orientation_of_parent_joint = parent_bone.orientation.inverse();

			//?
			const Q4f	joint_orientation_relative_to_parent = bone.orientation
				.MulWith( inverse_orientation_of_parent_joint )
				;

			const V3f	joint_position_relative_to_parent = inverse_orientation_of_parent_joint
				.rotateVector( bone.position - parent_bone.position )
				;


			// Concatenate transforms.
			V3f	rotated_position = parent_bone.orientation.rotateVector( joint_position_relative_to_parent );
			mxASSERT( V3_Equal( bone.position, parent_bone.position + rotated_position, 1e-5f ) );

			Q4f	joint_orientation = joint_orientation_relative_to_parent.MulWith( parent_bone.orientation );
			joint_orientation.NormalizeSelf();

			mxASSERT( bone.orientation.equals( joint_orientation, 1e-5f ) );

			//
			joint.orientation_relative_to_parent = joint_orientation_relative_to_parent;
			joint.position_relative_to_parent = joint_position_relative_to_parent;

			//
			joint.local_mat_rel_to_parent = M44_Inverse( parent_bone.obj_space_joint_mat ) * bone.obj_space_joint_mat;

			//
			const M44f	local_mat_from_quat = RF1::buildMat4(
				joint_orientation_relative_to_parent,
				joint_position_relative_to_parent
				);

			//mxASSERT(M44_Equal(joint.local_mat_rel_to_parent, local_mat_from_quat));
		}
		else
		{
			//
			joint.orientation_relative_to_parent = bone.orientation;
			joint.position_relative_to_parent = bone.position;
		}

		//
		TmpJoint &dst_joint = tmp_joints[ joint_index ];
		dst_joint.relative_orientation = joint.orientation_relative_to_parent;
		dst_joint.relative_position = joint.position_relative_to_parent;
		dst_joint.parent = joint.parent;
		dst_joint.bone_index = joint.bone_index;

	}//For each joint.


	// The root joint has index 0 so start with index 1.
	for( UINT joint_index = 1; joint_index < num_bones; joint_index++ )
	{
		TmpJoint& joint = tmp_joints._data[ joint_index ];
		//
		const TmpJoint& parent_joint = tmp_joints[ joint.parent ];

		// Concatenate transforms.

		V3f	joint_position = parent_joint.relative_orientation.rotateVector( joint.relative_position ) + parent_joint.relative_position;
		Q4f	joint_orientation = joint.relative_orientation.MulWith( parent_joint.relative_orientation );
		joint_orientation.NormalizeSelf();

		//
		joint.relative_orientation = joint_orientation;
		joint.relative_position = joint_position;

		//
		const Bone& bone = _bones[ joint.bone_index ];
		mxASSERT( V3_Equal( bone.position, joint_position, 1e-5f ) );
		mxASSERT( bone.orientation.equals( joint_orientation, 1e-5f ) );

	}//For each joint.

#if 0
	//
	// Build the bind-pose skeleton joints in object-local space.

	// The root joint has index 0 so start with index 1.
	for( UINT iJoint = 1; iJoint < num_bones; iJoint++ )
	{
		AnimatedJoint& joint = _animated_joints._data[ iJoint ];

		// To build the final skeleton joint in object-local space,
		// you have to add the position and orientation of the joint’s parent.

		const AnimatedJoint& parent_joint = _animated_joints[ joint.parent ];

		// Concatenate transforms.
		V3f	rotated_position = parent_joint.orientation.rotateVector( joint.position );
		joint.position = parent_joint.position + rotated_position;

		joint.orientation = joint.orientation.MulWith( parent_joint.orientation );
		joint.orientation.NormalizeSelf();

		//

	}//For each joint.
#endif

	return ALL_OK;
}

void TestSkinnedModel::draw(
	ALineRenderer & renderer
	, const M44f& model_transform
	, const RGBAf& color
	, int flags
	) const
{
	UNDONE;

	//for( int iSubMesh = 0; iSubMesh < _submeshes.num(); iSubMesh++ )
	//{
	//	const Submesh& submesh = _submeshes[ iSubMesh ];

	//	const V3f* positions = submesh.transformedPositions.raw();

	//	for( int iTri = 0; iTri < submesh.triangles.num(); iTri++ )
	//	{
	//		const UInt3& tri = submesh.triangles[ iTri ];

	//		V3f pA = M44_TransformPoint( model_transform, positions[ tri.a[ 2 ] ] );
	//		V3f pB = M44_TransformPoint( model_transform, positions[ tri.a[ 1 ] ] );
	//		V3f pC = M44_TransformPoint( model_transform, positions[ tri.a[ 0 ] ] );

	//		renderer.DrawWireTriangle( pA, pB, pC, color );
	//	}
	//}
}

void TestSkinnedModel::drawBindPoseMesh(
	ALineRenderer & renderer
	, const M44f& model_transform
	, const RGBAf& color
	) const
{
	// Draw each triangle of the model. Slow!

	for( UINT iTriangle = 0; iTriangle < _triangles.num(); iTriangle++ )
	{
		const UInt3& tri = _triangles._data[ iTriangle ];

		const Vertex& v0 = _vertices[ tri.a[ 0 ] ];
		const Vertex& v1 = _vertices[ tri.a[ 1 ] ];
		const Vertex& v2 = _vertices[ tri.a[ 2 ] ];

		const V3f p0 = M44_TransformPoint( model_transform, v0.pos );
		const V3f p1 = M44_TransformPoint( model_transform, v1.pos );
		const V3f p2 = M44_TransformPoint( model_transform, v2.pos );

		renderer.DrawWireTriangle( p0, p1, p2, color );
	}
}

void TestSkinnedModel::drawOriginalBones_Quat(
	ALineRenderer & renderer
	, ADebugTextRenderer& text_renderer
	, const NwView3D& camera_view
	, const M44f& model_transform
	, const RGBAf& child_joint_color
	, const RGBAf& parent_joint_color
	) const
{
	const bool	align_skeleton_with_mesh = 1;

	// Rotate to align the skeleton with the mesh.
	const NwQuaternion rotate_around_X_by_180 = NwQuaternion::FromAxisAngle(
		CV3f(1,0,0),
		mxPI
		);

	const NwQuaternion rotate_to_align_skeleton_with_mesh =
		align_skeleton_with_mesh
		? rotate_around_X_by_180
		: Q4f::identity()
		;

	//
	const M44f	transform_and_align_skeleton_with_mesh
		= model_transform
		* RF1::buildMat4( rotate_to_align_skeleton_with_mesh, CV3f(0) )
		;

	//
	static const RGBAf s_bone_colors[] = {
		RGBAf::RED,
		RGBAf::GREEN,
		RGBAf::BLUE,
		RGBAf::WHITE,
	};

	// root joint has index 0 so start with index 1
	for( int iBone = 0; iBone < _bones.num(); iBone++ )
	{
		const Bone&	bone = _bones._data[ iBone ];
		//
		const V3f	rotated_joint_position = bone.orientation.rotateVector( bone.position );

		const V3f	transformed_joint_pos = M44_TransformPoint( transform_and_align_skeleton_with_mesh, rotated_joint_position );

		//
		const bool can_use_debug_color = ( iBone < mxCOUNT_OF(s_bone_colors) );
		const RGBAf	debug_color = can_use_debug_color ? s_bone_colors[ iBone ] : RGBAf::WHITE;

		//
		renderer.DrawCircle(
			transformed_joint_pos
			, camera_view.right_dir, camera_view.getUpDir()
			, can_use_debug_color ? debug_color : child_joint_color
			, 0.1f
			, 4
			);

		//
		if( bone.parent >= 0 )
		{
			const Bone&	parent_bone = _bones._data[ bone.parent ];
			//
			const V3f	rotated_parent_position = parent_bone.orientation.rotateVector( parent_bone.position );

			const V3f	transformed_parent_joint_pos = M44_TransformPoint( transform_and_align_skeleton_with_mesh, rotated_parent_position );

			//
			renderer.DrawLine(
				transformed_joint_pos, transformed_parent_joint_pos
				, can_use_debug_color ? debug_color : child_joint_color
				, can_use_debug_color ? debug_color : parent_joint_color
				);
		}
		else
		{
			// This is the root joint.
			renderer.DrawSternchen(
				transformed_joint_pos
				, camera_view.right_dir, camera_view.getUpDir()
				, can_use_debug_color ? debug_color : parent_joint_color
				, 0.5f
				);
		}

		//
		text_renderer.drawText(
			transformed_joint_pos
			, RGBAf::WHITE
			, nil
			, 1.5f
			, "%s", bone.name.c_str()
			);

	}//For each joint.
}


void TestSkinnedModel::drawOriginalBones_Mat(
	ALineRenderer & renderer
	, ADebugTextRenderer& text_renderer
	, const NwView3D& camera_view
	, const M44f& model_transform
	, const RGBAf& child_joint_color
	, const RGBAf& parent_joint_color
	) const
{
	const M44f	rotate_to_align_skeleton_with_mesh
		= 0
		? M44_FromRotationMatrix(M33_RotationX(mxPI))
		: M44_Identity()
		;

	static const RGBAf s_bone_colors[] = {
		RGBAf::RED,
		RGBAf::GREEN,
		RGBAf::BLUE,
		RGBAf::WHITE,
	};

	// root joint has index 0 so start with index 1
	for( int iBone = 0; iBone < _bones.num(); iBone++ )
	{
		const Bone&	bone = _bones._data[ iBone ];
		//
		const V3f	transformed_joint_pos = M44_TransformPoint(
			model_transform * rotate_to_align_skeleton_with_mesh
			, RF1::getRotatedPointFromBoneMat( bone.obj_space_joint_mat )
			);

		//
		const bool can_use_debug_color = ( iBone < mxCOUNT_OF(s_bone_colors) );
		const RGBAf	debug_color = can_use_debug_color ? s_bone_colors[ iBone ] : RGBAf::WHITE;

		//
		renderer.DrawCircle(
			transformed_joint_pos
			, camera_view.right_dir, camera_view.getUpDir()
			, can_use_debug_color ? debug_color : child_joint_color
			, 0.1f
			, 4
			);

		//
		if( bone.parent >= 0 )
		{
			const Bone&	parent_bone = _bones._data[ bone.parent ];
			//
			const V3f	transformed_parent_joint_pos = M44_TransformPoint(
				model_transform * rotate_to_align_skeleton_with_mesh
				, RF1::getRotatedPointFromBoneMat( parent_bone.obj_space_joint_mat )
				);

			//
			renderer.DrawLine(
				transformed_joint_pos, transformed_parent_joint_pos
				, can_use_debug_color ? debug_color : child_joint_color
				, can_use_debug_color ? debug_color : parent_joint_color
				);
		}
		else
		{
			// This is the root joint.
			renderer.DrawSternchen(
				transformed_joint_pos
				, camera_view.right_dir, camera_view.getUpDir()
				, can_use_debug_color ? debug_color : parent_joint_color
				, 0.5f
				);
		}

		//
		text_renderer.drawText(
			transformed_joint_pos
			, RGBAf::WHITE
			, nil
			, 1.5f
			, "%s", bone.name.c_str()
			);

	}//For each joint.
}

void TestSkinnedModel::drawBindPoseSkeleton(
	ALineRenderer & renderer
	, const NwView3D& camera_view
	, const M44f& model_transform
	, const RGBAf& child_joint_color
	, const RGBAf& parent_joint_color
	) const
{
	static const RGBAf s_bone_colors[] = {
		RGBAf::RED,
		RGBAf::GREEN,
		RGBAf::BLUE,
		RGBAf::WHITE,
	};

	// root joint has index 0 so start with index 1
	for( int iJoint = 0; iJoint < _animated_joints.num(); iJoint++ )
	{
		const AnimatedJoint& joint = _animated_joints._data[ iJoint ];

		const V3f	transformed_joint_pos = M44_TransformPoint( model_transform, joint.position_relative_to_parent );

		//
		const bool can_use_debug_color = ( iJoint < mxCOUNT_OF(s_bone_colors) );
		const RGBAf	debug_color = can_use_debug_color ? s_bone_colors[ iJoint ] : RGBAf::WHITE;

		renderer.DrawCircle(
			transformed_joint_pos
			, camera_view.right_dir, camera_view.getUpDir()
			, can_use_debug_color ? debug_color : child_joint_color
			, 0.1f
			, 4
			);

		if( joint.parent >= 0 )
		{
			const Bone&	parent_bone = _bones._data[ joint.parent ];
			const V3f	transformed_parent_joint_pos = M44_TransformPoint( model_transform, parent_bone.position );

			//
			renderer.DrawLine(
				transformed_joint_pos, transformed_parent_joint_pos
				, can_use_debug_color ? debug_color : child_joint_color
				, can_use_debug_color ? debug_color : parent_joint_color
				);
		}
		else
		{
			// This is the root joint.
			renderer.DrawSternchen(
				transformed_joint_pos
				, camera_view.right_dir, camera_view.getUpDir()
				, can_use_debug_color ? debug_color : parent_joint_color
				, 0.5f
				);
		}
	}//For each joint.
}




template< typename KEY >
static
void findKeyFrameInterval(
				  const TSpan< const KEY >& keys
				  , const U32 current_time_msec
				  , KEY *min_key_
				  , KEY *max_key_
				  , float *lerp_fraction_
				  )
{
	mxASSERT( keys._count >= 2 );

	UINT iNextKey;

	// Locate the start of the interval.
	for( iNextKey = 0; iNextKey < keys._count; iNextKey++ )
	{
		const KEY& key = keys._data[ iNextKey ];

		if( key.time >= current_time_msec )
		{
			break;
		}

	}//For each rot key.

	//
	if( iNextKey == 0 )
	{
		const KEY& next_key = keys.getFirst();
		const KEY& prev_key = keys.getLast();

		// here we assume that the track is looping
		*min_key_ = prev_key;
		*max_key_ = next_key;

		if( next_key.time != 0.0f )
		{
			*lerp_fraction_ = float( next_key.time - current_time_msec ) / float( next_key.time );
		}
		else
		{
			*lerp_fraction_ = 0.0f;
		}
	}
	else
	{
		mxASSERT( keys.isValidIndex( iNextKey ) );
		const KEY& prev_key = keys[ iNextKey - 1 ];
		const KEY& next_key = keys[ iNextKey ];
		*min_key_ = prev_key;
		*max_key_ = next_key;
		*lerp_fraction_ = float( current_time_msec - prev_key.time ) / float( next_key.time - prev_key.time );
	}
}

ERet TestSkinnedModel::drawAnimatedSkeleton_Quat(
	ALineRenderer & renderer
	, ADebugTextRenderer& text_renderer
	, const NwView3D& camera_view
	, const M44f& model_transform
	, const RGBAf& child_joint_color
	, const RGBAf& parent_joint_color

	, const RF1::Anim& anim
	, const float current_time_ratio	// [0..1]
	) const
{
	const bool	align_skeleton_with_mesh = 1;

	// Rotate to align the skeleton with the mesh.
	const NwQuaternion rotate_around_X_by_180 = NwQuaternion::FromAxisAngle(
		CV3f(1,0,0),
		mxPI
		);

	const NwQuaternion rotate_to_align_skeleton_with_mesh =
		align_skeleton_with_mesh
		? rotate_around_X_by_180
		: Q4f::identity()
		;

	//
	const M44f	transform_and_align_skeleton_with_mesh
		= model_transform
		* RF1::buildMat4( rotate_to_align_skeleton_with_mesh, CV3f(0) )
		;

	//
	const U32	current_time_msec = (U32) ( current_time_ratio * (float) anim.end_time );

	//DBGOUT("current_time_msec = %u", current_time_msec);

	//
	AllocatorI &	scratchpad = MemoryHeaps::temporary();

	const U32	num_bones = _bones.num();
	mxASSERT( num_bones == anim.bone_tracks.num() );

	DynamicArray< TmpJoint >	tmp_joints( scratchpad );
	mxDO(tmp_joints.setNum( num_bones ));

	//
	for( UINT iBoneTrack = 0; iBoneTrack < num_bones; iBoneTrack++ )
	{
		const RF1::BoneTrack&	bone_track = anim.bone_tracks[ iBoneTrack ];

		//
		Q4f	interp_orientation;
		{
			RF1::RotKey	prev_rot_key;
			RF1::RotKey	next_rot_key;
			float		rot_key_frac;

			findKeyFrameInterval(
				Arrays::GetSpan(bone_track.rot_keys)
				, current_time_msec
				, &prev_rot_key
				, &next_rot_key
				, &rot_key_frac
				);

			//interp_orientation.SlerpSelf( prev_rot_key.q, next_rot_key.q, rot_key_frac );
			interp_orientation = Q4f::lerp( prev_rot_key.q, next_rot_key.q, rot_key_frac );

			interp_orientation = interp_orientation.normalized();
		}

		//
		V3f	interp_pos;
		{
			RF1::PosKey	prev_pos_key;
			RF1::PosKey	next_pos_key;
			float		pos_key_frac;

			findKeyFrameInterval(
				Arrays::GetSpan(bone_track.pos_keys)
				, current_time_msec
				, &prev_pos_key
				, &next_pos_key
				, &pos_key_frac
				);

			interp_pos = V3_Lerp( prev_pos_key.t, next_pos_key.t, pos_key_frac );
		}

		//
		//const UINT joint_index = _bone_to_joint_index[ iBoneTrack ];
		const UINT joint_index = iBoneTrack;
		const AnimatedJoint& anim_joint = _animated_joints._data[ joint_index ];

		//
		TmpJoint &dst_joint = tmp_joints[ joint_index ];
		dst_joint.relative_orientation =
			interp_orientation
			//Q4f::concat( anim_joint.orientation_relative_to_parent, interp_orientation )
			;
		dst_joint.relative_position =
			interp_pos
			//interp_pos + anim_joint.position_relative_to_parent
			;
		dst_joint.parent = anim_joint.parent;
		dst_joint.bone_index = anim_joint.bone_index;

		dst_joint.local_mat_rel_to_parent = RF1::buildMat4( interp_orientation, interp_pos );
		dst_joint.obj_space_joint_mat = dst_joint.local_mat_rel_to_parent;

	}//For each bone track.


#if 0
	for( UINT joint_index = 0; joint_index < num_bones; joint_index++ )
	{
		const AnimatedJoint& anim_joint = _animated_joints._data[ joint_index ];
		//
		TmpJoint &dst_joint = tmp_joints[ joint_index ];
		dst_joint.relative_orientation = anim_joint.orientation_relative_to_parent;
		dst_joint.relative_position = anim_joint.position_relative_to_parent;
		dst_joint.parent = anim_joint.parent;
		dst_joint.bone_index = anim_joint.bone_index;
	}
#endif

	////
	//tmp_joints._data[0].relative_orientation = _animated_joints[0].orientation_relative_to_parent;
	//tmp_joints._data[0].relative_position = _animated_joints[0].position_relative_to_parent;


	// The root joint has index 0 so start with index 1.
	for( UINT joint_index = 0; joint_index < num_bones; joint_index++ )
	{
		TmpJoint& joint = tmp_joints._data[ joint_index ];
		const Bone& bone = _bones[ joint.bone_index ];

		if( joint.parent >= 0 )
		{
			//
			const TmpJoint& parent_joint = tmp_joints[ joint.parent ];

			//
			// Concatenate transforms.
			V3f	joint_position
				= parent_joint.relative_orientation.rotateVector( joint.relative_position )
				+ parent_joint.relative_position
				;
			Q4f	joint_orientation = joint.relative_orientation <<= parent_joint.relative_orientation;
			joint_orientation.NormalizeSelf();

			//
			joint.relative_orientation = joint_orientation;
			joint.relative_position = joint_position;




			//
			const V3f	rotated_joint_position = joint_orientation.rotateVector( joint_position );

			//
			const V3f	transformed_joint_pos = M44_TransformPoint(
				transform_and_align_skeleton_with_mesh,
				rotated_joint_position
				);

			//
			renderer.DrawSternchen(
				transformed_joint_pos
				, camera_view.right_dir, camera_view.getUpDir()
				, parent_joint_color
				, 0.1f
				);

			//
			const V3f	rotated_parent_position = parent_joint.relative_orientation.rotateVector( parent_joint.relative_position );
			const V3f	transformed_parent_joint_pos = M44_TransformPoint(
				transform_and_align_skeleton_with_mesh,
				rotated_parent_position
				);

			//
			renderer.DrawLine(
				transformed_joint_pos, transformed_parent_joint_pos
				, child_joint_color, parent_joint_color
				);

			//
			text_renderer.drawText(
				transformed_joint_pos
				, RGBAf::WHITE
				, nil
				, 1.0f
				, "%s", bone.name.c_str()
				);
		}//If has parent.
		else
		{
			//
			const V3f	rotated_joint_position = joint.relative_orientation.rotateVector( joint.relative_position );

			//
			const V3f	transformed_joint_pos = M44_TransformPoint(
				transform_and_align_skeleton_with_mesh,
				rotated_joint_position
				);

			//
			renderer.DrawSternchen(
				transformed_joint_pos
				, camera_view.right_dir, camera_view.getUpDir()
				, parent_joint_color
				, 0.1f
				);

			//
			text_renderer.drawText(
				transformed_joint_pos
				, RGBAf::WHITE
				, nil
				, 1.0f
				, "%s", bone.name.c_str()
				);
		}

	}//For each joint.

	return ALL_OK;
}

struct AnimTrackInfo
{
	const char *	name;
	int				num_rot_samples;
	int				num_pos_samples;
};

ERet TestSkinnedModel::drawAnimatedSkeleton_Mat(
	ALineRenderer & renderer
	, ADebugTextRenderer& text_renderer
	, const NwView3D& camera_view
	, const M44f& model_transform
	, const RGBAf& child_joint_color
	, const RGBAf& parent_joint_color

	, const RF1::Anim& anim
	, const float current_time_ratio	// [0..1]
	) const
{
	//
	const U32	current_time_msec = (U32) ( current_time_ratio * (float) anim.end_time );

	//DBGOUT("current_time_msec = %u", current_time_msec);

	//
	AllocatorI &	scratchpad = MemoryHeaps::temporary();

	const U32	num_bones = _bones.num();
	mxASSERT( num_bones == anim.bone_tracks.num() );

	DynamicArray< TmpJoint >	tmp_joints( scratchpad );
	mxDO(tmp_joints.setNum( num_bones ));

	//
	AnimTrackInfo	track_infos[ RF1::MAX_BONES ];
	mxZERO_OUT(track_infos);

	//
	for( UINT iBoneTrack = 0; iBoneTrack < num_bones; iBoneTrack++ )
	{
		const RF1::BoneTrack&	bone_track = anim.bone_tracks[ iBoneTrack ];

		//
		Q4f	interp_orientation;
		{
			RF1::RotKey	prev_rot_key;
			RF1::RotKey	next_rot_key;
			float		rot_key_frac;

			findKeyFrameInterval(
				Arrays::GetSpan(bone_track.rot_keys)
				, current_time_msec
				, &prev_rot_key
				, &next_rot_key
				, &rot_key_frac
				);

			interp_orientation.SlerpSelf( prev_rot_key.q, next_rot_key.q, rot_key_frac );
			//interp_orientation = Q4f::lerp( prev_rot_key.q, next_rot_key.q, rot_key_frac );

			interp_orientation = interp_orientation.normalized();
		}

		//
		V3f	interp_pos;
		{
			RF1::PosKey	prev_pos_key;
			RF1::PosKey	next_pos_key;
			float		pos_key_frac;

			findKeyFrameInterval(
				Arrays::GetSpan(bone_track.pos_keys)
				, current_time_msec
				, &prev_pos_key
				, &next_pos_key
				, &pos_key_frac
				);

			interp_pos = V3_Lerp( prev_pos_key.t, next_pos_key.t, pos_key_frac );
		}

		//
		//const UINT joint_index = _bone_to_joint_index[ iBoneTrack ];
		const UINT joint_index = iBoneTrack;
		const AnimatedJoint& anim_joint = _animated_joints._data[ joint_index ];
		mxASSERT(anim_joint.bone_index == iBoneTrack);
		const Bone& bone = _bones[ anim_joint.bone_index ];

		//
		TmpJoint &dst_joint = tmp_joints[ joint_index ];
		dst_joint.relative_orientation =
			interp_orientation//.inverse()
			;
		dst_joint.relative_position =
			interp_pos
			;
		dst_joint.parent = anim_joint.parent;
		dst_joint.bone_index = anim_joint.bone_index;

#if 1
		//
		M44f	local_mat_rel_to_parent =
			//M44_Inverse( anim_joint.local_mat_rel_to_parent ) *
			//M44_Inverse( anim_joint.local_mat_rel_to_parent ) *
			RF1::buildMat4( interp_orientation, interp_pos )
			//anim_joint.local_mat_rel_to_parent
			;
		dst_joint.local_mat_rel_to_parent = local_mat_rel_to_parent;
		dst_joint.obj_space_joint_mat = local_mat_rel_to_parent;
#endif

		//
		track_infos[ iBoneTrack ].name = bone.name.c_str();
		track_infos[ iBoneTrack ].num_rot_samples = bone_track.rot_keys.num();
		track_infos[ iBoneTrack ].num_pos_samples = bone_track.pos_keys.num();

	}//For each bone track.


	////
	//tmp_joints._data[ 0 ].obj_space_joint_mat = _animated_joints[0].local_mat_rel_to_parent;
	//tmp_joints._data[ 0 ].local_mat_rel_to_parent = M44_Identity();

	//tmp_joints._data[ 0 ].obj_space_joint_mat
	//	= tmp_joints._data[ 0 ].obj_space_joint_mat
	//	;
	//M44_Inverse( anim_joint.local_mat_rel_to_parent ) *



	// The root joint has index 0 so start with index 1.
	for( UINT joint_index = 0; joint_index < num_bones; joint_index++ )
	{
		TmpJoint& joint = tmp_joints._data[ joint_index ];
		const Bone& bone = _bones[ joint.bone_index ];

		//
		V3f	transformed_joint_pos;

		//
		if( joint.parent != PARENT_NONE )
		{
			const TmpJoint& parent_joint = tmp_joints[ joint.parent ];
			const Bone& parent_bone = _bones[ parent_joint.bone_index ];

			//
			joint.obj_space_joint_mat =
				parent_joint.obj_space_joint_mat * joint.local_mat_rel_to_parent
				;

			//
			transformed_joint_pos = M44_TransformPoint(
				model_transform,
				RF1::getRotatedPointFromBoneMat(
				/*M44_Inverse(bone.obj_space_joint_mat) **/ joint.obj_space_joint_mat
				)
				);

			//
			const V3f	transformed_parent_joint_pos = M44_TransformPoint(
				model_transform,
				RF1::getRotatedPointFromBoneMat(
				/*M44_Inverse(parent_bone.obj_space_joint_mat) **/ parent_joint.obj_space_joint_mat
				)
				);

			//
			renderer.DrawLine(
				transformed_joint_pos, transformed_parent_joint_pos
				, child_joint_color, parent_joint_color
				);
		}
		else
		{
			//
			transformed_joint_pos = M44_TransformPoint(
				model_transform,
				RF1::getRotatedPointFromBoneMat(
				/*M44_Inverse(bone.obj_space_joint_mat) **/ joint.obj_space_joint_mat
				)
				);
		}

		//
		renderer.DrawSternchen(
			transformed_joint_pos
			, camera_view.right_dir, camera_view.getUpDir()
			, child_joint_color
			, 0.1f
			);

		//
		text_renderer.drawText(
			transformed_joint_pos
			, RGBAf::WHITE
			, nil
			, 1.0f
			, "%s", bone.name.c_str()
			);

	}//For each joint.

	return ALL_OK;
}

const M44f JointTransform::getLocalTransform() const
{
	return RF1::buildMat4(
		orientation_relative_to_parent,
		position_relative_to_parent
		);
}

static
const M44f getJointTransform_recursive(
	const TSpan< const JointTransform >& joints,
	const int joint_index
	)
{
	const JointTransform& joint = joints[ joint_index ];

	//
	const M44f joint_local_xform = joint.getLocalTransform();
//return joint_local_xform;
	//
	if( joint.parent >= 0 )
	{
		return getJointTransform_recursive( joints, joint.parent ) * joint_local_xform;
	}
	else
	{
		return joint_local_xform;
	}
}

ERet TestSkinnedModel::drawAnimatedSkeleton_Quat2(
	ALineRenderer & renderer
	, ADebugTextRenderer& text_renderer
	, const NwView3D& camera_view
	, const M44f& model_transform
	, const RGBAf& child_joint_color
	, const RGBAf& parent_joint_color

	, const RF1::Anim& anim
	, const float current_time_ratio	// [0..1]
	) const
{
	const bool	align_skeleton_with_mesh = 1;

	// Rotate to align the skeleton with the mesh.
	const NwQuaternion rotate_around_X_by_180 = NwQuaternion::FromAxisAngle(
		CV3f(1,0,0),
		mxPI
		);

	const NwQuaternion rotate_to_align_skeleton_with_mesh =
		align_skeleton_with_mesh
		? rotate_around_X_by_180
		: Q4f::identity()
		;

	//
	const M44f	transform_and_align_skeleton_with_mesh
		= model_transform
		* RF1::buildMat4( rotate_to_align_skeleton_with_mesh, CV3f(0) )
		;

	//
	const U32	current_time_msec = (U32) ( current_time_ratio * (float) anim.end_time );

	//
	const U32	num_bones = _bones.num();

	//
	TFixedArray< JointTransform, RF1::MAX_BONES >	joints;
	mxDO(joints.setNum( num_bones ));

	//
	for( UINT iBone = 0; iBone < num_bones; iBone++ )
	{
		const RF1::BoneTrack&	bone_track = anim.bone_tracks[ iBone ];

		//
		Q4f	interp_orientation;
		{
			RF1::RotKey	prev_rot_key;
			RF1::RotKey	next_rot_key;
			float		rot_key_frac;

			findKeyFrameInterval(
				Arrays::GetSpan(bone_track.rot_keys)
				, current_time_msec
				, &prev_rot_key
				, &next_rot_key
				, &rot_key_frac
				);

			//interp_orientation.SlerpSelf( prev_rot_key.q, next_rot_key.q, rot_key_frac );
			interp_orientation = Q4f::lerp( prev_rot_key.q, next_rot_key.q, rot_key_frac );

			interp_orientation = interp_orientation.normalized();
		}

		//
		V3f	interp_pos;
		{
			RF1::PosKey	prev_pos_key;
			RF1::PosKey	next_pos_key;
			float		pos_key_frac;

			findKeyFrameInterval(
				Arrays::GetSpan(bone_track.pos_keys)
				, current_time_msec
				, &prev_pos_key
				, &next_pos_key
				, &pos_key_frac
				);

			interp_pos = V3_Lerp( prev_pos_key.t, next_pos_key.t, pos_key_frac );
		}

		//
		const Bone& bone = _bones[ iBone ];

		//
		JointTransform &joint = joints[ iBone ];
		joint.orientation_relative_to_parent =
			interp_orientation
			//Q4f::concat( anim_joint.orientation_relative_to_parent, interp_orientation )
			;
		joint.position_relative_to_parent =
			interp_pos
			//interp_pos + anim_joint.position_relative_to_parent
			;
		joint.parent = bone.parent;

	}//For each bone track.


	// The root joint has index 0 so start with index 1.
	for( UINT bone_index = 0; bone_index < num_bones; bone_index++ )
	{
		//
		const Bone& bone = _bones._data[ bone_index ];

		//
		JointTransform & joint = joints._data[ bone_index ];

		const M44f joint_model_xform = getJointTransform_recursive( joints, bone_index );

		//
		const V3f	transformed_joint_pos = M44_getTranslation(
			joint_model_xform * transform_and_align_skeleton_with_mesh
			);

		//
		renderer.DrawSternchen(
			transformed_joint_pos
			, camera_view.right_dir, camera_view.getUpDir()
			, parent_joint_color
			, 0.1f
			);

		//
		text_renderer.drawText(
			transformed_joint_pos
			, RGBAf::WHITE
			, nil
			, 1.0f
			, "%s", bone.name.c_str()
			);

		//
		if( joint.parent >= 0 )
		{
			//
			const M44f parent_joint_model_xform = getJointTransform_recursive( joints, joint.parent );

			const V3f	transformed_parent_joint_pos = M44_getTranslation(
				parent_joint_model_xform * transform_and_align_skeleton_with_mesh
				);

			//
			renderer.DrawLine(
				transformed_joint_pos, transformed_parent_joint_pos
				, child_joint_color, parent_joint_color
				);
		}
		else
		{
			int a;
			a = 0;
		}


#if 0	// WRONG

		//
		if( joint.parent >= 0 )
		{
			//
			const JointTransform& parent_joint = joints._data[ joint.parent ];

			//
			// Concatenate transforms.
			V3f	joint_position
				= parent_joint.orientation_relative_to_parent.rotateVector( joint.position_relative_to_parent )
				+ parent_joint.position_relative_to_parent
				;
			Q4f	joint_orientation = joint.orientation_relative_to_parent <<= parent_joint.orientation_relative_to_parent;
			joint_orientation.NormalizeSelf();


			//
			joint.orientation_relative_to_parent = joint_orientation;
			joint.position_relative_to_parent = joint_position;


			//
			const V3f	rotated_joint_position = joint_orientation.rotateVector( joint_position );

			//
			const V3f	transformed_joint_pos = M44_TransformPoint(
				transform_and_align_skeleton_with_mesh,
				rotated_joint_position
				);

			//
			renderer.DrawSternchen(
				transformed_joint_pos
				, camera_view.right_dir, camera_view.getUpDir()
				, parent_joint_color
				, 0.1f
				);

			//
			const V3f	rotated_parent_position = parent_joint.orientation_relative_to_parent
				.rotateVector( parent_joint.position_relative_to_parent );
			const V3f	transformed_parent_joint_pos = M44_TransformPoint(
				transform_and_align_skeleton_with_mesh,
				rotated_parent_position
				);

			//
			renderer.DrawLine(
				transformed_joint_pos, transformed_parent_joint_pos
				, child_joint_color, parent_joint_color
				);

			//
			text_renderer.drawText(
				transformed_joint_pos
				, RGBAf::WHITE
				, nil
				, 1.0f
				, "%s", bone.name.c_str()
				);
		}//If has parent.
		else
		{
			//
			const V3f	rotated_joint_position = joint.orientation_relative_to_parent
				.rotateVector( joint.position_relative_to_parent );

			//
			const V3f	transformed_joint_pos = M44_TransformPoint(
				transform_and_align_skeleton_with_mesh,
				rotated_joint_position
				);

			//
			renderer.DrawSternchen(
				transformed_joint_pos
				, camera_view.right_dir, camera_view.getUpDir()
				, parent_joint_color
				, 0.1f
				);

			//
			text_renderer.drawText(
				transformed_joint_pos
				, RGBAf::WHITE
				, nil
				, 1.0f
				, "%s", bone.name.c_str()
				);
		}
#endif

	}//For each joint.

	return ALL_OK;
}
