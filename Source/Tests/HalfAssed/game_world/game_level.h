#pragma once


#include "npc/game_nps.h"	// MonsterType8
#include "gameplay_constants.h"	// PickableItem8


enum EGameMode
{
	GM_REACH_EXIT,
	GM_SURVIVE_WAVES,
};
mxDECLARE_ENUM( EGameMode, U8, GameMode8 );

enum EAttackWave
{
	Wave0 = BIT(0),
	Wave1 = BIT(1),
	Wave2 = BIT(2),
	Wave3 = BIT(3),
	Wave4 = BIT(4),

	WaveAll = ~0,
};
mxDECLARE_FLAGS( EAttackWave, U32, AttackWavesT );


struct MyLevelData: CStruct
{

public:
	//struct Sky: CStruct
	//{
	//public:
	//	mxDECLARE_CLASS(Sky, CStruct);
	//	mxDECLARE_REFLECTION;
	//	Sky();
	//};
	//Sky	sky;

	GameMode8	game_mode;

	U8			num_attack_waves;

	//NwAudioClip
	AssetID	background_music;

public:

	//
	struct ItemSpawnPoint: CStruct
	{
		V3f				pos;
		PickableItem8	type;
		AttackWavesT	waves;
	public:
		mxDECLARE_CLASS(ItemSpawnPoint, CStruct);
		mxDECLARE_REFLECTION;
		ItemSpawnPoint();
	};
	DynamicArray<ItemSpawnPoint>	item_spawn_points;

	//
	struct NpcSpawnPoint: CStruct
	{
		V3f				pos;
		MonsterType8	type;
		U8				count;	// >= 1
		AttackWavesT	waves;
		String32		comment;
	public:
		mxDECLARE_CLASS(NpcSpawnPoint, CStruct);
		mxDECLARE_REFLECTION;
		NpcSpawnPoint();
	};
	DynamicArray<NpcSpawnPoint>	npc_spawn_points;

	//
	V3f	player_spawn_pos;

	//
	V3f	mission_exit_pos;

	/// empty if the last level in the game
	AssetID	next_level;

public:
	mxDECLARE_CLASS(MyLevelData, CStruct);
	mxDECLARE_REFLECTION;
	MyLevelData(AllocatorI& allocator = MemoryHeaps::process());
};

namespace MyLevelData_
{

	V3f GetPosForSpawnPoint();

	ERet CreateMissionExitIfNeeded(const V3f& mission_exit_pos);

}//namespace MyLevelData_
