// Voxel-based Level editor.
//
#include "stdafx.h"
#pragma hdrstop

#include <gainput/gainput.h>

//
#include <ImGui/imgui.h>
// ImGuizmo::DecomposeMatrixToComponents
#include <ImGuizmo/ImGuizmo.h>

#include <Base/Memory/Allocators/FixedBufferAllocator.h>

#include <Core/Memory/MemoryHeaps.h>
#include <Core/Serialization/Text/TxTSerializers.h>

#include <Renderer/private/shader_uniforms.h>

#include <Engine/WindowsDriver.h>

#include <Utility/GUI/Nuklear/Nuklear_Support.h>
#include <Utility/GUI/ImGui/ImGUI_Renderer.h>

#include <VoxelsSupport/MeshStamp.h>

//
#include <Voxels/public/vx5.h>
//
#include "game_states/game_states.h"
#include "game_states/game_state_world_editor.h"
#include "Localization/game_localization.h"

#include "game_world/pickable_items.h"	// SpawnItem()
#include "gameplay_constants.h"	// PickableItem8

#include "FPSGame.h"



namespace
{
	static const char* DEFAULT_LEVEL_FILEPATH = 
		//"R:/test_level.son"
		"D:/dev/___FPS_Game_Prealpha/__TO_ADD_TO_ASSETS/Levels/test_level.son"
		;
	static const char* SAVED_VOXELS_FILENAME =
		"R:/test_level.vxl"
		;
}


EditingOperationInProgress::EditingOperationInProgress()
{
	is_valid = false;
}

static
void composeDrawMeshCommand( GL::Cmd_Draw *batch_
							, const NwMesh& mesh
							, const HProgram shader_program
							, const NwRenderState32& render_state
							)
{
	batch_->reset();
	batch_->states = render_state;

	//
	batch_->inputs = GL::Cmd_Draw::EncodeInputs(
		mesh.vertex_format->input_layout_handle,
		mesh.m_topology,
		mesh.m_flags & MESH_USES_32BIT_IB
		);

	//
	batch_->VB = mesh.geometry_buffers.VB->buffer_handle;
	batch_->IB = mesh.geometry_buffers.IB->buffer_handle;

	//
	batch_->base_vertex = 0;
	batch_->vertex_count = mesh.m_numVertices;
	batch_->start_index = 0;
	batch_->index_count = mesh.m_numIndices;

	//
	batch_->program = shader_program;
}


ERet EditingOperationInProgress::DrawShape(
	MyGameRenderer& game_renderer
	, const MyGameSettings& game_settings
	, NwClump& storage_clump
	)
{
	TResPtr<NwMesh>	brush_mesh;


	//
	switch(current_shape_type)
	{
	case VoxelBrushShape::Sphere:
		brush_mesh._setId(MakeAssetID("unit_sphere_ico"));
		break;

	case VoxelBrushShape::Cube:
		brush_mesh._setId(MakeAssetID("unit_cube"));
		break;

	case VoxelBrushShape::Mesh:
		brush_mesh = mesh_brush.mesh;
		break;

		mxNO_SWITCH_DEFAULT;
	}

	if(!brush_mesh._id.IsValid()) {
		return ERR_OBJECT_NOT_FOUND;
	}

	//
	mxTRY(VXExt::LoadMeshForStamping(
		brush_mesh
		, storage_clump
		));


	//
	const NwCameraView& camera_matrices = game_renderer._camera_matrices;

	const TbRenderSystemI& render_system = *game_renderer._render_system;

	NwShaderEffect* shader;
	mxDO(Resources::Load(shader, MakeAssetID("voxel_tool_shape"), &storage_clump));

	//
	const NwShaderEffect::Variant& default_variant = shader->getDefaultVariant();

	const U32 view_id = render_system.getRenderPath()
		.getPassDrawingOrder(ScenePasses::Translucent)
		;

	//
	GL::NwRenderContext& render_context = GL::getMainRenderContext();
	GL::CommandBufferWriter	cmd_writer( render_context._command_buffer );

	//
	const U32 first_command_offset = render_context._command_buffer.currentOffset();

	// Update constant buffers.
	{
		G_PerObject	*	per_object_cb_data;
		mxDO(cmd_writer.allocateUpdateBufferCommandWithData(
			render_system._global_uniforms.per_model.handle
			, &per_object_cb_data
			));

		ShaderGlobals::copyPerObjectConstants(
			per_object_cb_data
			, gizmo_transform // local_to_world_matrix
			, camera_matrices.view_matrix
			, camera_matrices.view_projection_matrix
			, 0
			);
	}

	//
	GL::Cmd_Draw	batch;

	composeDrawMeshCommand( &batch
		, *brush_mesh
		, default_variant.program
		, default_variant.render_state
		);

	IF_DEBUG batch.src_loc = GFX_SRCFILE_STRING;

	cmd_writer.draw( batch );

	//
	render_context.submit(
		first_command_offset,
		GL::buildSortKey(view_id,
		GL::buildSortKey(batch.program, 0)
		)
		);

	return ALL_OK;
}

