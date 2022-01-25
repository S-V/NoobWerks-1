#pragma once

#include <Physics/Bullet_Wrapper.h>


///
mxALIGN_BY_CACHELINE struct NwCollisionObject: CStruct
{
	btCollisionObject	bt;

	// 32: 320

public:
	enum { ALLOC_GRANULARITY = 256 };
	mxDECLARE_CLASS( NwCollisionObject, CStruct );
	mxDECLARE_REFLECTION;

	NwCollisionObject();

	mxFORCEINLINE btCollisionObject & bt_colobj()
	{ return bt; }

	mxFORCEINLINE const btCollisionObject& bt_colobj() const
	{ return bt; }

public:
	//const AABBf GetAabbWorld() const;
};
