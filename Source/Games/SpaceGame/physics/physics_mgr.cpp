//
#include "stdafx.h"
#pragma hdrstop

#define XM_NO_OPERATOR_OVERLOADS
#include <XNAMath/xnamath.h>

#include <Base/Math/BoundingVolumes/ViewFrustum.h>

#include <Core/Util/Tweakable.h>

// TbPrimitiveBatcher
#include <Graphics/graphics_utilities.h>


#include "experimental/game_experimental.h"
#include "physics/physics_mgr.h"
#include "physics/aabb_tree.h"
#include "game_app.h"


//
#define B3_USE_SSE
#define B3_USE_SSE_IN_API
#define B3_NO_SIMD_OPERATOR_OVERLOADS

#include <Bullet3Common/shared/b3Float4.h>
#include <Bullet3Common/shared/b3Quat.h>
#include <Bullet3Common/shared/b3Mat3x3.h>
#include <Bullet3Common/b3Matrix3x3.h>
#include <Bullet3Collision/BroadPhaseCollision/shared/b3Aabb.h>

#include <Physics/Bullet_Wrapper.h>

mxSTOLEN("heavily based on experimental Bullet3 code");


namespace
{
	enum { SIMD_ALIGNMENT = 16 };

	struct b4RigidBodyData
	{
		b3Float4	m_pos;
		b3Quat		m_quat;
		b3Float4	linear_velocity;
		b3Float4	angular_velocity;
		//64

		ColShapeHandle		col_shape_idx;
		float				m_invMass;
		//float				restitution_coeff;
		//float				m_frictionCoeff;

		//
		RigidBodyUserIndex	user_obj_index;
		RigidBodyUserType	user_obj_type;

		//
		RigidBodyHandle		my_handle;
	};//96
	//tbPRINT_SIZE_OF(b4RigidBodyData);

	struct b4Inertia
	{
		b3Mat3x3 m_invInertiaWorld;
		b3Mat3x3 m_initInvInertia;
	};

	struct b4ColShape
	{
		b3Aabb	aabb_local;
	};

}//namespace



///
struct SgPhysicsMgrInternalData
{
	NwHandleTable	body_handle_mgr;

	b4RigidBodyData *	rigid_bodies;
	//b4Inertia *			m_inertias;
	b3Aabb *			aabbs_world_space;

	U32					num_col_shapes;
	b4ColShape *		col_shapes;

	PhysConfig			cfg;

public:
	SgPhysicsMgrInternalData(AllocatorI& allocator)
		: body_handle_mgr(allocator)
	{}
};

//tbPRINT_SIZE_OF(SgPhysicsMgrInternalData);



mxFORCEINLINE b4RigidBodyData& GetRigidBodyDataByHandle(
	const RigidBodyHandle& rigid_body_handle
	, const SgPhysicsMgr& phys_mgr
	)
{
	SgPhysicsMgrInternalData& mydata = *phys_mgr._data;
	const U32 rigid_body_index = mydata.body_handle_mgr.GetItemIndexByHandle(rigid_body_handle.id);
	return mydata.rigid_bodies[ rigid_body_index ];
}



SgPhysicsMgr::SgPhysicsMgr()
{
	_aabb_tree = nil;
}

SgPhysicsMgr::~SgPhysicsMgr()
{
}

