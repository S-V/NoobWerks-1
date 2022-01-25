#include <Base/Base.h>
#pragma hdrstop

#include <Physics/CollisionObject.h>

#include <Physics/Collision/VoxelTerrainCollisions.h>
#include <Physics/Collision/TbQuantizedBIH.h>
#include <Physics/Bullet_Wrapper.h>


/*
----------------------------------------------------------
	NwCollisionObject
----------------------------------------------------------
*/
mxDEFINE_CLASS(NwCollisionObject);
mxBEGIN_REFLECTION(NwCollisionObject)
	//mxMEMBER_FIELD( camera ),
	//mxMEMBER_FIELD( position_in_world_space ),
mxEND_REFLECTION;

//tbPRINT_SIZE_OF(NwCollisionObject);
//tbPRINT_SIZE_OF(btRigidBody);

NwCollisionObject::NwCollisionObject()
{
}

//const AABBf NwCollisionObject::GetAabbWorld() const
//{
//	btVector3	aabb_min;
//	btVector3	aabb_max;
//	this->bt_colobj().getAabb( aabb_min, aabb_max );
//
//	return AABBf::make(
//		fromBulletVec(aabb_min)
//		, fromBulletVec(aabb_max)
//		);
//}