static ERet GetSDFShape(
						SDF::Shape &sdf_shape_,
						SDF::ShapeType &sdf_shape_type_,
						const VoxelEditorSettings& editor
						)
{
	switch(editor.current_shape_type)
	{
	case VoxelBrushShape::Sphere:
		sdf_shape_type_ = SDF::SHAPE_SPHERE;
		sdf_shape_.sphere.center = V3f::fromXYZ(editor.sphere_brush.center);
		sdf_shape_.sphere.radius = editor.sphere_brush.radius;
		break;

	case VoxelBrushShape::Cube:
		sdf_shape_type_ = SDF::SHAPE_CUBE;
		sdf_shape_.cube.center = V3f::fromXYZ(editor.cube_brush.center);
		sdf_shape_.cube.half_size = editor.cube_brush.half_size;
		break;

	case VoxelBrushShape::Mesh:
		sdf_shape_type_ = SDF::SHAPE_MESH;
		strcpy_s( sdf_shape_.mesh.name, editor.mesh_brush.mesh._id.c_str() );
		//sdf_shape_.mesh.half_size = editor.cube_brush.half_size;
		break;

		mxNO_SWITCH_DEFAULT;
	}
	return ALL_OK;
}

static SDF::OpType GetSDFOpType(
						const VoxelEditorSettings& editor
						)
{
	switch(editor.current_tool_type)
	{
	case VoxelTool::CSG_Add_Shape:
		return SDF::OpType::CSG_ADD;

		mxNO_SWITCH_DEFAULT;
	}
	return SDF::OpType::CSG_ADD;
}

ERet EditingOperationInProgress::Apply(
									   SDF::Tape &blob_tree
									   , VX5::WorldI* voxel_world
									   , NwClump& storage_clump
									   , AABBf *changed_region_aabb_ /*= nil*/
									   )
{
	DBGOUT("[Editor] Applying...");

	FPSGame::Get().world.voxels.voxel_engine_dbg_drawer.VX_Clear_AnyThread();

	//
	V3f matrix_translation, matrix_rotation_angles_deg, matrix_scale;
	ImGuizmo::DecomposeMatrixToComponents(
		gizmo_transform.a,
		matrix_translation.a, matrix_rotation_angles_deg.a, matrix_scale.a
		);
	LogStream(LL_Info)
		,"T: ", matrix_translation,"\n"
		,"R: ", matrix_rotation_angles_deg,"\n"
		,"S: ", matrix_scale,"\n"
		;

	//
	if( current_shape_type == VoxelBrushShape::Mesh )
	{
		//
		SDF::Transform	world_transform;
		world_transform.translation = matrix_translation;
		world_transform.rotation_angles_deg = matrix_rotation_angles_deg;
		world_transform.scaling = matrix_scale;

		AABBf	changed_region_aabb;

		mxDO(blob_tree.AddMesh(
			mesh_brush.mesh._id
			, world_transform
			, current_material
			, storage_clump
			, voxel_world
			, &changed_region_aabb
			));

		if(changed_region_aabb_) {
			*changed_region_aabb_ = changed_region_aabb;
		}
	}
	else
	{
		const SDF::OpType op_type = GetSDFOpType(*this);

		SDF::Shape	sdf_shape;
		SDF::ShapeType	sdf_shape_type;
		mxDO(GetSDFShape(
			sdf_shape,
			sdf_shape_type,
			*this
			));
UNDONE;
		//
		//mxDO(loaded_tape.AddEditOp(
		//	op_type
		//	, sdf_shape
		//	, sdf_shape_type
		//	, current_material
		//	));
	}

	return ALL_OK;
}