ERet SgPhysicsMgr::Initialize(
	const PhysConfig& phys_config
	, const SgAppSettings& game_settings
	, NwClump* scene_clump
	, AllocatorI& allocator
	)
{
	mxDO(nwCREATE_OBJECT(
		_data
		, allocator
		, SgPhysicsMgrInternalData
		, allocator
		));

	//
	SgAABBTreeConfig	aabb_tree_cfg;
	aabb_tree_cfg.max_item_count = phys_config.max_bodies;
	aabb_tree_cfg.max_items_per_leaf = 1;

	mxDO(SgAABBTree::Create(
		_aabb_tree
		, aabb_tree_cfg
		, allocator
		));

	//
	SgPhysicsMgrInternalData& mydata = *_data;

	mxDO(mydata.body_handle_mgr.InitWithMaxCount(
		phys_config.max_bodies
		));

	//
	mxDO(nwAllocArray(
		mydata.rigid_bodies
		, phys_config.max_bodies
		, allocator
		, SIMD_ALIGNMENT
		));
	//mxDO(nwAllocArray(
	//	mydata.m_inertias
	//	, phys_config.max_bodies
	//	, allocator
	//	, SIMD_ALIGNMENT
	//	));
	mxDO(nwAllocArray(
		mydata.aabbs_world_space
		, phys_config.max_bodies
		, allocator
		, SIMD_ALIGNMENT
		));

	//
	mydata.num_col_shapes = 0;
	mxDO(nwAllocArray(
		mydata.col_shapes
		, phys_config.max_col_shapes
		, allocator
		, SIMD_ALIGNMENT
		));

	mydata.cfg = phys_config;


	//
	ColShapeHandle	col_shape_handle;
	mxDO(this->CreateColShape_Sphere(
		col_shape_handle
		, 1
		));

	return ALL_OK;
}

void SgPhysicsMgr::Shutdown()
{
	SgPhysicsMgrInternalData& mydata = *_data;

	AllocatorI& allocator = mydata.body_handle_mgr._allocator;

	//
	mxDELETE_AND_NIL(mydata.rigid_bodies, allocator);
	//mxDELETE_AND_NIL(mydata.m_inertias, allocator);
	mxDELETE_AND_NIL(mydata.aabbs_world_space, allocator);

	//
	mxDELETE_AND_NIL(mydata.col_shapes, allocator);
	mydata.num_col_shapes = 0;

	mydata.body_handle_mgr.Shutdown();

	//
	SgAABBTree::Destroy(_aabb_tree);
	_aabb_tree = nil;

	//
	nwDESTROY_OBJECT(_data, allocator);
	_data = nil;
}

void SgPhysicsMgr::RemoveAllRigidBodies()
{
	_data->body_handle_mgr.ResetState();
}

ERet SgPhysicsMgr::CreateColShape_Sphere(
	ColShapeHandle &col_shape_handle_
	, float radius
	)
{
	SgPhysicsMgrInternalData& mydata = *_data;

	mxENSURE(mydata.num_col_shapes < mydata.cfg.max_col_shapes
		, ERR_BUFFER_TOO_SMALL, ""
		);

	b4ColShape &new_col_shape = mydata.col_shapes[ mydata.num_col_shapes ];
	new_col_shape.aabb_local.m_minVec.mVec128 = V4_REPLICATE(-radius);
	new_col_shape.aabb_local.m_maxVec.mVec128 = V4_REPLICATE(+radius);

	//
	col_shape_handle_.id = mydata.num_col_shapes;

	mydata.num_col_shapes++;

	return ALL_OK;
}

ERet SgPhysicsMgr::CreateColShape_Box(
	ColShapeHandle &col_shape_handle_
	, const AABBf& aabb_local
	)
{
	SgPhysicsMgrInternalData& mydata = *_data;

	mxENSURE(mydata.num_col_shapes < mydata.cfg.max_col_shapes
		, ERR_BUFFER_TOO_SMALL, ""
		);

	b4ColShape &new_col_shape = mydata.col_shapes[ mydata.num_col_shapes ];
	new_col_shape.aabb_local.m_minVec.mVec128 = Vector4_LoadFloat3_Unaligned(aabb_local.min_corner.a);
	new_col_shape.aabb_local.m_maxVec.mVec128 = Vector4_LoadFloat3_Unaligned(aabb_local.max_corner.a);

	//
	col_shape_handle_.id = mydata.num_col_shapes;

	mydata.num_col_shapes++;

	return ALL_OK;
}

