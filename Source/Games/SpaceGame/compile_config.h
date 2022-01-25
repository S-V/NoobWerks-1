#pragma once


#define GAME_CFG_RELEASE_BUILD	(1)

#define GAME_CFG_WITH_SOUND		(1)
#define GAME_CFG_WITH_PHYSICS	(0)
#define GAME_CFG_WITH_SCRIPTING	(0)
#define GAME_CFG_WITH_VOXELS	(0)

#if GAME_CFG_WITH_PHYSICS
#pragma comment( lib, "Physics.lib" )
#endif


#if GAME_CFG_WITH_SCRIPTING
#pragma comment( lib, "Scripting.lib" )
#endif


#define MY_GAME_NAME	"SpaceGame"