/*
-----------------------------------------------------------------------------
	MyGameState_LevelEditor
-----------------------------------------------------------------------------
*/
MyGameState_LevelEditor::MyGameState_LevelEditor()
	: loaded_tape(MemoryHeaps::process())
{
	this->DbgSetName("WorldEditor(Voxels)");

	Str::CopyS(default_save_file, DEFAULT_LEVEL_FILEPATH);

	npc_spawn_point_comment[0]=0;
}

ERet MyGameState_LevelEditor::Initialize(
	const MyGameSettings& game_settings
	)
{
	const VoxelEditorSettings& src_voxel_editor_settings = game_settings.editor.voxel_editor;
	TCopyBase( edit_op_in_progress, src_voxel_editor_settings );
	return ALL_OK;
}

void MyGameState_LevelEditor::Shutdown()
{
}

void MyGameState_LevelEditor::tick(
								float delta_time_in_seconds
								, NwGameI* game_app
								)
{
	ImGui_Renderer::UpdateInputs();
}

EStateRunResult MyGameState_LevelEditor::handleInput(
	NwGameI* game_app
	, const NwTime& game_time
)
{
	// to prevent chatter
	static U32 last_keypress_time_msec;
	enum { KEYPRESS_INTERVAL_MSEC = 100 };

	const U32 curr_time_msec = mxGetTimeInMilliseconds();


	//
	FPSGame* game = checked_cast< FPSGame* >( game_app );

	//const InputState& input_state = NwInputSystemI::Get().getState();
	const InputState& input_state = game->input._input_system->getState();

	const gainput::InputMap& input_map = *game->input._input_map;

	//
	const gainput::InputState& keyboard_input_state =
		*game->input._keyboard->GetInputState()
		;

	if( game->input.wasButtonDown( eGA_Exit ) )
	{
		// If editing...
		if(edit_op_in_progress.is_valid)
		{
			// ...stop editing.
			_CancelCurrentOp();
			return ContinueFurtherProcessing;
		}

		game->state_mgr.popState();
		return StopFurtherProcessing;
	}

	//
	if( game->input.wasButtonDown( eGA_Show_Debug_HUD ) )
	{
		game->state_mgr.pushState(
			&game->states->state_debug_hud
			);
		return StopFurtherProcessing;
	}

	//
	const bool LCtrl_held = keyboard_input_state.GetBool( gainput::KeyCtrlL );
	const bool RCtrl_held = keyboard_input_state.GetBool( gainput::KeyCtrlR );
	const bool Ctrl_held = LCtrl_held || RCtrl_held;

	//
	if(
		!Ctrl_held	// if not controlling the camera
		&&
		game->input.wasButtonDown( eGA_Editor_StartOperation )
		//keyboard_input_state.GetBool( gainput::KeySpace )
		&& curr_time_msec - last_keypress_time_msec > KEYPRESS_INTERVAL_MSEC
		)
	{
		last_keypress_time_msec = mxGetTimeInMilliseconds();

		_StartEditingWithCurrentOp();
	}

	//
	if(
		game->input.wasButtonDown( eGA_Editor_ApplyOperation )
		//input_state.keyboard.held[KEY_Enter]
		//keyboard_input_state.GetBool( gainput::KeyReturn )
		&&
		edit_op_in_progress.is_valid
		&& curr_time_msec - last_keypress_time_msec > KEYPRESS_INTERVAL_MSEC
		)
	{
		last_keypress_time_msec = mxGetTimeInMilliseconds();

		_ApplyCurrentOp();
	}

	//
	if( Ctrl_held
		&&
		game->input.wasButtonDown( eGA_Editor_UndoOperation )
		&& curr_time_msec - last_keypress_time_msec > KEYPRESS_INTERVAL_MSEC
		//input_state.keyboard.held[KEY_Z]
		//keyboard_input_state.GetBool( gainput::KeyZ )
		)
	{
		last_keypress_time_msec = mxGetTimeInMilliseconds();

		_UndoLastChange();
	}
	//
	if( Ctrl_held
		&&
		(
		keyboard_input_state.GetBool( gainput::KeyY )
		||
		keyboard_input_state.GetBool( gainput::KeyX )
		)
		)
	{
		_RedoLastChange();
	}

	//
	if( !Ctrl_held )
	{
		if( keyboard_input_state.GetBool( gainput::KeyT ) )
		{
			edit_op_in_progress.gizmo_manipulator.gizmo_operation = GizmoOperation::Translate;
		}
		if( keyboard_input_state.GetBool( gainput::KeyR ) )
		{
			edit_op_in_progress.gizmo_manipulator.gizmo_operation = GizmoOperation::Rotate;
		}
		if( keyboard_input_state.GetBool( gainput::KeyS ) )
		{
			edit_op_in_progress.gizmo_manipulator.gizmo_operation = GizmoOperation::Scale;
		}
	}

	//
	return ContinueFurtherProcessing;
}

