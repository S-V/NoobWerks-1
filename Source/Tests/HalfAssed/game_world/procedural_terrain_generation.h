/*
===============================================================================
=	PROCEDURAL TERRAIN GENERATION
===============================================================================
*/
#pragma once

//
#include <ProcGen/FastNoiseSupport.h>
#pragma comment( lib, "ProcGen.lib" )


#include <Renderer/Renderer.h>	// LargePosition, NwCameraView
#include <ProcGen/Noise/NwNoiseFunctions.h>
#include <Planets/PlanetsCommon.h>
#include <Planets/Noise.h>	// HybridMultiFractal
#include <Utility/VoxelEngine/VoxelTerrainRenderer.h>

#include <Voxels/public/vx5.h>
#include <Voxels/private/Data/Storage/vx5_database.h>	// ChunkDatabase_LMDB

#include "game_forward_declarations.h"
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
//class ChunkNoiseCache: NonCopyable
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
