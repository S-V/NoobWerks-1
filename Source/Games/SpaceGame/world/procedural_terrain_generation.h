/*
===============================================================================
=	PROCEDURAL TERRAIN GENERATION
===============================================================================
*/
#pragma once

//
#include <ProcGen/FastNoiseSupport.h>
#pragma comment( lib, "ProcGen.lib" )


#include <Rendering/Public/Globals.h>	// LargePosition, NwCameraView
#include <ProcGen/Noise/NwNoiseFunctions.h>
#include <Planets/PlanetsCommon.h>
#include <Planets/Noise.h>	// HybridMultiFractal
#include <VoxelsSupport/VoxelTerrainRenderer.h>

#include <Voxels/public/vx5.h>
#include <voxels/private/scene/storage/vxp_world_storage_interface.h>	// ChunkDatabase_LMDB

#include "common.h"
#include "experimental/game_experimental.h"


//struct ProcGenWorldStats
//{
//	int		num_noise_octaves_per_LoD[ VX5::ChunkID::NUM_LODs ];
//
//public:
//	ProcGenWorldStats()
//	{
//		mxZERO_OUT(*this);
//	}
//};
//
//class ChunkNoiseCache: NwNonCopyable
//{
//public:
//
//	struct ChunkNoise
//	{
//		//
//	};
//
//public:
//	ChunkNoiseCache()
//	{
//	}
//};
