#include <Base/Base.h>
#pragma hdrstop

#include <Physics/Bullet_Wrapper.h>
#include <Physics/PhysicsWorld.h>
#include <Physics/RigidBody.h>
#include <Physics/Bullet/DynamicCharacterController.h>


namespace
{
	const U32 BULLET_OBJECT_ALIGNMENT = 16;

	// SVD: reduced DEFAULT_STEP_HEIGHT to 0.01 to reduce jitter
	//const float DEFAULT_STEP_HEIGHT = 0.05f; // The default amount to move the character up before resolving collisions.
	const float DEFAULT_STEP_HEIGHT = 0.01f;

	const btVector3 UP_VECTOR(0.0f, 0.0f, 1.0f);	// In our coord system, Z is UP.
	const btVector3 ZERO_VECTOR(0.0f, 0.0f, 0.0f);

	// Ray/convex result callbacks that ignore the self object passed to them as well as ghost objects.

	class RayResultCallback : public btCollisionWorld::ClosestRayResultCallback
	{
	public:
		RayResultCallback(btCollisionObject* self)
		: btCollisionWorld::ClosestRayResultCallback(ZERO_VECTOR, ZERO_VECTOR)
		, mSelf(self)
		{
		}

		btScalar addSingleResult(btCollisionWorld::LocalRayResult& rayResult, bool normalInWorldSpace)
		{
			if (rayResult.m_collisionObject == mSelf) return 1.0f;
			if (rayResult.m_collisionObject->getInternalType() == btCollisionObject::CO_GHOST_OBJECT) return 1.0f;
			return ClosestRayResultCallback::addSingleResult(rayResult, normalInWorldSpace);
		}

	private:
		btCollisionObject* mSelf;
	};

	class ConvexResultCallback : public btCollisionWorld::ClosestConvexResultCallback
	{
	public:
		ConvexResultCallback(btCollisionObject* self, const btVector3& up, btScalar minSlopeDot)
		: btCollisionWorld::ClosestConvexResultCallback(ZERO_VECTOR, ZERO_VECTOR)
		, mSelf(self)
		, mUp(up)
		, mMinSlopeDot(minSlopeDot)
		{
		}

		btScalar addSingleResult(btCollisionWorld::LocalConvexResult& convexResult, bool normalInWorldSpace)
		{
			if (convexResult.m_hitCollisionObject == mSelf) return 1.0f;
			if (convexResult.m_hitCollisionObject->getInternalType() == btCollisionObject::CO_GHOST_OBJECT) return 1.0f;

			btVector3 hitNormalWorld;
			if (normalInWorldSpace)
			{
				hitNormalWorld = convexResult.m_hitNormalLocal;
			}
			else
			{
				hitNormalWorld = convexResult.m_hitCollisionObject->getWorldTransform().getBasis() * convexResult.m_hitNormalLocal;
			}
#if 0
			btScalar dotUp = mUp.dot(hitNormalWorld);
			if (dotUp < mMinSlopeDot)
			{
				return 1.0f;
			}
#endif
			return ClosestConvexResultCallback::addSingleResult(convexResult, normalInWorldSpace);
		}

	private:
		btCollisionObject* mSelf;
		btVector3 mUp;
		btScalar mMinSlopeDot;
	};
}

DynamicCharacterController::DynamicCharacterController()
	: mPhysicsWorld(nullptr)
	, mCollisionWorld(nullptr)
	, mRigidBody(nullptr)
	, mShape(nullptr)
	, mOnGround(false)
	, mOnSteepSlope(false)
	, mStepHeight(DEFAULT_STEP_HEIGHT)
	, mMaxClimbSlopeAngle(0.6f)
	, mIsStepping(false)
{
	mSlopeNormal.setZero();

	mCharacterMovementX = 0.0f;
	mCharacterMovementY = 0.0f;
}

DynamicCharacterController::~DynamicCharacterController()
{
	mxASSERT(!mRigidBody && !mShape);
}

