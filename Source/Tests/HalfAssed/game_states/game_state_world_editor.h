// Voxel-based Level editor.
#pragma once

#include <Utility/GameFramework/NwGameStateManager.h>
#include <Utility/GUI/ImGui/ImGUI_EditorManipulatorGizmo.h>

#include <BlobTree/BlobTree.h>

#include "game_world/game_level.h"
#include "game_forward_declarations.h"
#include "game_settings.h"


struct EditingOperationInProgress: VoxelEditorSettings
{
	bool				is_valid;

	//
	EditorManipulator	gizmo_manipulator;

public:
	EditingOperationInProgress();

	ERet DrawShape(
		MyGameRenderer& game_renderer
		, const MyGameSettings& game_settings
		, NwClump& storage_clump
		);

	ERet Apply(
		SDF::Tape &blob_tree
		, VX5::WorldI* voxel_world
		, NwClump& storage_clump
		, AABBf *changed_region_aabb_ = nil
		);
};

/*
-----------------------------------------------------------------------------
	MyGameState_LevelEditor
-----------------------------------------------------------------------------
*/
class MyGameState_LevelEditor: public NwGameStateI
{
	MyLevelData	level_data;

	FilePathStringT		default_save_file;

	char	npc_spawn_point_comment[32];

	SDF::Tape	loaded_tape;

	TPtr< VX5::WorldI >	voxel_world;

	//
	EditingOperationInProgress	edit_op_in_progress;

public:
	MyGameState_LevelEditor();

	ERet Initialize(
		const MyGameSettings& game_settings
		);

	void Shutdown();


	void SetVoxelWorld(VX5::WorldI* voxel_world_ptr)
	{
		voxel_world = voxel_world_ptr;
	}

	virtual void tick(
		float delta_time_in_seconds
		, NwGameI* game_app
		) override;

	virtual EStateRunResult handleInput(
		NwGameI* game_app
		, const NwTime& game_time
		) override;

	//
	virtual bool pinsMouseCursorToCenter() const override
	{return false;}

	virtual bool hidesMouseCursor() const
	{return false;}


	/// control the camera
	virtual bool allowsFirstPersonCameraToReceiveInput() const;

	/// don't fire player weapons
	virtual bool AllowPlayerToReceiveInput() const
	{return false;}

	virtual bool ShouldRenderPlayerWeapon() const
	{return false;}


	virtual EStateRunResult drawScene(
		NwGameI* game_app
		, const NwTime& game_time
		) override;

	virtual void drawUI(
		float delta_time_in_seconds
		, NwGameI* game_app
		) override;

private:
	void _StartEditingWithCurrentOp();
	ERet _ApplyCurrentOp();
	void _CancelCurrentOp();

	void _UndoLastChange();
	void _RedoLastChange();

	void _SaveEditsToTape();
	ERet _LoadEditsFromTape();
	ERet _ApplyLoadedEdits();

private:
	void _RebuildChangedWorldParts(
		const AABBf& changed_region_aabb
		);

private:
	void _ImGui_DrawPlacementEditor();

	void _ImGui_DrawShapeUI();
	void _ImGui_DrawEditOperationUI();

private:
	//void _DrawInGameGUI(
	//	const FPSGame& game
	//	, const UInt2 window_size
	//	, nk_context* ctx
	//	);
};
