#pragma once

#include <Physics/Bullet_Wrapper.h>

class btCapsuleShape;
class NwPhysicsWorld;

// An implementation of a "3 step" character controller for Bullet, based on ideas described here: http://dev-room.blogspot.fi/2015/03/some-example-works-like.html

// The idea is that the rigid body goes through 3 steps every update:
// 1. Move the character up a certain amount.
// 2. Move the character based on input and let Bullet resolve collisions normally (ie. step the simulation).
// 3. Do a ray or convex test downwards to find any objects that the character can stand on (ie. the ground) and move the character back down.

class DynamicCharacterController
{
public:


	enum State
	{
		ON_GROUND,
		CROUCHING,	// ducking
		CLIMBING,
		JUMPING,
		FLYING,
	};

	State	state;

	//is_sprinting;


public_internal:
	btDefaultMotionState	_motion_state;

	NwPhysicsWorld* mPhysicsWorld;
	btCollisionWorld* mCollisionWorld;
	btRigidBody* mRigidBody;
	btCapsuleShape* mShape;
	bool mOnGround;
	bool mOnSteepSlope;
	btVector3 mSlopeNormal;
	float mStepHeight;
	float mMaxClimbSlopeAngle;
	bool mIsStepping;

	// horizontal movement
	float mCharacterMovementX;
	float mCharacterMovementY;

public:
	DynamicCharacterController();
	~DynamicCharacterController();
	
	//
	struct Settings
	{
		float	mass;

		float	radius;
		float	height;	// total height

		///// The maximum slope the character can climb
		//float	max_slope_angle_deg;

		//float	max_jump_height;
	};

	void create(
		btMotionState* motionState
		, NwPhysicsWorld* physicsWorld
		, const Settings& settings
		, void* rigidBodyUserPointer
		);
	void destroy();

	void setParameters(float maxClimbSlopeAngle);

public:	// Update

	/// Call before the physics are stepped (btDynamicsWorld::stepSimulation).
	void preStep(
		SecondsF delta_time
		);

	/// Call after the physics are stepped.
	void postStep();

public:	// Handling Input



public:
	bool onGround() const;

	bool CanClimb() const
	{
		return mOnSteepSlope;
	}

	bool jump(const btVector3& direction, float force);

	void setMovementXZ(const V2f& movementVector);

	void setLinearVelocity(const btVector3& vel);
	const btVector3& getLinearVelocity() const;

	btRigidBody* getRigidBody() const { return mRigidBody; }
	btCollisionShape* getCollisionShape() const { return mShape; }

	const btVector3 getRigidBodyCM() const
	{
		return mRigidBody->getCenterOfMassPosition();
	}

	const V3f& getPos() const
	{
		return fromBulletVec(this->getRigidBodyCM());
	}

	void setPos(const V3f& pos)
	{
		btTransform	t;
		t.setIdentity();
		t.setOrigin(toBulletVec(pos));
		mRigidBody->getMotionState()->setWorldTransform(t);
		mRigidBody->setCenterOfMassTransform(t);
	}

	void setCollisionsEnabled( bool enable_collisions );

	/// Walking Player vs Flying Spectator
	void setGravityEnabled( bool enable_gravity );

private:
	void moveCharacterAlongVerticalAxis(float step);
};
