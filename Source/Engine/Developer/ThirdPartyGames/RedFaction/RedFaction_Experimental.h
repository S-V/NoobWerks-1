#pragma once

#include <Base/Math/Quaternion.h>
#include <Base/Template/Containers/Array/TInplaceArray.h>
#include <Graphics/Public/graphics_utilities.h>	// ALineRenderer






namespace RF1
{
	struct Anim;


}//namespace RF1


struct JointTransform
{
	Q4f		orientation_relative_to_parent;	// joint's orientation quaternion relative to the parent joint
	V3f		position_relative_to_parent;	// joint's position relative to the parent joint
	int		parent;			// index of the parent joint; this is the root joint if (-1)

public:
	const M44f getLocalTransform() const;
};
ASSERT_SIZEOF(JointTransform, 32);

///
class TestSkinnedModel: NonCopyable
{
public:
	struct Vertex
	{
		V3f		pos;
		int		bone_indices[4];
		float	bone_weights[4];
	};

	//
	DynamicArray< Vertex >	_vertices;
	DynamicArray< UInt3 >	_triangles;

	//
	DynamicArray< V3f >		_transformed_vertices;

	//
	struct Submesh
	{
		//
	};
	DynamicArray< Submesh >	_submeshes;

	/// Represents a bind-pose skeleton's joint in object-local space.
	struct Bone
	{
		Q4f			orientation;	// joint's orientation quaternion in model/object space
		V3f			position;		// joint's position in model/object space
		int			parent;			// index of the parent joint; this is the root joint if (-1)

		String32	name;

		M44f		obj_space_joint_mat;

	//	Q4f		orientation_relative_to_parent;
	};
	DynamicArray< Bone >	_bones;

	///
	struct AnimatedJoint
	{
		Q4f		orientation_relative_to_parent;	// joint's orientation quaternion relative to the parent joint
		V3f		position_relative_to_parent;	// joint's position relative to the parent joint
		int		parent;			// index of the parent joint; this is the root joint if (-1)

		int		bone_index;

		String32	name;

		M44f		local_mat_rel_to_parent;
	};

	// joints are sorted by depth in the hierarchy (so the root is placed first)
	DynamicArray< AnimatedJoint >	_animated_joints;

	DynamicArray< U8 >	_bone_to_joint_index;

	static const int PARENT_NONE = -1;




	//
	struct Bone3: ReferenceCounted
	{
		Q4f			orientation;	// joint's orientation quaternion in model/object space
		V3f			position;		// joint's position in model/object space
		int			parent;			// index of the parent joint; this is the root joint if (-1)

		String32	name;

		TInplaceArray< TRefPtr< Bone3 >, 8 >		kids;
	};
	Bone3	_root_bone;


public:
	TestSkinnedModel( AllocatorI & allocator );


	//
	bool isSkinned() const { return !_bones.IsEmpty(); }


public:

	enum EDrawFlags
	{
		DrawSkeleton = BIT(1),
		DrawEverything = BITS_ALL
	};

	void draw(
		ALineRenderer & renderer
		, const M44f& model_transform
		, const RGBAf& color
		, int flags = DrawEverything
		) const;

	void drawBindPoseMesh(
		ALineRenderer & renderer
		, const M44f& model_transform
		, const RGBAf& color
		) const;

	void drawOriginalBones_Quat(
		ALineRenderer & renderer
		, ADebugTextRenderer& text_renderer
		, const NwView3D& camera_view
		, const M44f& model_transform
		, const RGBAf& child_joint_color
		, const RGBAf& parent_joint_color
		) const;

	void drawOriginalBones_Mat(
		ALineRenderer & renderer
		, ADebugTextRenderer& text_renderer
		, const NwView3D& camera_view
		, const M44f& model_transform
		, const RGBAf& child_joint_color
		, const RGBAf& parent_joint_color
		) const;

	void drawBindPoseSkeleton(
		ALineRenderer & renderer
		, const NwView3D& camera_view
		, const M44f& model_transform
		, const RGBAf& child_joint_color
		, const RGBAf& parent_joint_color
		) const;

	ERet drawAnimatedSkeleton_Quat(
		ALineRenderer & renderer
		, ADebugTextRenderer& text_renderer
		, const NwView3D& camera_view
		, const M44f& model_transform
		, const RGBAf& child_joint_color
		, const RGBAf& parent_joint_color

		, const RF1::Anim& anim
		, const float current_time_ratio	// [0..1]
		) const;

	ERet drawAnimatedSkeleton_Mat(
		ALineRenderer & renderer
		, ADebugTextRenderer& text_renderer
		, const NwView3D& camera_view
		, const M44f& model_transform
		, const RGBAf& child_joint_color
		, const RGBAf& parent_joint_color

		, const RF1::Anim& anim
		, const float current_time_ratio	// [0..1]
		) const;

	ERet drawAnimatedSkeleton_Quat2(
		ALineRenderer & renderer
		, ADebugTextRenderer& text_renderer
		, const NwView3D& camera_view
		, const M44f& model_transform
		, const RGBAf& child_joint_color
		, const RGBAf& parent_joint_color

		, const RF1::Anim& anim
		, const float current_time_ratio	// [0..1]
		) const;

public:
	ERet computeBindPoseJoints();
};