ERet SgPhysicsMgr::CreateRigidBody(
	RigidBodyHandle &rigid_body_handle_
	, const ColShapeHandle col_shape_handle
	, const RigidBodyUserType user_obj_type
	, const RigidBodyUserIndex user_obj_index
	, const float mass
	, const V3f& pos
	)
{
	mxASSERT(mass > 0);

	//
	SgPhysicsMgrInternalData& mydata = *_data;

	//
	const UINT new_rigid_body_index = mydata.body_handle_mgr._num_alive_items;

	RigidBodyHandle	new_rigid_body_handle;
	mxDO(mydata.body_handle_mgr.AllocHandle(
		new_rigid_body_handle.id
		));

	//
	rigid_body_handle_ = new_rigid_body_handle;

	//
	b4RigidBodyData &new_rb = mydata.rigid_bodies[ new_rigid_body_index ];
	new_rb.m_pos.mVec128 = Vector4_LoadFloat3_Unaligned( pos.a );
	new_rb.m_quat = b3Quat::getIdentity();
	new_rb.linear_velocity.mVec128 = g_MM_Zero;
	new_rb.angular_velocity.mVec128 = g_MM_Zero;

	new_rb.col_shape_idx = col_shape_handle;
	new_rb.m_invMass = 1.f / mass;
	//new_rb.restitution_coeff = 0.1f;
	//new_rb.m_frictionCoeff = 0.9f;

	//
	new_rb.user_obj_index = user_obj_index;
	new_rb.user_obj_type = user_obj_type;

	new_rb.my_handle = new_rigid_body_handle;

	//
	//mydata.aabbs_world_space[ new_rigid_body_index ] = ?;

	return ALL_OK;
}

ERet SgPhysicsMgr::DeleteRigidBody(
	const RigidBodyHandle rigid_body_handle
	)
{
	SgPhysicsMgrInternalData& mydata = *_data;

	U32	indices_of_swapped_items[2];
	mxDO(mydata.body_handle_mgr.DestroyHandle(
		rigid_body_handle.id
		, mydata.rigid_bodies
		, indices_of_swapped_items
		));

	if(indices_of_swapped_items[0] != indices_of_swapped_items[1])
	{
		mydata.aabbs_world_space[ indices_of_swapped_items[0] ]
		= mydata.aabbs_world_space[ indices_of_swapped_items[1] ]
		;
	}

	return ALL_OK;
}

const StridedRBInstanceData SgPhysicsMgr::GetRBInstancesData() const
{
	SgPhysicsMgrInternalData& mydata = *_data;

	return StridedRBInstanceData::FromArray(
		mydata.rigid_bodies,
		mydata.body_handle_mgr._num_alive_items
		);
}

ERet SgPhysicsMgr::GetRigidBodyCenterOfMassPosition(
	Vector4 *pos_
	, const RigidBodyHandle& rigid_body_handle
	) const
{
	const b4RigidBodyData& rb = GetRigidBodyDataByHandle(rigid_body_handle, *this);
	*pos_ = rb.m_pos.mVec128;
	return ALL_OK;
}

const V3f SgPhysicsMgr::GetRigidBodyPredictedPosition(
	const RigidBodyHandle& rigid_body_handle
	, const NwTime& game_time
) const
{
	const b4RigidBodyData& rb = GetRigidBodyDataByHandle(rigid_body_handle, *this);

	return Vector4_As_V3(rb.m_pos.mVec128)
		+ Vector4_As_V3(rb.linear_velocity.mVec128) * game_time.real.delta_seconds
		;
}

const Vector4 SgPhysicsMgr::GetRigidBodyCenterOfMassPosition(
	const RigidBodyHandle& rigid_body_handle
) const
{
	b4RigidBodyData &rb = GetRigidBodyDataByHandle(rigid_body_handle, *this);
	return rb.m_pos.mVec128;
}

const Vector4 SgPhysicsMgr::GetRigidBodyForwardDirection(
	const RigidBodyHandle& rigid_body_handle
) const
{
	b4RigidBodyData &rb = GetRigidBodyDataByHandle(rigid_body_handle, *this);

	const b3Vector3	local_forward_dir = {V4_Forward};
	const b3Vector3 current_forward_dir = b3QuatRotate(rb.m_quat, local_forward_dir).normalized();
	return current_forward_dir.mVec128;
}

