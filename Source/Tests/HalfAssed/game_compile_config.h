#pragma once


#define GAME_CFG_RELEASE_BUILD	(0)

#define GAME_CFG_WITH_SOUND		(0)
#define GAME_CFG_WITH_PHYSICS	(1)
#define GAME_CFG_WITH_SCRIPTING	(1)


#if GAME_CFG_WITH_PHYSICS
#pragma comment( lib, "Physics.lib" )
#endif


#if GAME_CFG_WITH_SCRIPTING
#pragma comment( lib, "Scripting.lib" )
#endif