void DynamicCharacterController::create(
										btMotionState* motionState
										, NwPhysicsWorld* physicsWorld
										, const Settings& settings
										, void* rigidBodyUserPointer
										)
{
	mxASSERT(!mRigidBody && !mShape && !mPhysicsWorld);

	mPhysicsWorld = physicsWorld;
	mCollisionWorld = &mPhysicsWorld->m_dynamicsWorld;

	// From btCapsuleShape.h:
	// "The total height is height+2*radius, so the height is just the height between the center of each 'sphere' of the capsule caps."
	float capsuleHeight = settings.height - 2.0f * settings.radius;

	mShape = new btCapsuleShape(settings.radius, capsuleHeight);

	btVector3 inertia(0.0f, 0.0f, 0.0f);
	mShape->calculateLocalInertia(settings.mass, inertia);

	if(!motionState) {
		motionState = &_motion_state;
	}

	//
	btRigidBody::btRigidBodyConstructionInfo ci(
		settings.mass
		, motionState
		, mShape
		, inertia
		);

	// SVD: make linear damping very small so that we can move by inertia while jumping
#if 0
	ci.m_linearDamping = 0.99f;
	ci.m_friction = 0;
#else
	ci.m_linearDamping = 0.1f;
	ci.m_friction = 1.0f;
#endif

	mRigidBody = new btRigidBody(ci);

	mRigidBody->setActivationState(DISABLE_DEACTIVATION);
	mRigidBody->setUserPointer(rigidBodyUserPointer);

	mRigidBody->setAngularFactor(ZERO_VECTOR); // No rotation allowed. We only rotate the visuals.

	//
	mRigidBody->setUserIndex(PHYS_OBJ_PLAYER_CHARACTER);

	mPhysicsWorld->m_dynamicsWorld.addRigidBody(mRigidBody);
}

void DynamicCharacterController::destroy()
{
	mxASSERT(mRigidBody && mShape && mPhysicsWorld);

	mPhysicsWorld->m_dynamicsWorld.removeRigidBody(mRigidBody);

	delete(mRigidBody);
	mRigidBody = nullptr;

	delete(mShape);
	mShape = nullptr;

	mPhysicsWorld = nullptr;
	mCollisionWorld = nullptr;
}

void DynamicCharacterController::setParameters(float maxClimbSlopeAngle)
{
	mMaxClimbSlopeAngle = maxClimbSlopeAngle;
}

void DynamicCharacterController::preStep(SecondsF delta_time)
{
	mIsStepping = true;

	if (mOnSteepSlope)
	{
//		DBGOUT("mOnSteepSlope!");
		btVector3 uAxis = mSlopeNormal.cross(UP_VECTOR).normalize();
		btVector3 vAxis = uAxis.cross(mSlopeNormal);
		btVector3 fixVel = vAxis / mSlopeNormal.dot(UP_VECTOR);
		//mRigidBody->setLinearVelocity(mRigidBody->getLinearVelocity() - fixVel);
	}

	// Move character upwards.
	moveCharacterAlongVerticalAxis(mStepHeight);

	// Set the linear velocity based on the movement vector.
	// Don't adjust the vertical component, instead let Bullet handle that.
	if (mOnGround)
	{
#if 0 // SVD-
		btVector3 linVel = mRigidBody->getLinearVelocity();
		linVel.setX(mCharacterMovementX);
		linVel.setY(mCharacterMovementY);
		mRigidBody->setLinearVelocity(linVel);
#else
		btVector3	movement_vec(mCharacterMovementX, mCharacterMovementY, 0);
		mRigidBody->applyCentralImpulse(movement_vec);
		mCharacterMovementX = 0;
		mCharacterMovementY = 0;

		// prevent 'sliding' when on the ground, but allow movement by inertia when jumping
		btVector3	linear_velocity = mRigidBody->getLinearVelocity();
		float linear_damping = 0.9f;
		linear_velocity *= btPow(btScalar(1)-linear_damping, delta_time);
		mRigidBody->setLinearVelocity(linear_velocity);
#endif
	}
}

// If defined, a ray test is used to detect objects below the character, otherwise a convex test.
//#define USE_RAY_TEST

