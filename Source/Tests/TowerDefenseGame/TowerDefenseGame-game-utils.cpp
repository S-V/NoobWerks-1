#include <Lua/lua.hpp>
#include <LuaBridge/LuaBridge.h>

#include <Base/Base.h>
#include <Base/Template/Containers/Blob.h>
#include <Base/Template/Containers/BitSet/BitArray.h>
#include <Base/Text/String.h>
#include <Base/Math/ViewFrustum.h>

#include <bx/bx.h>	// uint8_t

//#include <libnoise/noise.h>
//#pragma comment( lib, "libnoise.lib" )

#include <Base/Template/Containers/Array/TLocalArray.h>
#include <Core/Client.h>
#include <Core/ConsoleVariable.h>
#include <Core/Serialization/Serialization.h>
#include <Core/Tasking/TaskSchedulerInterface.h>
#include <Core/Tasking/SlowTasks.h>
#include <Core/Util/ScopedTimer.h>
#include <Graphics/private/d3d_common.h>
#include <Driver/Driver.h>	// Window stuff
#include <Developer/TypeDatabase/TypeDatabase.h>

#include <Utility/TxTSupport/TxTSerializers.h>

#include <Engine/Engine.h>

#include <Renderer/Core/Material.h>
#include <Renderer/private/RenderSystem.h>
#include <Renderer/private/shader_uniforms.h>
#include <Renderer/Shaders/__shader_globals_common.h>	// G_VoxelTerrain

#include <Utility/MeshLib/TriMesh.h>

#include <Utility/VoxelEngine/VoxelTerrainChunk.h>
#include <Utility/VoxelEngine/VoxelTerrainRenderer.h>

#include <Sound/SoundSystem.h>

#include "TowerDefenseGame-game-utils.h"


//QQQ
void test_PlaySound()
{
	NwSoundSystemI::Get().test_playSound2D(
		);
}
