#include <Base/Base.h>
#pragma hdrstop

#include <Utility/GUI/ImGui/ImGUI_Renderer.h>
#include <VoxelsSupport/VoxelTerrainChunk.h>

#include <VoxelsSupport/VoxelTerrainMaterials.h>
#include <VoxelsSupport/VX_DebugHUD_ImGui.h>

#include <Voxels/private/Scene/vx5_octree_world.h>
#include <Voxels/private/debug/vxp_debug.h>


namespace VXExt
{

namespace
{


#pragma region "Editing"

ERet ImGui_DrawUI_Editing(
	VX5::OctreeWorld& octree_world
	
	, const V3d& eye_pos
	, const V3f& look_direction

	, VX5::DebugViewI& dbgview
	)
{
	if( ImGui::TreeNodeEx( "Editing"
		, ImGuiTreeNodeFlags_DefaultOpen
		) )
	{
		VX5::EditingBrush	editing_brush;


		//
		const double brush_radius = octree_world._info.chunk_size_at_LoD0 * 0.4;

		editing_brush.shape.type = VX5::EditingBrush::Cube;
		editing_brush.shape.cube.center = eye_pos;
		editing_brush.shape.cube.half_size = brush_radius;


		//
		if( ImGui::TreeNodeEx( "Paint"
			, ImGuiTreeNodeFlags_DefaultOpen
			) )
		{
			static float s_paint_brush_radius = brush_radius;
			ImGui::InputFloat("Brush Radius", &s_paint_brush_radius);

			static VoxMatID	s_paint_brush_material = VoxMat_Snow;
			ImGui_drawEnumComboBoxT(&s_paint_brush_material, "Material");

			//

			if( ImGui::Button("Paint" ) )
			{
				//
				VX5::EditHistoryI::ScopedTransaction	edit_transaction(octree_world);

				editing_brush.op.type = VX5::EditingBrush::PaintMaterial;
				editing_brush.op.paint.volume_material = s_paint_brush_material;

				editing_brush.shape.cube.half_size = s_paint_brush_radius;

				//
				octree_world.ApplyBrush(
					editing_brush
					);
			}

			ImGui::TreePop();
		}//CSG Add

		//
		if( ImGui::TreeNodeEx( "CSG Union/Addition"
			, ImGuiTreeNodeFlags_DefaultOpen
			) )
		{
			static float s_csg_add_brush_radius = brush_radius;
			ImGui::InputFloat("Brush Radius", &s_csg_add_brush_radius);

			static VoxMatID	s_add_brush_material = VoxMat_Snow;
			ImGui_drawEnumComboBoxT(&s_add_brush_material, "Volume Material");

			if( ImGui::Button("Add" ) )
			{
				//
				VX5::EditHistoryI::ScopedTransaction	edit_transaction(octree_world);

				editing_brush.op.type = VX5::EditingBrush::CSG_Add;
				editing_brush.op.csg_add.volume_material = s_add_brush_material;
				editing_brush.shape.cube.half_size = s_csg_add_brush_radius;

				//editing_brush.shape.type = VX5::EditingBrush::Sphere;
				//editing_brush.shape.sphere.center = eye_pos;
				//editing_brush.shape.sphere.radius = octree_world._info.chunk_size_at_LoD0 * 0.4;

				octree_world.ApplyBrush(
					editing_brush
					);
			}

			ImGui::TreePop();
		}//CSG Add


		//
		if( ImGui::TreeNodeEx( "CSG Difference/Subtraction"
			, ImGuiTreeNodeFlags_DefaultOpen
			) )
		{
			static float s_csg_remove_brush_radius = brush_radius;
			ImGui::InputFloat("Brush Radius", &s_csg_remove_brush_radius);


			if( ImGui::Button("Subtract" ) )
			{
				//
				VX5::EditHistoryI::ScopedTransaction	edit_transaction(octree_world);

				//
				editing_brush.op.type = VX5::EditingBrush::CSG_Remove;
				editing_brush.shape.cube.half_size = s_csg_remove_brush_radius;

				//editing_brush.shape.type = VX5::EditingBrush::Sphere;
				//editing_brush.shape.sphere.center = eye_pos;
				//editing_brush.shape.sphere.radius = octree_world._info.chunk_size_at_LoD0 * 0.4;

				//const double brush_radius = octree_world._info.chunk_size_at_LoD0 * 0.4;

				//editing_brush.shape.type = VX5::EditingBrush::Box;
				//editing_brush.shape.box.min_corner = eye_pos - CV3d(brush_radius);
				//editing_brush.shape.box.max_corner = eye_pos + CV3d(brush_radius);

				octree_world.ApplyBrush(
					editing_brush
					);
			}

			ImGui::TreePop();
		}//CSG Sub


		////
		//ImGui::Separator();
		////


		ImGui::TreePop();
	}

	return ALL_OK;
}

#pragma endregion


}//namespace


mxDEFINE_CLASS(WorldDebugSettingsExt);
mxBEGIN_REFLECTION(WorldDebugSettingsExt)
	mxMEMBER_FIELD(world_debug_draw_settings),
mxEND_REFLECTION;

WorldDebugSettingsExt::WorldDebugSettingsExt()
{
}


ERet ImGui_DrawVoxelEngineDebugHUD(
	VX5::WorldI& voxel_world
	, WorldDebugSettingsExt & world_debug_settings
	
	, const V3d& eye_pos
	, const V3f& look_direction

	, VX5::DebugViewI& dbgview
	)
{
	if( ImGui::Begin("[VX] Debug") )
	{
		VX5::OctreeWorld& octree_world = *checked_cast<VX5::OctreeWorld*>( &voxel_world );

		//
		VX5::ChunkID_t containing_chunk_id = VX5::ChunkID_t(0,0,0,0);

		const VX5::LODNode* enclosing_node = octree_world.findEnclosingChunk(
			eye_pos
			, containing_chunk_id
			);


		//
		ImGui::Checkbox("Draw LOD Octree?",
			&world_debug_settings.world_debug_draw_settings.draw_lod_octree
			);
		if( ImGui::Button("Clear Debug Lines") ) {
			dbgview.VX_Lock();
			dbgview.VX_Clear();
			dbgview.VX_Unlock();
		}
		//
		if( ImGui::Button("Regenerate world") )
		{
			octree_world.Regenerate();
		}


		//
		if( ImGui::TreeNodeEx( "Current Chunk's Coords", ImGuiTreeNodeFlags_DefaultOpen ) )
		{
			const char* current_chunk_coords_label_name = "<-- Current chunk's coords: ";

			if( enclosing_node )
			{
				const VX5::LODNode* chunk = (VX5::LODNode*) enclosing_node;

				const VX5::LoD::SeamConfig	chunk_seam_config(containing_chunk_id);

				//const bool chunk_is_busy = chunk->state != VX5::Chunk_Ready;

				String128	tmp;
				StringStream	stream(tmp);
				stream << containing_chunk_id;
				ImGui::Text(
					"%s, octant: %d, outer octant: %d, LOD mask: 0x%x",
					tmp.c_str()
					, chunk_seam_config.my_inner_octant
					, chunk_seam_config.my_outer_octant
					, chunk->lod_transition_mask.coarse_neighbors_mask
					//, (int)chunk_is_busy
					);
			}
			else
			{
				ImGui::Text("OUT OF WORLD BOUNDS");
			}



#if 0
			//
			const VX5::ChunkID_t	root_chunk_id = octree_world._info.getRootChunkId();
			const Int3 eye_chunk_coords_at_LoD_0 = octree_world._info.GetChunkCoordsContainingPoint_NoClampingToWorldBounds(eye_pos, 0);

			const bool should_split_chunk = VX5::ShouldSplitChunk2(
				VX5::ChunkID_Key::FromChunkID(root_chunk_id)
				, eye_chunk_coords_at_LoD_0
				, octree_world._settings.lod
				);
			ImGui::Text(
				"Should split root?: %d, eye_chunk_coords_at_LoD_0: (%d, %d, %d)",
				should_split_chunk,
				eye_chunk_coords_at_LoD_0.x,
				eye_chunk_coords_at_LoD_0.y,
				eye_chunk_coords_at_LoD_0.z
				);
#endif
			//
			ImGui::TreePop();
		}

		//
		if( enclosing_node )
		{
			ImGui_DrawUI_Editing(
				octree_world

				, eye_pos
				, look_direction

				, dbgview
				);


#pragma region "Chunk Data"
			if( ImGui::TreeNodeEx( "Chunk Data"
				, ImGuiTreeNodeFlags_DefaultOpen
				) )
			{
				if( ImGui::Button("Show Chunk Voxels" ) )
				{
					VX5::Dbg::ShowChunkVoxels(
						containing_chunk_id
						, octree_world
						, dbgview
						);
				}

				//
				ImGui::Separator();
				//

				ImGui::TreePop();
			}
#pragma endregion




#pragma region "Meshing"
			if( ImGui::TreeNodeEx( "Meshing"
				//, ImGuiTreeNodeFlags_DefaultOpen
				) )
			{
				if( ImGui::Button("Show Octree Cells used for Contouring" ) )
				{
					VX5::Dbg::ShowOctreeCellsUsedForContouring(
						containing_chunk_id
						, octree_world
						);
				}

#if vx5_CFG_DEBUG_MESHING
				if( ImGui::Button("Cast Ray and Show Intersected Triangles" ) )
				{
					VX5::Dbg::CastRay_and_DisplayDebugInfo(
						eye_pos
						, look_direction
						, octree_world
						, dbgview
						);
				}
#endif

				//
				ImGui::Separator();
				//

				//
				static int	debug_chunk_coords[4] = {
					2, 1, 1, 0
				};
				ImGui::LabelText("Debug Chunk ID:", "");
				ImGui::InputInt("X", &debug_chunk_coords[0]);
				ImGui::InputInt("Y", &debug_chunk_coords[1]);
				ImGui::InputInt("Z", &debug_chunk_coords[2]);
				ImGui::InputInt("LOD", &debug_chunk_coords[3]);

				debug_chunk_coords[0] = largest(debug_chunk_coords[0], 0);
				debug_chunk_coords[1] = largest(debug_chunk_coords[1], 0);
				debug_chunk_coords[2] = largest(debug_chunk_coords[2], 0);
				debug_chunk_coords[3] = largest(debug_chunk_coords[3], 0);

				//
				const VX5::ChunkID_t	debug_chunk_id(
					debug_chunk_coords[0],
					debug_chunk_coords[1],
					debug_chunk_coords[2],
					debug_chunk_coords[3]
				);

				//
				if( ImGui::Button("Cast Ray and Show Intersected Cells" ) )
				{
					//ImGui::SetTooltip("Display intersected octree cells used for meshing");

					VX5::Dbg::CastRay_and_ShowIntersectedCells(
						eye_pos
						, look_direction
						, debug_chunk_id
						, octree_world
						, dbgview
						);
				}

				//
				if( ImGui::Button("Cast Ray against Chunk Octree" ) )
				{
					VX5::Dbg::Meshing::IntersectRayWithCellOctree(
						debug_chunk_id
						, V3f::fromXYZ(eye_pos)
						, look_direction
						, octree_world
						);
				}

				//
				ImGui::Separator();
				//


				if(enclosing_node && enclosing_node->mesh)
				{
					VoxelTerrainChunk* renderable_chunk = (VoxelTerrainChunk*) enclosing_node->mesh;
					ImGui::DragFloat3(
						"Dbg Gfx Mesh Offset"
						, renderable_chunk->dbg_mesh_offset.a
						, 0.1f
						, 0
						, 100.0f
						);
				}

#if 0
				//
				ImGui::Separator();
				//

				if( ImGui::Button("Show Active Edges of Octree Cells" ) )
				{
					VX5::Dbg::ShowActiveEdgesOfOctreeCells(
						containing_chunk_id
						, octree_world
						, dbgview
						);
				}
#endif

				ImGui::TreePop();
			}
#pragma endregion

			//
			if( ImGui::TreeNodeEx(
				"LOD / Stitching"
				//, ImGuiTreeNodeFlags_DefaultOpen
				) )
			{
				{
					static int coarse_neighbors_mask = 0;
					ImGui::InputInt(
						"LOD Transition mask (hex; 0 == use cached)", &coarse_neighbors_mask
						, 1, 100
						, ImGuiInputTextFlags_CharsHexadecimal
						);

					//
					if( ImGui::Button("Show Coarse Cells used for Stitching" ) )
					{
						//ImGui::SetTooltip("Display only _coarse_ cells used for meshing");

						VX5::Dbg::ShowCoarseCellsUsedForStitching(
							containing_chunk_id
							, coarse_neighbors_mask
							? VX5::LODTransitionMask(coarse_neighbors_mask)
							: enclosing_node->lod_transition_mask
							, octree_world
							, dbgview
							);
					}
				}

				//
				ImGui::Separator();
				//

				//
				{
					static int coarse_octant_in_seam_octree = 0;
					ImGui::InputInt(
						"Octant in Seam Octree"
						, &coarse_octant_in_seam_octree
						, 1, 1
						, ImGuiInputTextFlags_CharsDecimal
						);

					coarse_octant_in_seam_octree = Clamp(coarse_octant_in_seam_octree, 0, 7);

					if( ImGui::Button("Show_Coarse_Cells_from_Given_Octant_in_Seam_Octree" ) )
					{
						VX5::Dbg::Show_Coarse_Cells_from_Given_Octant_in_Seam_Octree(
							containing_chunk_id
							, coarse_octant_in_seam_octree
							, octree_world
							, dbgview
							);
					}
				}

				ImGui::TreePop();

				if( ImGui::Button("Check LOD transitions masks" ) )
				{
					VX5::Dbg::CheckLodTransitionMasksAreCorrect(
						octree_world
						);
				}
			}


			if( ImGui::TreeNodeEx( "LOD / Meshing"
				//, ImGuiTreeNodeFlags_DefaultOpen
				) )
			{
#if 0
				if( ImGui::Button("Remesh Current Chunk" ) )
				{
					VX5::dbg_remesh_chunk(
						c_cast( VX5::LODNode* ) enclosing_node
						, containing_chunk_id
						, octree_world
						);
				}

				//
				if( ImGui::Button("Show Chunk's Texture Info" ) )
				{
					VX5::dbg_showChunkTextureInfo(
						containing_chunk_id
						, octree_world
						, m_dbgView
						, MemoryHeaps::temporary()
						);
				}

				//
				if( ImGui::Button("Show Chunk's Edges" ) )
				{
					VX5::dbg_showChunkEdges(
						containing_chunk_id
						, octree_world
						, m_dbgView
						, MemoryHeaps::temporary()
						);
				}

				//
				if( ImGui::Button("Show Chunk's Cells" ) )
				{
					VX5::dbg_showChunkCells(
						containing_chunk_id
						, octree_world
						, m_dbgView
						, MemoryHeaps::temporary()
						);
				}

				//
				if( ImGui::Button("Show Chunk's Voxels" ) )
				{
					VX5::ShowChunkVoxels(
						containing_chunk_id
						, octree_world
						, m_dbgView
						, MemoryHeaps::temporary()
						);
				}

				//
				if( ImGui::Button("Show Cells Overlapping Neighbors' Meshes" ) )
				{
					VX5::dbg_show_Cells_overlapping_Meshes_of_Neighboring_Chunks(
						containing_chunk_id
						, octree_world
						, m_dbgView
						, MemoryHeaps::temporary()
						);
				}
#endif

				if( ImGui::Button("Print LOD octree" ) )
				{
					VX5::Dbg::PrintLODOctree(
						octree_world
						);
				}

				//
				ImGui::Separator();
				//

				if( ImGui::Button("Show Actual Coarse Neighbors Mask" ) )
				{
					VX5::Dbg::ShowActualCoarseNeighborsMask(
						containing_chunk_id
						, octree_world
						);
				}
				if( ImGui::Button("Show Cached Coarse Neighbors Mask" ) )
				{
					VX5::Dbg::ShowCachedCoarseNeighborsMask(
						containing_chunk_id
						, octree_world
						);
				}
				if( ImGui::Button("Show LOD Transition Mask" ) )
				{
					VX5::Dbg::ShowLODTransitionMask(
						enclosing_node
						, containing_chunk_id
						, octree_world
						);
				}

				//
				ImGui::Separator();
				//

				if( ImGui::Button("Check if node must be subdivided" ) )
				{
					VX5::Dbg::CheckIfNodeMustBeSubdivided(
						containing_chunk_id
						, octree_world
						);
				}
				if( ImGui::Button("Show External neighbors" ) )
				{
					VX5::Dbg::ShowExternalNeighbors(
						containing_chunk_id
						, octree_world
						);
				}

				if( ImGui::Button("Show Chunk's Seam Space" ) )
				{
					VX5::Dbg::ShowSeamSpaceOctants(
						containing_chunk_id
						, octree_world._info
						);
				}

				//
				if( ImGui::Button("Show Chunk's Fine Octree" ) )
				{
					VX5::dbg_show_Chunk_Octree(
						containing_chunk_id
						, octree_world
						, dbgview
						, MemoryHeaps::temporary()
						, VX5::DbgVizFineCells
						);
				}

				if( ImGui::Button("Show Chunk's Coarse Cells" ) )
				{
					VX5::dbg_show_Chunk_Octree(
						containing_chunk_id
						, octree_world
						, dbgview
						, MemoryHeaps::temporary()
						, VX5::DbgVizCoarseCells
						);
				}

				//
				if( ImGui::Button("Print Chunk's Octree" ) )
				{
					VX5::Dbg::PrintChunkFineCells(
						containing_chunk_id
						, octree_world
						, MemoryHeaps::temporary()
						);
				}


				//
				if( ImGui::Button("Check Ghost Cells Synced with Neighbors" ) )
				{
					VX5::Dbg::dbg_check_ghost_cells_are_synced_with_neighbors(
						containing_chunk_id
						, octree_world
						, dbgview
						);
				}

				ImGui::TreePop();
			}

#if 0
			//
			if( ImGui::TreeNodeEx( "Ghost Cells Visualization", ImGuiTreeNodeFlags_DefaultOpen ) )
			{
				VoxelEngineDebugState::GhostCellsVisualization & ghost_cells_visualization = voxel_engine_debug_state.ghost_cells;

				if( ImGui::Button("Show Ghost Cells of Current Chunk" ) )
				{
					VX5::dbg_show_Ghost_Cells_of_Chunk_with_ID(
						containing_chunk_id
						, octree_world
						, m_dbgView
						, MemoryHeaps::temporary()
						, ghost_cells_visualization.show_cached_coarse_cells
						, ghost_cells_visualization.show_ghost_cells
						, ghost_cells_visualization.show_cells_overlapping_maximal_neighbors_mesh
						);
				}
				ImGui::TreePop();
			}

			//
			if( ImGui::Button("Show Chunk's Octree External/Ghost Cells" ) )
			{
				const VX5::MooreNeighborsMask27 which_neighbors_to_show = {
					voxel_engine_debug_state.show_ghost_cells_in_these_neighbors_mask.raw_value
				};
				VX5::dbg_show_Cells_located_inside_Neighboring_Chunks(
					containing_chunk_id
					, which_neighbors_to_show
					, octree_world
					, m_dbgView
					, MemoryHeaps::temporary()
					);
			}
#endif


			//
			if( ImGui::TreeNodeEx( "LOD / Updating"
				//, ImGuiTreeNodeFlags_DefaultOpen
				) )
			{
				//
				if( ImGui::Button("Print Queued Chunks for LoD updates" ) )
				{
					octree_world._dirty_lod_updater.DbgPrint(
						octree_world._info.num_LoDs
						);
				}

				ImGui::TreePop();
			}

		}//if enclosing node != nil





#if 0
		//
		if( ImGui::TreeNodeEx( "Level Of Detail", ImGuiTreeNodeFlags_DefaultOpen ) )
		{
			VoxelEngineDebugState::LevelOfDetailStitchingState & LoD_stitching = voxel_engine_debug_state.LoD_stitching;

			//
			if( ImGui::Button("Copy Coords From Current Chunk" ) )
			{
				LoD_stitching.debug_chunk_id.setFromChunkID( containing_chunk_id );
			}

			//
			if( ImGui::Button("Show Bounds For Selecting Cells From Coarse Neighbors" ) )
			{
				VX5::dbg_showBounds_for_SelectingCells_from_CoarseNeighbors(
					LoD_stitching.debug_chunk_id.asChunkId()
					, octree_world
					, m_dbgView
					, LoD_stitching.mask_to_show_bounds_only_for_specified_coarse_neighbors
					, LoD_stitching.show_bounds_for_C0_continuity_only
					);
			}

			//
			if( ImGui::Button("Show Cached Cells of Coarse Neighbors" ) )
			{
				VX5::dbg_show_Cached_Cells_of_Coarse_Neighbors(
					LoD_stitching.debug_chunk_id.asChunkId()
					, octree_world
					, m_dbgView
					, LoD_stitching.top_level_octants_mask_in_seam_octree
					, LoD_stitching.show_coarse_cells_needed_for_C0_continuity
					, LoD_stitching.show_coarse_cells_needed_for_C1_continuity
					);
			}

			ImGui::TreePop();
		}//


		//
		ImGui_DrawPropertyGrid(
			&voxel_engine_debug_state,
			mxCLASS_OF(voxel_engine_debug_state),
			"Voxel Engine Debug Settings"
			);
#endif

		//
		if( ImGui::TreeNodeEx( "Unit Tests"
			//, ImGuiTreeNodeFlags_DefaultOpen
			) )
		{
			if( ImGui::Button("Test Coarse Neighbor Octants" ) )
			{
				VX5::Dbg::Test_CheckCoarseNeighborsIDs(
					containing_chunk_id
					, octree_world
					);
			}

			ImGui::Separator();

			static int	mask_of_which_cell_types_to_show;
			ImGui::InputInt("Cell Type Mask", &mask_of_which_cell_types_to_show);

			if( ImGui::Button("Test Octree Cell Type Classification" ) )
			{
				VX5::Dbg::Test_VisualizeCellsOfTypes(
					containing_chunk_id
					, octree_world
					, mask_of_which_cell_types_to_show
					);
			}

			ImGui::TreePop();
		}

		//
		if( ImGui::TreeNodeEx( "Chunk Debugging 2"
			//, ImGuiTreeNodeFlags_DefaultOpen
			) )
		{
			if( ImGui::Button("Remesh current chunk" ) )
			{
				VX5::Dbg::RemeshChunk(
					containing_chunk_id
					, octree_world
					);
			}

			ImGui::TreePop();
		}
	}
	ImGui::End();
	return ALL_OK;
}

}//namespace VXExt