bool MyGameState_LevelEditor::allowsFirstPersonCameraToReceiveInput() const
{
	FPSGame* game = &FPSGame::Get();

	const gainput::InputMap& input_map = *game->input._input_map;

	const gainput::InputState& keyboard_input_state =
		*game->input._keyboard->GetInputState()
		;

	//
	//const bool LShift_held = keyboard_input_state.GetBool( gainput::KeyShiftL );
	//const bool RShift_held = keyboard_input_state.GetBool( gainput::KeyShiftR );
	//const bool Shift_held = LShift_held || RShift_held;

	const bool LCtrl_held = keyboard_input_state.GetBool( gainput::KeyCtrlL );
	const bool RCtrl_held = keyboard_input_state.GetBool( gainput::KeyCtrlR );
	const bool Ctrl_held = LCtrl_held || RCtrl_held;

	return Ctrl_held;
}

EStateRunResult MyGameState_LevelEditor::drawScene(
	NwGameI* game_app
	, const NwTime& game_time
	)
{
	FPSGame& the_game = FPSGame::Get();

	//
	MyGameRenderer& game_renderer = the_game.renderer;
	const MyGameSettings& game_settings = the_game.settings;

	//
	if(edit_op_in_progress.is_valid)
	{
		edit_op_in_progress.DrawShape(
			game_renderer
			, game_settings
			, the_game.runtime_clump
			);
	}

	return ContinueFurtherProcessing;
}

bool ImGui_BeginFullScreenWindow()
{
	ImGui::SetNextWindowPos( ImVec2(0,0) );
	ImGui::SetNextWindowSize( ImGui::GetIO().DisplaySize );

	return ImGui::Begin(
		"Fullscreen Window for ImGuizmo",
		nil,
		ImGuiWindowFlags_NoDecoration|
		ImGuiWindowFlags_NoTitleBar|
		ImGuiWindowFlags_NoResize|
		ImGuiWindowFlags_NoScrollbar|
		ImGuiWindowFlags_NoMove|
		ImGuiWindowFlags_NoCollapse|
		ImGuiWindowFlags_NoBackground|
		ImGuiWindowFlags_NoInputs
		);
}

