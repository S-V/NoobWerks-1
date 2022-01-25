#pragma once


#define TEST_LOAD_RED_FACTION_MODEL	(0)




#include <Rendering/Private/Modules/Animation/SkinnedMesh.h>
#include <Rendering/Private/Modules/idTech4/idRenderModel.h>



#include "common.h"





#if TEST_LOAD_RED_FACTION_MODEL

#include <Rendering/Private/Modules/Animation/SkinnedMesh.h>
// TestSkinnedModel
#include <Developer/ThirdPartyGames/RedFaction/RedFaction_Experimental.h>
#include <Developer/ThirdPartyGames/RedFaction/RedFaction_AnimationLoader.h>


ERet loadTestV3C(
				 TestSkinnedModel &RF_test_model_
				 , NwClump * storage_clump
				 , AllocatorI & scratch
				 );

ERet loadTestRFA(
				 RF1::Anim &RF_test_anim_
				 , const TestSkinnedModel& RF_test_model
				 );

#endif // TEST_LOAD_RED_FACTION_MODEL