ERet SgPhysicsMgr::GetRigidBodyPosAndQuat(
	const RigidBodyHandle& rigid_body_handle
	, SgRBInstanceData &instance_data_
	) const
{
	const b4RigidBodyData& rb = GetRigidBodyDataByHandle(rigid_body_handle, *this);

	instance_data_.pos = rb.m_pos.mVec128;
	instance_data_.quat = rb.m_quat.mVec128;

	return ALL_OK;
}

ERet SgPhysicsMgr::GetRigidBodyTransform(
	M44f &transform_
	, const RigidBodyHandle& rigid_body_handle
	) const
{
	const b4RigidBodyData& rb = GetRigidBodyDataByHandle(rigid_body_handle, *this);

	// based on btTransform::getOpenGLMatrix()
	b3Mat3x3	basis( rb.m_quat );
	basis.getOpenGLSubMatrix(transform_.a);

	transform_.a[12] = rb.m_pos.x;
	transform_.a[13] = rb.m_pos.y;
	transform_.a[14] = rb.m_pos.z;
	transform_.a[15] = 1.0f;

	return ALL_OK;
}

ERet SgPhysicsMgr::SetRigidBodyCenterOfMassPosition(
	const V3f& com_pos
	, const RigidBodyHandle& rigid_body_handle
	) const
{
	b4RigidBodyData &rb = GetRigidBodyDataByHandle(rigid_body_handle, *this);
	rb.m_pos.mVec128 = Vector4_LoadFloat3_Unaligned(com_pos.a);
	return ALL_OK;
}

ERet SgPhysicsMgr::SetRigidBodyFacingDirection(
	const RigidBodyHandle& rigid_body_handle
	, const Vector4& desired_direction
	)
{
	b4RigidBodyData &rb = GetRigidBodyDataByHandle(rigid_body_handle, *this);

	const b3Vector3	local_forward_dir = {V4_Forward};
	const b3Vector3 current_forward_dir = b3QuatRotate(rb.m_quat, local_forward_dir).normalized();

	const b3Vector3	v_desired_dir = {desired_direction};
	const b3Quat	shortest_arc_quat = b3ShortestArcQuat(current_forward_dir, v_desired_dir);

	b3Scalar	yawZ, pitchY, rollX;
	shortest_arc_quat.getEulerZYX(yawZ, pitchY, rollX);

	//
	V3f		delta_ang_vel = {rollX, pitchY, yawZ};

	nwNON_TWEAKABLE_CONST(float, DELTA_ANGULAR_VELOCITY_MUL, 1);

	delta_ang_vel *= (rb.m_invMass * DELTA_ANGULAR_VELOCITY_MUL);

	const b3Vector3	v_delta_ang_vel = {Vector4_LoadFloat3_Unaligned(delta_ang_vel.a)};
	rb.angular_velocity += v_delta_ang_vel;

	return ALL_OK;
}

ERet SgPhysicsMgr::ChangeAngularVelocityToFaceDirection(
	const RigidBodyHandle& rigid_body_handle
	, const Vector4& desired_direction
	, const float turn_speed
	)
{
	b4RigidBodyData &rb = GetRigidBodyDataByHandle(rigid_body_handle, *this);

	const b3Vector3	local_forward_dir = {V4_Forward};
	const b3Vector3 current_forward_dir = b3QuatRotate(rb.m_quat, local_forward_dir).normalized();

	const b3Vector3	v_desired_dir = {desired_direction};
	const b3Quat	shortest_arc_quat = b3ShortestArcQuat(current_forward_dir, v_desired_dir);

	b3Scalar	yawZ, pitchY, rollX;
	shortest_arc_quat.getEulerZYX(yawZ, pitchY, rollX);

	//
	V3f		delta_ang_vel = {rollX, pitchY, yawZ};

	delta_ang_vel *= (rb.m_invMass * turn_speed);

	const b3Vector3	v_delta_ang_vel = {Vector4_LoadFloat3_Unaligned(delta_ang_vel.a)};
	rb.angular_velocity += v_delta_ang_vel;

	return ALL_OK;
}