void MyGameState_LevelEditor::drawUI(
								   float delta_time_in_seconds
								   , NwGameI* game_app
								   )
{
	FPSGame* game = checked_cast< FPSGame* >( game_app );

	MyGameSettings & game_settings = game->settings;

	//
	//const bool draw_ImGui_gizmo = edit_op_in_progress.is_valid;

	//
	_ImGui_DrawPlacementEditor();

	//
	if( ImGui::Begin("Level File") )
	{
		ImGui::InputText("File Path",
			default_save_file.raw(),
			default_save_file.capacity()
			);

		if(ImGui::Button("Save To File"))
		{
			_SaveEditsToTape();
		}
		if(ImGui::Button("Load From File"))
		{
			_LoadEditsFromTape();
		}
		if(ImGui::Button("Apply to World"))
		{
			game->world.ApplyEditsFromTape(loaded_tape);
		}

		ImGui::Separator();
		if(ImGui::Button("Save Voxel World To File"))
		{
			game->world.voxels.voxel_world->SaveDataToFile(
				SAVED_VOXELS_FILENAME
				);
		}
		if(ImGui::Button("Load Voxel World From File"))
		{
			game->world.voxels.voxel_world->LoadFromFile(
				SAVED_VOXELS_FILENAME
				);
		}
	}
	ImGui::End();

	//
	if( ImGui::Begin("Voxel Edit Tool") )
	{
		_ImGui_DrawShapeUI();
	}
	ImGui::End();

	_ImGui_DrawEditOperationUI();
	//
	if(edit_op_in_progress.is_valid)
	{
		edit_op_in_progress.gizmo_manipulator.beginDrawing();

		//
		if( ImGui_BeginFullScreenWindow() )
		{
			edit_op_in_progress.gizmo_manipulator.drawGizmoAndManipulate(
				edit_op_in_progress.gizmo_transform
				, game->renderer._camera_matrices
				, ImGui::GetIO()
				);
			//
			//static int cnt;
			//++cnt;
			//if(cnt%8==0)
			//{
			//	LogStream(LL_Warn),
			//		"Gizmo xform: ",edit_op_in_progress.gizmo_manipulator._local_to_world
			//		;
			//}
		}
		ImGui::End();
	}
}


// file scope to view in debugger
AttackWavesT s_attack_waves = Wave0;

void MyGameState_LevelEditor::_ImGui_DrawPlacementEditor()
{
	if( ImGui::Begin("Item Placement") )
	{
		static PickableItem8 item_type;
		ImGui_drawEnumComboBoxT(
			&item_type, "Item"
			);

		//
		ImGui_DrawFlagsCheckboxesT(&s_attack_waves, "Attack Waves");

		if(ImGui::Button("Create"))
		{
			MyLevelData::ItemSpawnPoint	new_item_spawn_point;
			new_item_spawn_point.pos = MyLevelData_::GetPosForSpawnPoint();
			new_item_spawn_point.type = item_type;
			new_item_spawn_point.waves = s_attack_waves;

			level_data.item_spawn_points.add(new_item_spawn_point);

			//
			MyGameUtils::SpawnItem(item_type, new_item_spawn_point.pos);
		}
	}
	ImGui::End();

	//
	if( ImGui::Begin("Entity Placement") )
	{
		if(ImGui::Button("Set Player Spawn Point"))
		{
			level_data.player_spawn_pos = FPSGame::Get().player.GetPhysicsBodyCenter();
		}
		if(ImGui::Button("Set Mission Exit Point"))
		{
			level_data.mission_exit_pos = FPSGame::Get().player.GetPhysicsBodyCenter();
		}


		//
		static MonsterType8	monster_type;
		ImGui_drawEnumComboBoxT(
			&monster_type, "Monster"
			);

		ImGui::InputText(
			"Comment",
			npc_spawn_point_comment, sizeof(npc_spawn_point_comment)
			);

		static int num_monsters_to_spawn = 1;
		ImGui::InputInt(
			"Count",
			&num_monsters_to_spawn
			);
		num_monsters_to_spawn = Clamp(num_monsters_to_spawn, 1, 10);

		//
		ImGui_DrawFlagsCheckboxesT(&s_attack_waves, "Attack Waves");

		//
		if(ImGui::Button("Create"))
		{
			MyLevelData::NpcSpawnPoint	new_monster_spawn_point;
			new_monster_spawn_point.pos = MyLevelData_::GetPosForSpawnPoint();
			new_monster_spawn_point.type = monster_type;
			new_monster_spawn_point.count = num_monsters_to_spawn;
			new_monster_spawn_point.waves = s_attack_waves;
			Str::CopyS(new_monster_spawn_point.comment, npc_spawn_point_comment);

			level_data.npc_spawn_points.add(new_monster_spawn_point);

			//
			MyGameUtils::SpawnMonster(
				new_monster_spawn_point.pos
				, monster_type
				);
		}
	}
	ImGui::End();
}

