#include <Base/Base.h>
#pragma hdrstop

#include <Physics/RigidBody.h>

//tbPRINT_SIZE_OF(btSphereShape);

/*
----------------------------------------------------------
	NwRigidBody
----------------------------------------------------------
*/
mxDEFINE_CLASS(NwRigidBody);
mxBEGIN_REFLECTION(NwRigidBody)
	//mxMEMBER_FIELD( camera ),
	//mxMEMBER_FIELD( position_in_world_space ),
mxEND_REFLECTION;

//tbPRINT_SIZE_OF(NwRigidBody);
//tbPRINT_SIZE_OF(btRigidBody);

NwRigidBody::NwRigidBody()
{
	rocket_did_collide = false;
}

const AABBf NwRigidBody::GetAabbWorld() const
{
	btVector3	aabb_min;
	btVector3	aabb_max;
	this->bt_rb().getAabb( aabb_min, aabb_max );

	return AABBf::make(
		fromBulletVec(aabb_min)
		, fromBulletVec(aabb_max)
		);
}

const V3f NwRigidBody::GetCenterWorld() const
{
	return fromBulletVec(this->bt_rb().getCenterOfMassPosition());
}

//tbPRINT_SIZE_OF(btCylinderShapeZ);

namespace btRigidBody_
{
	void DisableGravity(btRigidBody& bt_rb)
	{
		const int old_flags = bt_rb.getFlags();

		bt_rb.setFlags(
			old_flags | BT_DISABLE_WORLD_GRAVITY
			);
		bt_rb.setGravity(btVector3(0,0,0));
	}
}//namespace btRigidBody_