ERet SgPhysicsMgr::GetRigidBodyAABB(
	SimdAabb &simd_aabb_
	, const RigidBodyHandle& rigid_body_handle
	) const
{
	SgPhysicsMgrInternalData& mydata = *_data;
	const U32 rigid_body_index = mydata.body_handle_mgr.GetItemIndexByHandle(rigid_body_handle.id);

	const b3Aabb& aabb = mydata.aabbs_world_space[ rigid_body_index ];
	simd_aabb_.mins = aabb.m_minVec.mVec128;
	simd_aabb_.maxs = aabb.m_maxVec.mVec128;
	return ALL_OK;
}

Vector4 SgPhysicsMgr::GetRigidBodyLinearVelocity(
	const RigidBodyHandle& rigid_body_handle
	) const
{
	b4RigidBodyData &rb = GetRigidBodyDataByHandle(rigid_body_handle, *this);
	return rb.linear_velocity.mVec128;
}

ERet SgPhysicsMgr::ApplyCentralImpulse(
									   const RigidBodyHandle& rigid_body_handle
									   , const V3f& impulse
									   )
{
	b4RigidBodyData &rb = GetRigidBodyDataByHandle(rigid_body_handle, *this);

	rb.linear_velocity += b3MakeVector3(
		impulse.x, impulse.y, impulse.z
		)
		* rb.m_invMass
		;

	return ALL_OK;
}

ERet SgPhysicsMgr::ClampLinearVelocityTo(
									   const RigidBodyHandle& rigid_body_handle
									   , const float max_velocity_magnitude
									   )
{
	b4RigidBodyData &rb = GetRigidBodyDataByHandle(rigid_body_handle, *this);

	rb.linear_velocity.mVec128 = XMVector3ClampLength(
		rb.linear_velocity.mVec128
		, 0
		, max_velocity_magnitude
		);

	return ALL_OK;
}

ERet SgPhysicsMgr::SetLinearVelocityDirection(
	const RigidBodyHandle& rigid_body_handle
	, const V3f& desired_direction
	)
{
	b4RigidBodyData &rb = GetRigidBodyDataByHandle(rigid_body_handle, *this);
	const Vector4 v_velocity_mag = XMVector3Length(rb.linear_velocity.mVec128);
	rb.linear_velocity.mVec128 = V4_MUL(
		Vector4_LoadFloat3_Unaligned(desired_direction.a)
		, v_velocity_mag
		);
	return ALL_OK;
}

ERet SgPhysicsMgr::AddAngularVelocity(
	const RigidBodyHandle& rigid_body_handle
	, const V3f& delta_ang_vel_in_world_space
	)
{
	b4RigidBodyData &rb = GetRigidBodyDataByHandle(rigid_body_handle, *this);

	const b3Vector3	v_delta_ang_vel_rel_to_local_frame = b3QuatRotate(
		rb.m_quat,
		b3MakeVector3(
			delta_ang_vel_in_world_space.x,
			delta_ang_vel_in_world_space.y,
			delta_ang_vel_in_world_space.z
		) * rb.m_invMass
	);
	rb.angular_velocity += v_delta_ang_vel_rel_to_local_frame;

	return ALL_OK;
}

ERet SgPhysicsMgr::DampAngularVelocity(
	const RigidBodyHandle& rigid_body_handle
	, const float angular_damping
	)
{
	b4RigidBodyData &rb = GetRigidBodyDataByHandle(rigid_body_handle, *this);
	rb.angular_velocity.x *= angular_damping;
	rb.angular_velocity.y *= angular_damping;
	rb.angular_velocity.z *= angular_damping;
	return ALL_OK;
}

