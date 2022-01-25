//
#pragma once

#include "common.h"



struct PhysConfig
{
	int	max_bodies;
	int max_col_shapes;

public:
	PhysConfig()
		: max_bodies(1024)
		, max_col_shapes(32)
	{
	}
};

mxDECLARE_32BIT_HANDLE(ColShapeHandle);

mxDECLARE_32BIT_HANDLE(RigidBodyHandle);

///
typedef U8 RigidBodyUserType;
typedef U32 RigidBodyUserIndex;


struct SgRBInstanceData
{
	Vector4	pos;
	Vector4	quat;
	Vector4	linear_velocity;
	Vector4	angular_velocity;
};

//struct SgRBVelocityData
//{
//	Vector4	linear_velocity;
//	Vector4	angular_velocity;
//};



template< typename T >
struct TStridedData
{
	const T *	start;
	U32			stride;
	U32			count;

public:
	template< class ARRAY >
	static TStridedData FromArray(const ARRAY& a)
	{
		TStridedData	result;
		result.start = reinterpret_cast< const T* >(a.raw());
		result.stride = sizeof(a[0]);
		result.count = a.num();
		return result;
	}

	template< class Y >
	static TStridedData FromArray(const Y* items, const U32 count)
	{
		mxSTATIC_ASSERT(sizeof(items[0]) >= sizeof(T*));

		TStridedData	result;
		result.start = (T*) items;
		result.stride = sizeof(items[0]);
		result.count = count;
		return result;
	}

	mxFORCEINLINE const T& GetAt(UINT index) const
	{
		return *(T*) ( (char*)start + (index * stride) );
	}
};

typedef TStridedData<SgRBInstanceData> StridedRBInstanceData;


struct SgPhysicsMgrInternalData;


class SgPhysicsMgr: TSingleInstance<SgPhysicsMgr>
{
public:
	SgAABBTree *	_aabb_tree;

	SgPhysicsMgrInternalData *	_data;

public:
	SgPhysicsMgr();
	~SgPhysicsMgr();

	ERet Initialize(
		const PhysConfig& phys_config
		, const SgAppSettings& game_settings
		, NwClump* scene_clump
		, AllocatorI& allocator
		);

	void Shutdown();

	void RemoveAllRigidBodies();


	JobID UpdateOncePerFrame(
		const NwTime& game_time
		, const bool is_game_paused
		, NwJobSchedulerI& job_sched
		, const NwHandleTable& ship_handle_mgr
		);

	void DebugDraw();

	// for debugging!

	ERet GatherObjectsInFrustum(
		const ViewFrustum& frustum
		, DynamicArray<U32> &culled_objects_ids_
		, const NwHandleTable& ship_handle_mgr
		) const;

public:
	const StridedRBInstanceData GetRBInstancesData() const;

	mxDEPRECATED
	ERet GetRigidBodyCenterOfMassPosition(
		Vector4 *pos_
		, const RigidBodyHandle& rigid_body_handle
		) const;


	const Vector4 GetRigidBodyCenterOfMassPosition(
		const RigidBodyHandle& rigid_body_handle
	) const;

	const V3f GetRigidBodyPredictedPosition(
		const RigidBodyHandle& rigid_body_handle
		, const NwTime& game_time
	) const;

	const Vector4 GetRigidBodyForwardDirection(
		const RigidBodyHandle& rigid_body_handle
	) const;

	ERet GetRigidBodyPosAndQuat(
		const RigidBodyHandle& rigid_body_handle
		, SgRBInstanceData &instance_data_
		) const;

	ERet GetRigidBodyTransform(
		M44f &transform_
		, const RigidBodyHandle& rigid_body_handle
		) const;

	ERet SetRigidBodyCenterOfMassPosition(
		const V3f& com_pos
		, const RigidBodyHandle& rigid_body_handle
		) const;

	ERet SetRigidBodyFacingDirection(
		const RigidBodyHandle& rigid_body_handle
		// must be normalized:
		, const Vector4& desired_direction
		);

	ERet ChangeAngularVelocityToFaceDirection(
		const RigidBodyHandle& rigid_body_handle
		// must be normalized:
		, const Vector4& desired_direction
		, const float turn_speed
		);

	ERet GetRigidBodyAABB(
		SimdAabb &simd_aabb_
		, const RigidBodyHandle& rigid_body_handle
		) const;

	Vector4 GetRigidBodyLinearVelocity(
		const RigidBodyHandle& rigid_body_handle
		) const;

	//
	ERet ApplyCentralImpulse(
		const RigidBodyHandle& rigid_body_handle
		, const V3f& impulse
		);

	ERet ClampLinearVelocityTo(
		const RigidBodyHandle& rigid_body_handle
		, const float max_velocity_magnitude
		);
	ERet SetLinearVelocityDirection(
		const RigidBodyHandle& rigid_body_handle
		, const V3f& desired_direction
		);


	ERet AddAngularVelocity(
		const RigidBodyHandle& rigid_body_handle
		, const V3f& delta_ang_vel
		);


	ERet DampAngularVelocity(
		const RigidBodyHandle& rigid_body_handle
		, const float angular_damping
		);

	ERet DampLinearVelocity(
		const RigidBodyHandle& rigid_body_handle
		, const float linear_damping
		);


	/// for debugging
	ERet Dbg_ZeroOutVelocity(
		const RigidBodyHandle& rigid_body_handle
		);
	ERet Dbg_ResetOrientation(
		const RigidBodyHandle& rigid_body_handle
		);

public:
	ERet CreateColShape_Sphere(
		ColShapeHandle &col_shape_handle_
		, float radius
		);

	ERet CreateColShape_Box(
		ColShapeHandle &col_shape_handle_
		, const AABBf& aabb_local
		);

	ERet CreateRigidBody(
		RigidBodyHandle &rigid_body_handle_
		, const ColShapeHandle col_shape_handle
		, const RigidBodyUserType user_obj_type
		, const RigidBodyUserIndex user_obj_index
		, const float mass
		, const V3f& pos
		);

	ERet DeleteRigidBody(
		const RigidBodyHandle rigid_body_handle
		);

public:

};
