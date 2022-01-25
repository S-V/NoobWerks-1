#pragma once


#define TEST_LOAD_RED_FACTION_MODEL	(0)




#include <Renderer/Modules/Animation/SkinnedMesh.h>
#include <Renderer/Modules/idTech4/idRenderModel.h>



#include "game_forward_declarations.h"





#if TEST_LOAD_RED_FACTION_MODEL

#include <Renderer/Modules/Animation/SkinnedMesh.h>
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