ERet SgPhysicsMgr::DampLinearVelocity(
	const RigidBodyHandle& rigid_body_handle
	, const float linear_damping
	)
{
	b4RigidBodyData &rb = GetRigidBodyDataByHandle(rigid_body_handle, *this);
	rb.linear_velocity.x *= linear_damping;
	rb.linear_velocity.y *= linear_damping;
	rb.linear_velocity.z *= linear_damping;
	return ALL_OK;
}

ERet SgPhysicsMgr::Dbg_ZeroOutVelocity(
								   const RigidBodyHandle& rigid_body_handle
								   )
{
	b4RigidBodyData &rb = GetRigidBodyDataByHandle(rigid_body_handle, *this);
	rb.linear_velocity.mVec128 = g_MM_Zero;
	rb.angular_velocity.mVec128 = g_MM_Zero;
	return ALL_OK;
}

ERet SgPhysicsMgr::Dbg_ResetOrientation(
								   const RigidBodyHandle& rigid_body_handle
								   )
{
	b4RigidBodyData &rb = GetRigidBodyDataByHandle(rigid_body_handle, *this);
	rb.m_quat = Quaternion_Identity();
	return ALL_OK;
}

namespace
{
	void updateAabbWorldSpace(
		SgPhysicsMgrInternalData& mydata
		)
	{
		const U32 num_bodies = mydata.body_handle_mgr._num_alive_items;
		for (int rb_idx = 0; rb_idx < num_bodies; rb_idx++)
		{
			b4RigidBodyData* body = &mydata.rigid_bodies[rb_idx];
			b3Float4 position = body->m_pos;
			b3Quat orientation = body->m_quat;

			//
			const b4ColShape& col_shape = mydata.col_shapes[ body->col_shape_idx.id ];
			const b3Aabb localAabb = col_shape.aabb_local;

			{
				b3Aabb &worldAabb = mydata.aabbs_world_space[rb_idx];
				float margin = 0.f;
				b3TransformAabb2(
					localAabb.m_minVec, localAabb.m_maxVec
					, margin
					, position
					, orientation
					, &worldAabb.m_minVec, &worldAabb.m_maxVec
					);
			}
		}
	}


	//
	class UpdateWorldSpaceAABBsJob: NwNonCopyable
	{
		SgPhysicsMgrInternalData& _mydata;

	public:
		UpdateWorldSpaceAABBsJob(
			SgPhysicsMgrInternalData& mydata
			)
			: _mydata(mydata)
		{
		}

		ERet Run( const NwThreadContext& context, int start, int end )
		{
			for (int rb_idx = start; rb_idx < end; rb_idx++)
			{
				b4RigidBodyData* body = &_mydata.rigid_bodies[ rb_idx ];
				b3Float4 position = body->m_pos;
				b3Quat orientation = body->m_quat;

				//
				const b4ColShape& col_shape = _mydata.col_shapes[ body->col_shape_idx.id ];
				const b3Aabb localAabb = col_shape.aabb_local;

				{
					b3Aabb &worldAabb = _mydata.aabbs_world_space[ rb_idx ];
					float margin = 0.f;
					b3TransformAabb2(
						localAabb.m_minVec, localAabb.m_maxVec
						, margin
						, position
						, orientation
						, &worldAabb.m_minVec, &worldAabb.m_maxVec
						);
				}
			}

			return ALL_OK;
		}
	};