void MyGameState_LevelEditor::_ImGui_DrawShapeUI()
{
	if( ImGui::BeginChild("Shape") )
	{
		//
		bool show_open = true;
		if( ImGui::Begin("Transform", &show_open) )
		{
			{
				ImGui::InputFloat4("row0", edit_op_in_progress.gizmo_transform.v0.a);
				ImGui::InputFloat4("row1", edit_op_in_progress.gizmo_transform.v1.a);
				ImGui::InputFloat4("row2", edit_op_in_progress.gizmo_transform.v2.a);
				ImGui::InputFloat4("row3", edit_op_in_progress.gizmo_transform.v3.a);
			}

			if(ImGui::Button("Reset")) {
				edit_op_in_progress.gizmo_transform = M44_Identity();
			}
			{
				V3f matrix_translation, matrix_rotation_angles_deg, matrix_scale;
				ImGuizmo::DecomposeMatrixToComponents(
					edit_op_in_progress.gizmo_transform.a,
					matrix_translation.a, matrix_rotation_angles_deg.a, matrix_scale.a
					);
				//
				bool	rebuild_gizmo_matrix = false;

				if(ImGui::DragFloat3("Translation", matrix_translation.a)) {
					rebuild_gizmo_matrix = true;
				}
				if(ImGui::DragFloat3("Rotation", matrix_rotation_angles_deg.a)) {
					rebuild_gizmo_matrix = true;
				}
				if(ImGui::DragFloat3("Scale", matrix_scale.a)) {
					rebuild_gizmo_matrix = true;
				}

				if(rebuild_gizmo_matrix)
				{
					ImGuizmo::RecomposeMatrixFromComponents(
						matrix_translation.a,
						matrix_rotation_angles_deg.a,
						matrix_scale.a,
						edit_op_in_progress.gizmo_transform.a
						);
				}
			}
		}
		ImGui::End();

		//
		ImGui_drawEnumComboBoxT(
			&edit_op_in_progress.current_shape_type, "Shape"
			);

		switch(edit_op_in_progress.current_shape_type)
		{
		case VoxelBrushShape::Sphere:
			ImGui_DrawPropertyGridT(
				&edit_op_in_progress.sphere_brush,
				"Sphere"
				);
			break;

		case VoxelBrushShape::Cube:
			ImGui_DrawPropertyGridT(
				&edit_op_in_progress.cube_brush,
				"Cube"
				);
			break;

		case VoxelBrushShape::Mesh:
			ImGui_DrawPropertyGridT(
				&edit_op_in_progress.mesh_brush,
				"Mesh"
				);
			if(edit_op_in_progress.is_valid)
			{
				VXExt::LoadMeshForStamping(
					edit_op_in_progress.mesh_brush.mesh
					, FPSGame::Get().runtime_clump
					);

				if(NwMesh* loaded_mesh = edit_op_in_progress.mesh_brush.mesh._ptr)
				{
					AABBf transformed_mesh_aabb_in_world_space
						= loaded_mesh->local_aabb
						.transformed(edit_op_in_progress.gizmo_transform)
						;
					ImGui::InputFloat3(
						"World AABB Min",
						transformed_mesh_aabb_in_world_space.min_corner.a,
						"%.3f",
						ImGuiInputTextFlags_ReadOnly
						);
					ImGui::InputFloat3(
						"World AABB Max",
						transformed_mesh_aabb_in_world_space.max_corner.a,
						"%.3f",
						ImGuiInputTextFlags_ReadOnly
						);

					//
					V3f	world_aabb_size = transformed_mesh_aabb_in_world_space.size();
					ImGui::InputFloat3(
						"World AABB Size",
						world_aabb_size.a,
						"%.3f",
						ImGuiInputTextFlags_ReadOnly
						);
				}
			}
			break;

			mxNO_SWITCH_DEFAULT;
		}
	}
	ImGui::EndChild();
}

