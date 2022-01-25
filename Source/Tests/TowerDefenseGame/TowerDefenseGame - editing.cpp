#include <Base/Base.h>
#pragma once

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
#include <Renderer/Shaders/__shader_globals_common.h>	// G_VoxelTerrain
#include <Renderer/private/shader_uniforms.h>	// ShaderGlobals

#include <Utility/Meshok/SDF.h>
#include <Utility/Meshok/BSP.h>
#include <Utility/Meshok/BIH.h>
#include <Utility/Meshok/Noise.h>
#include <Utility/Meshok/Volumes.h>
#include <Utility/MeshLib/Simplification.h>
#include <Utility/Meshok/Octree.h>
#include <Utility/TxTSupport/TxTConfig.h>
#include <Utility/VoxelEngine/VoxelizedMesh.h>

#include "TowerDefenseGame.h"

ERet TowerDefenseGame::loadOrCreateModel(
	TRefPtr< VX::VoxelizedMesh > &model
	, const char* _sourceMesh
	)
{
	UNDONE;
	return ERR_NOT_IMPLEMENTED;

	//const char *	path_to_compiled_files_cache = RELEASE_BUILD
	//	? _path_to_app_cache.c_str()
	//	: "D:/research/__test/meshes/__compiled/";

	//bool _forceRebuild = false;
	//return VX::loadOrCreateModel( model, _sourceMesh, MemoryHeaps::temporary(), path_to_compiled_files_cache, _forceRebuild );
}

void TowerDefenseGame::_applyEditingOnLeftMouseButtonClick( const VX::SVoxelToolSettings_All& voxel_tool_settings
															 , const InputState& input_state )
{
	switch( voxel_tool_settings.current_tool )
	{
	case VX::VoxelToolType::CSG_Add_Shape:
		{
			const VX::SVoxelToolSettings_CSG_Add& additive_brush_settings = voxel_tool_settings.csg_add;
			if( _voxel_tool_raycast_hit_anything )
			{
				VX::BrushParams	brush;
				brush.center_in_world_space = _last_pos_where_voxel_tool_raycast_hit_world;
				brush.radius_in_world_units = additive_brush_settings.brush_radius_in_world_units;
				brush.edit_mode = VX::BrushEdit_Add;
				brush.shape_type = additive_brush_settings.brush_shape_type;
				brush.volume_material = additive_brush_settings.fill_material;

				if( brush.volume_material != VX5::EMPTY_SPACE )
				{
					_voxel_world.editWithBrush( brush );
				}
				else
				{
					ptWARN("Select a non-empty material in brush settings!");
				}
			}
		}
		break;

	case VX::VoxelToolType::CSG_Subtract_Shape:
		{
			const VX::SVoxelToolSettings_CSG_Subtract& csg_subtract = voxel_tool_settings.csg_subtract;
			if( _voxel_tool_raycast_hit_anything )
			{
				VX::BrushParams	brush;
				brush.center_in_world_space = _last_pos_where_voxel_tool_raycast_hit_world;
				brush.radius_in_world_units = csg_subtract.brush_radius_in_world_units;
				brush.edit_mode = VX::BrushEdit_Remove;
				brush.shape_type = csg_subtract.brush_shape_type;

				_voxel_world.editWithBrush( brush );
			}
		}
		break;

	case VX::VoxelToolType::CSG_Add_Model:
		break;

	case VX::VoxelToolType::CSG_Subtract_Model:
		break;

	case VX::VoxelToolType::Paint_Brush:
		break;

	case VX::VoxelToolType::SmoothGeometry:
		break;

	case VX::VoxelToolType::Grow:
		break;

	case VX::VoxelToolType::Shrink:
		break;

	default:
		LogStream(LL_Warn),"Unhandled tool: ",m_settings.voxel_tool.current_tool;
	}
}