	inline void b4IntegrateTransform(
		__global b4RigidBodyData* body
		, float timeStep
		, float angularDamping
		, b3Float4ConstArg gravityAcceleration
		)
	{
		float BT_GPU_ANGULAR_MOTION_THRESHOLD = (0.25f * 3.14159254f);

		mxASSERT((body->m_invMass != 0.f));

		//angular velocity
		{
			b3Float4 axis;
			//add some hardcoded angular damping
			body->angular_velocity.x *= angularDamping;
			body->angular_velocity.y *= angularDamping;
			body->angular_velocity.z *= angularDamping;

			b3Float4 angvel = body->angular_velocity;
			float fAngle = b3Sqrt(b3Dot3F4(angvel, angvel));
			//limit the angular motion
			if (fAngle * timeStep > BT_GPU_ANGULAR_MOTION_THRESHOLD)
			{
				fAngle = BT_GPU_ANGULAR_MOTION_THRESHOLD / timeStep;
			}
			if (fAngle < 0.001f)
			{
				// use Taylor's expansions of sync function
				axis = angvel * (0.5f * timeStep - (timeStep * timeStep * timeStep) * 0.020833333333f * fAngle * fAngle);
			}
			else
			{
				// sync(fAngle) = sin(c*fAngle)/t
				axis = angvel * (b3Sin(0.5f * fAngle * timeStep) / fAngle);
			}
			b3Quat dorn;
			dorn.x = axis.x;
			dorn.y = axis.y;
			dorn.z = axis.z;
			dorn.w = b3Cos(fAngle * timeStep * 0.5f);
			b3Quat orn0 = body->m_quat;

			b3Quat predictedOrn = b3QuatMul(dorn, orn0);
			predictedOrn = b3QuatNormalized(predictedOrn);
			body->m_quat = predictedOrn;
		}

		//apply gravity
		body->linear_velocity += gravityAcceleration * timeStep;

		//linear velocity
		body->m_pos += body->linear_velocity * timeStep;
	}

	void integrateTransforms(
		SgPhysicsMgrInternalData& mydata
		, float deltaTime
		)
	{
		float angDamping = 0.999f;

		b3Vector3 gravityAcceleration = 
			//b3MakeVector3(0, 0, -0.1f)
			b3MakeVector3(0, 0, 0)
			;

		const U32 num_bodies = mydata.body_handle_mgr._num_alive_items;

		//integrate transforms (external forces/gravity should be moved into constraint solver)
		for (int i = 0; i < num_bodies; i++)
		{
			b4IntegrateTransform(
				&mydata.rigid_bodies[i]
				, deltaTime
				, angDamping
				, gravityAcceleration
				);
		}
	}

	//
	class IntegrateTransformsJob: NwNonCopyable
	{
		SgPhysicsMgrInternalData& _mydata;
		const float	_delta_time;

	public:
		IntegrateTransformsJob(
			SgPhysicsMgrInternalData& mydata
			, const float delta_time
			)
			: _mydata(mydata)
			, _delta_time(delta_time)
		{
		}

		ERet Run( const NwThreadContext& context, int start, int end )
		{
			float angDamping = 0.999f;

			b3Vector3 gravityAcceleration = 
				//b3MakeVector3(0, 0, -0.1f)
				b3MakeVector3(0, 0, 0)
				;

			//integrate transforms (external forces/gravity should be moved into constraint solver)
			for (int i = start; i < end; i++)
			{
				b4IntegrateTransform(
					&_mydata.rigid_bodies[i]
					, _delta_time
					, angDamping
					, gravityAcceleration
					);
			}
			return ALL_OK;
		}
	};

}//namespace


