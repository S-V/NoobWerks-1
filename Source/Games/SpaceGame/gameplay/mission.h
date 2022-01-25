#pragma once

#include <Utility/Camera/NwFlyingCamera.h>
#include <Developer/ModelEditor/ModelEditor.h>	// NwLocalTransform
#include "common.h"
#include "physics/physics_mgr.h"


enum EMissionType
{
	mission_destroy_structure = 0,
};


struct SgMissionDef: NwSharedResource
{
	EMissionType	type;

public:
	mxDECLARE_CLASS(SgMissionDef, CStruct);
	mxDECLARE_REFLECTION;
	SgMissionDef();
};