void DynamicCharacterController::postStep()
{
	const float TEST_DISTANCE = 4.0f;

#ifdef USE_RAY_TEST
	btVector3 from = mRigidBody->getWorldTransform().getOrigin();
	btVector3 to = from - btVector3(0, 0, TEST_DISTANCE);	// In our coord system, Z is UP.

	// Detect ground collision and update the "on ground" status.
	RayResultCallback callback(mRigidBody);
	mCollisionWorld->rayTest(from, to, callback);

	// Check if there is something below the character.
	if (callback.hasHit())
	{
		btVector3 end = from + (to - from) * callback.m_closestHitFraction;
		btVector3 normal = callback.m_hitNormalWorld;

		// Slope test.
		btScalar slopeDot = normal.dot(UP_VECTOR);
		mOnSteepSlope = (slopeDot < mMaxClimbSlopeAngle);
		mSlopeNormal = normal;

		// compute the distance to the floor
		float distance = btDistance(end, from);

		//SVD: changed condition to (distance < 1.5):
		//mOnGround = (distance < 1.0 && !mOnSteepSlope);
		mOnGround = (distance < 1.5 && !mOnSteepSlope);

		// Move down.
		if (distance < mStepHeight)
		{
			moveCharacterAlongVerticalAxis(-distance * 0.99999f);
		}
		else
		{
			moveCharacterAlongVerticalAxis(-mStepHeight * 0.9999f);
		}
	}
	else
	{
		// In the air.
		mOnGround = false;
		moveCharacterAlongVerticalAxis(-mStepHeight);
	}
#else
	btTransform tsFrom, tsTo;
	btVector3 from = mRigidBody->getWorldTransform().getOrigin();
	btVector3 to = from - btVector3(0, 0, TEST_DISTANCE);	// In our coord system, Z is UP.

	tsFrom.setIdentity();
	tsFrom.setOrigin(from);

	tsTo.setIdentity();
	tsTo.setOrigin(to);

	ConvexResultCallback callback(mRigidBody, UP_VECTOR, 0.01f);
	callback.m_collisionFilterGroup = mRigidBody->getBroadphaseHandle()->m_collisionFilterGroup;
	callback.m_collisionFilterMask = mRigidBody->getBroadphaseHandle()->m_collisionFilterMask;

	mCollisionWorld->convexSweepTest(
		(btConvexShape*)mShape
		, tsFrom, tsTo
		, callback
		, mCollisionWorld->getDispatchInfo().m_allowedCcdPenetration
		);

	if (callback.hasHit())
	{
		btVector3 end = from + (to - from) * callback.m_closestHitFraction;
		btVector3 normal = callback.m_hitNormalWorld;

		// Slope test.
		btScalar slopeDot = normal.dot(UP_VECTOR);
		mOnSteepSlope = (slopeDot < mMaxClimbSlopeAngle);
		mSlopeNormal = normal;

		// Compute distance to the floor.
		float distance = btDistance(end, from);
		mOnGround = (distance < 0.1f && !mOnSteepSlope);

		// Move down.
		if (distance < mStepHeight)
		{
			moveCharacterAlongVerticalAxis(-distance * 0.99999f);
		}
		else
		{
			moveCharacterAlongVerticalAxis(-mStepHeight * 0.9999f);
		}
	}
	else
	{
		mOnGround = false;
		moveCharacterAlongVerticalAxis(-mStepHeight);
	}
#endif

	mIsStepping = false;
}

bool DynamicCharacterController::onGround() const
{
	return mOnGround;
}

bool DynamicCharacterController::jump(const btVector3& direction, float force)
{
	if (mOnGround)
	{
		mRigidBody->applyCentralImpulse(direction * force);
		return true;
	}
	return false;
}

void DynamicCharacterController::setMovementXZ(const V2f& movementVector)
{
	mCharacterMovementX = movementVector.x;
	mCharacterMovementY = movementVector.y;
}

void DynamicCharacterController::setLinearVelocity(const btVector3& vel)
{
	DBGOUT("setLinearVelocity()");
	mRigidBody->setLinearVelocity(vel);
}

const btVector3& DynamicCharacterController::getLinearVelocity() const
{
	return mRigidBody->getLinearVelocity();
}

void DynamicCharacterController::moveCharacterAlongVerticalAxis(float step)
{
	btVector3 pos = mRigidBody->getWorldTransform().getOrigin();
	mRigidBody->getWorldTransform().setOrigin(pos + btVector3(0, 0, step));
}

void DynamicCharacterController::setCollisionsEnabled( bool enable_collisions )
{
	if( enable_collisions ) {
		mRigidBody->setCollisionFlags(mRigidBody->getCollisionFlags() & ~btCollisionObject::CF_NO_CONTACT_RESPONSE);
	} else {
		mRigidBody->setCollisionFlags(mRigidBody->getCollisionFlags() | btCollisionObject::CF_NO_CONTACT_RESPONSE);
	}
}

void DynamicCharacterController::setGravityEnabled( bool enable_gravity )
{
	mRigidBody->setGravity( enable_gravity
		? mPhysicsWorld->m_dynamicsWorld.getGravity()
		: toBulletVec(CV3f(0))
		);
}