JobID SgPhysicsMgr::UpdateOncePerFrame(
									  const NwTime& game_time
									  , const bool is_game_paused
									  , NwJobSchedulerI& job_sched
									  , const NwHandleTable& ship_handle_mgr
									  )
{
	if(is_game_paused)
	{
		return NIL_JOB_ID;
	}

	const JobID update_physics_job_group = job_sched.beginGroup();

//
	AllocatorI & scratch_allocator = MemoryHeaps::temporary();

	//
	SgPhysicsMgrInternalData& mydata = *_data;

	const U32 num_bodies = mydata.body_handle_mgr._num_alive_items;

	//
	JobID	h_job_update_aabbs;
	nwCREATE_JOB(h_job_update_aabbs
		, job_sched
		, -1, num_bodies
		, JobPriority_Critical
		, UpdateWorldSpaceAABBsJob
		, mydata
		);

	//
	mxOPTIMIZE("chokepoint");
	job_sched.waitFor(h_job_update_aabbs);

	//
	{
		const SimdAabb* simd_aabbs = (SimdAabb*) mydata.aabbs_world_space;

		struct FunScope
		{
			static void FillObjIDs(CulledObjectID *	culled_obj_ids_
				, const U32 num_objs
				, void* user_data0
				, void* user_data1)
			{
				SgPhysicsMgrInternalData& mydata = *(SgPhysicsMgrInternalData*) user_data0;
				const NwHandleTable& ship_handle_mgr = *(NwHandleTable*) user_data1;

				const U32 num_bodies = mydata.body_handle_mgr._num_alive_items;

				for(U32 rb_idx = 0; rb_idx < num_bodies; rb_idx++)
				{
					const b4RigidBodyData& rb = mydata.rigid_bodies[rb_idx];

#if MX_DEBUG || MX_DEVELOPER
					const U32 ship_index = ship_handle_mgr.GetItemIndexByHandle(rb.user_obj_index);
					mxASSERT2(ship_index == rb_idx, "ship and rigid body handles are synced with each other");
#endif

					culled_obj_ids_[ rb_idx ] = (rb.user_obj_type << CULLED_OBJ_TYPE_SHIFT) | rb_idx;
				}
			}
		};

		_aabb_tree->Update(
			TSpan< const SimdAabb >( simd_aabbs, num_bodies )
			, &FunScope::FillObjIDs
			, &mydata
			, (void*) &ship_handle_mgr
			, scratch_allocator
			);
	}

	//computeOverlappingPairs();

	//computeContactPoints();

	////solve contacts

	//update transforms
	JobID	h_job_integrate_transforms;
	nwCREATE_JOB(h_job_integrate_transforms
		, job_sched
		, -1, num_bodies
		, JobPriority_Critical
		, IntegrateTransformsJob
		, mydata
		, game_time.real.delta_seconds
		);

	//
	job_sched.endGroup();

	return update_physics_job_group;
}

void SgPhysicsMgr::DebugDraw()
{

}

ERet SgPhysicsMgr::GatherObjectsInFrustum(
	const ViewFrustum& view_frustum
	, CulledObjectsIDs &culled_objects_ids_
	, const NwHandleTable& ship_handle_mgr
	) const
{
//TODO: in contrast to BVH, this can be multithreaded
//	const SimdFrustum	simd_frustum( view_frustum );

	//
	SgPhysicsMgrInternalData& mydata = *_data;

	const U32 num_bodies = mydata.body_handle_mgr._num_alive_items;

	//
	mxDO(culled_objects_ids_.reserve(num_bodies));

	//
	const SimdAabb* aabbs = (SimdAabb*) mydata.aabbs_world_space;

	for(UINT item_index = 0; item_index < num_bodies; item_index++)
	{
		const SimdAabb aabb = aabbs[ item_index ];


		const AABBf node_aabb_f = SimdAabb_Get_AABBf(aabb);

		//
#if USE_SSE_CULLING
		const SpatialRelation::Enum result = Test_AABB_against_ViewFrustum(
			(SimdAabb&)node
			, simd_frustum
			);
#else
		const SpatialRelation::Enum result = view_frustum.Classify(node_aabb_f);
#endif


#if 0	// DEBUG

		TbPrimitiveBatcher& prim_batcher = SgGameApp::Get().renderer._render_system->_debug_renderer;

		if(result == SpatialRelation::Outside) {
			prim_batcher.DrawAABB( node_aabb_f, RGBAf::WHITE );
		} else if(result == SpatialRelation::Inside) {
			prim_batcher.DrawAABB( node_aabb_f, RGBAf::GREEN );
		} else {
			prim_batcher.DrawAABB( node_aabb_f, RGBAf::RED );
		}
#endif

		if(result != SpatialRelation::Outside)
		{
			const b4RigidBodyData& rb = mydata.rigid_bodies[ item_index ];

			const U32 ship_index = ship_handle_mgr.GetItemIndexByHandle(rb.user_obj_index);
			mxASSERT(ship_index == item_index);

			culled_objects_ids_.add(
				(rb.user_obj_type << CULLED_OBJ_TYPE_SHIFT) | ship_index
				);
		}

	}//for

	return ALL_OK;
}