void MyGameState_LevelEditor::_ImGui_DrawEditOperationUI()
{
	if( ImGui::Begin("EditOp") )
	{
		ImGui_drawEnumComboBoxT(
			&edit_op_in_progress.current_tool_type, "Tool"
			);

		//
		ImGui_drawEnumComboBoxT(
			&edit_op_in_progress.gizmo_manipulator.gizmo_operation, "Gizmo"
			);

		//
		ImGui_drawEnumComboBoxT(
			&edit_op_in_progress.current_material, "Material"
			);

		//
		if(ImGui::Button("Start"))
		{
			// force reload if mesh ID changed
			edit_op_in_progress.mesh_brush.mesh._ptr = nil;

			_StartEditingWithCurrentOp();
		}

		//
		if(edit_op_in_progress.is_valid)
		{
			if(ImGui::Button("Apply"))
			{
				_ApplyCurrentOp();
			}
			if(ImGui::Button("Cancel"))
			{
				_CancelCurrentOp();
			}
		}
	}
	ImGui::End();
}

void MyGameState_LevelEditor::_StartEditingWithCurrentOp()
{
	DBGOUT("Starting editing...");

	edit_op_in_progress.is_valid = true;

	//
	VoxelEditorSettings& voxel_editor_settings
		= FPSGame::Get().settings.editor.voxel_editor
		;
	voxel_editor_settings = edit_op_in_progress;
}

ERet MyGameState_LevelEditor::_ApplyCurrentOp()
{
	mxENSURE(edit_op_in_progress.is_valid, ERR_INVALID_FUNCTION_CALL, "");
	DBGOUT("Applying current edit op...");

	AABBf	changed_region_aabb;
	mxDO(edit_op_in_progress.Apply(
		loaded_tape
		, FPSGame::Get().world.voxels.voxel_world
		, FPSGame::Get().runtime_clump
		, &changed_region_aabb
		));

	//NOTE: don't rebuild the world from proc data source - we stamp mesh immediately
	//_RebuildChangedWorldParts(changed_region_aabb);

	return ALL_OK;
}

void MyGameState_LevelEditor::_CancelCurrentOp()
{
	DBGOUT("Cancelling current edit op...");

	edit_op_in_progress.is_valid = false;
}

void MyGameState_LevelEditor::_UndoLastChange()
{
	AABBf	changed_region_aabb;
	if(mxSUCCEDED(loaded_tape.UndoLastEdit(&changed_region_aabb)))
	{
		_RebuildChangedWorldParts(changed_region_aabb);
	}
}

void MyGameState_LevelEditor::_RedoLastChange()
{
	DBGOUT("");
}

void MyGameState_LevelEditor::_SaveEditsToTape()
{
	SON::SaveToFile(level_data, default_save_file.c_str());
	loaded_tape.SaveToFile("R:/edit_tape.son");
}

ERet MyGameState_LevelEditor::_LoadEditsFromTape()
{
	mxDO(SON::LoadFromFile(
		default_save_file.c_str()
		, level_data
		, MemoryHeaps::temporary())
		);
	//mxDO(loaded_tape.LoadFromFile(LEVEL_VOXEL_TAPE_FILENAME));
	return ALL_OK;
}

ERet MyGameState_LevelEditor::_ApplyLoadedEdits()
{
	mxDO(loaded_tape.ApplyEditsToWorld(
		voxel_world
		, FPSGame::Get().runtime_clump
		));
	return ALL_OK;
}

void MyGameState_LevelEditor::_RebuildChangedWorldParts(
	const AABBf& changed_region_aabb
	)
{
	voxel_world->RebuildRegion(
		AABBd::fromOther(changed_region_aabb)
		);
}
