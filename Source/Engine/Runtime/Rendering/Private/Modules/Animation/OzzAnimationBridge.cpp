/*
=============================================================================
	Graphics model used for rendering.
=============================================================================
*/
#include <Base/Base.h>
#pragma hdrstop
#include <Rendering/Private/Modules/Animation/OzzAnimationBridge.h>

//#pragma comment( lib, "ozz-animation.lib" )


namespace Rendering
{

int getJointIndexByName(
						const char* joint_name
						, const ozz::animation::Skeleton& skeleton
						)
{
	const ozz::Range<const char* const> joint_names = skeleton.joint_names();

	int joint_index = -1;

	for( int i = 0; i < joint_names.count(); i++ )
	{
		if( strcmp( joint_names[i], joint_name ) == 0 )
		{
			joint_index = i;
			break;
		}
	}

	return joint_index;
}


#if 0

void dbg_drawSkeletonInBindPose(const ozz::animation::Skeleton& _skeleton,
								//const ozz::math::Float4x4& _transform,
								TbPrimitiveBatcher & debugRenderer)
{
	const int num_joints = _skeleton.num_joints();
	const ozz::animation::Skeleton::JointProperties* properties =
		_skeleton.joint_properties().begin;

	for (int i = 0; i < num_joints; ++i)
	{
		// Root isn'bindPoseLocalAabbHalfSize rendered.
		const int parent_id = properties[i].parent;
		if (parent_id == ozz::animation::Skeleton::kNoParentIndex) {
			continue;
		}

		// Selects joint matrices.
		const ozz::math::Transform& parent_xform = ozz::animation::GetJointLocalBindPose( _skeleton, parent_id );
		const ozz::math::Transform& current_xform = ozz::animation::GetJointLocalBindPose( _skeleton, i );

		debugRenderer.DrawLine(ozz_test::getTranslation(parent_xform), ozz_test::getTranslation(current_xform)
			, RGBAf::WHITE, RGBAf::DARK_YELLOW_GREEN);
	}
}

bool dbg_drawPosture(const ozz::animation::Skeleton& _skeleton,
					 ozz::Range<const ozz::math::Float4x4> _matrices,
					 //const ozz::math::Float4x4& _transform,
					 TbPrimitiveBatcher & debugRenderer,
					 bool _draw_joints)
{
	if (!_matrices.begin || !_matrices.end) {
		return false;
	}
	if (_matrices.end - _matrices.begin < _skeleton.num_joints()) {
		return false;
	}

	const int num_joints = _skeleton.num_joints();
	const ozz::animation::Skeleton::JointProperties* properties =
		_skeleton.joint_properties().begin;

	for (int i = 0; i < num_joints; ++i)
	{
		// Root isn'bindPoseLocalAabbHalfSize rendered.
		const int parent_id = properties[i].parent;
		if (parent_id == ozz::animation::Skeleton::kNoParentIndex) {
			continue;
		}

		// Selects joint matrices.
		const ozz::math::Float4x4& parent = _matrices.begin[parent_id];
		const ozz::math::Float4x4& current = _matrices.begin[i];

		debugRenderer.DrawLine(ozz_test::getTranslation(parent), ozz_test::getTranslation(current)
			, RGBAf::WHITE, RGBAf::GREEN);
	}
	return true;
}

void dbg_drawSkeletalAnim( const ozz_test::ExampleData& anim_data
						  , TbPrimitiveBatcher & debugRenderer )
{
	dbg_drawPosture(anim_data._skeleton, anim_data.models_, debugRenderer, false);
}

#endif // TB_USE_OZZ_ANIMATION

}//namespace Rendering
