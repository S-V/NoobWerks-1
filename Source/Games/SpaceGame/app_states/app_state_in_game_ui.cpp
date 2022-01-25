//
#include "stdafx.h"
#pragma hdrstop

#include <nativefiledialog/include/nfd.h>

#include <Core/Serialization/Text/TxTSerializers.h>

#include <Rendering/Public/Core/RenderPipeline.h>
#include <Rendering/Public/Globals.h>
#include <Utility/GUI/ImGui/ImGUI_Renderer.h>

#include <Voxels/private/debug/vxp_debug.h>
#include <VoxelsSupport/VX_DebugHUD_ImGui.h>

#include "game_app.h"
#include "app_states/app_states.h"
#include "app_states/game_state_debug_hud.h"


/*
-----------------------------------------------------------------------------
	SgAppState_InGameUI
-----------------------------------------------------------------------------
*/
SgAppState_InGameUI::SgAppState_InGameUI()
{
	this->DbgSetName("InGameUI");
}

EStateRunResult SgAppState_InGameUI::handleInput(
	NwAppI* game_app
	, const NwTime& game_time
)
{
	SgGameApp& game = *checked_cast< SgGameApp* >( game_app );

	if( game.input_mgr.wasButtonDown( eGA_Exit ) )
	{
#if 0
		game.state_mgr.popState();
#else
		game.state_mgr.pushState(
			&game.states->escape_menu
			);
#endif
		return StopFurtherProcessing;
	}

	//
	if( game.input_mgr.wasButtonDown( eGA_Show_Debug_HUD ) )
	{
		game.state_mgr.popState();
		return StopFurtherProcessing;
	}



	//
	const InputState& input_state = NwInputSystemI::Get().getState();
	const bool need_to_go_back_to_main_game
		= (input_state.mouse.held_buttons_mask != 0)
		|| input_state.keyboard.held.Get(KEY_Enter)
		|| input_state.keyboard.held.Get(KEY_Space)
		;

	if( need_to_go_back_to_main_game )
	{
		game.state_mgr.popState();
		return StopFurtherProcessing;
	}


	// debug HUD blocks main app UI
	return StopFurtherProcessing;
}

void SgAppState_InGameUI::Update(
								const NwTime& game_time
								, NwAppI* game_app
								)
{
	ImGui_Renderer::UpdateInputs();
}

const V3f GetRayDirectionFromScreenCoords(
	int pos_x_in_screen_space, int pos_y_in_screen_space
	, const NwCameraView& camera_view
	)
{
	// Convert x, y from screen to normalized device coordinates [-1:1].
	V2f	NDC_coords;

	// [0..W] -> [-1..+1]
	NDC_coords.x = ((float)pos_x_in_screen_space / camera_view.screen_width) * 2.f - 1.f;

	// [0..H] -> [+1..-1]
	NDC_coords.y = 1.f - ((float)pos_y_in_screen_space / camera_view.screen_height) * 2.f;
	//NDC_coords.y = -1.f * ( (float)pos_y_in_screen_space - camera_view.screen_height*0.5f) / (camera_view.screen_height*0.5f);

	//
#if 0
	V3f	pt_on_near_plane
		= camera_view.eye_pos
		+ camera_view.right_dir * NDC_coords.x
		+ camera_view.getUpDir() * NDC_coords.y
		+ camera_view.look_dir
		;
	V3f	pt_on_far_plane
		= camera_view.eye_pos
		+ camera_view.right_dir * NDC_coords.x
		+ camera_view.getUpDir() * NDC_coords.y
		+ camera_view.look_dir * 100.f
		;

	return (camera_view.eye_pos - pt_on_near_plane).normalized();


#else

	CV4f	org_pt(NDC_coords.x, 0, NDC_coords.y, 1);
	CV4f	far_pt(NDC_coords.x, 1, NDC_coords.y, 1);

	const V4f org_h = M44_Project(
		camera_view.inverse_view_projection_matrix
		, org_pt
		);
	const V4f far_h = M44_Project(
		camera_view.inverse_view_projection_matrix
		, far_pt
		);

	return (far_h.asVec3() - org_h.asVec3()).normalized();

#endif
}



const V3f GetPointOnImagePlaneFromScreenCoords(
	int pos_x_in_screen_space, int pos_y_in_screen_space
	, const NwCameraView& camera_view
	)
{
	// Convert x, y from screen to normalized device coordinates [-1:1].
	V2f	NDC_coords;

	// [0..W] -> [-1..+1]
	NDC_coords.x = ((float)pos_x_in_screen_space / camera_view.screen_width) * 2.f - 1.f;

	// [0..H] -> [+1..-1]
	NDC_coords.y = 1.f - ((float)pos_y_in_screen_space / camera_view.screen_height) * 2.f;

	//
	V3f	pt_on_near_plane
		= camera_view.eye_pos
		+ camera_view.right_dir * NDC_coords.x
		+ camera_view.getUpDir() * NDC_coords.y
		+ camera_view.look_dir * camera_view.near_clip
		;
	return pt_on_near_plane;
}



EStateRunResult SgAppState_InGameUI::DrawScene(
	NwAppI* p_game_app
	, const NwTime& game_time
	)
{
	SgGameApp& game_app = SgGameApp::Get();

	//
	SgRenderer& renderer = game_app.renderer;
	const NwCameraView& camera_view = renderer.scene_view;


	//
	const MouseState& mouse_state = NwInputSystemI::Get().getState().mouse;


	//
	const V3f point_on_img_plane = GetPointOnImagePlaneFromScreenCoords(
		mouse_state.x, mouse_state.y
		, camera_view
		);

	//
	const V3f ray_dir0 = GetRayDirectionFromScreenCoords(
		mouse_state.x, mouse_state.y
		, camera_view
		);

	//
	const V3f pos_in_front_of_camera = camera_view.eye_pos + ray_dir0 * 5.f;
	//const V3f pos_in_front_of_camera = camera_view.eye_pos + camera_view.look_dir * 5.f;

#if 0
static int cnt;
cnt++;
if(cnt%16==0)
{
	LogStream(LL_Trace),"	!!!!";
	//LogStream(LL_Trace),"ray_dir.eye_pos: ",ray_dir;
	LogStream(LL_Trace),"point_on_img_plane: ",point_on_img_plane;
	//LogStream(LL_Trace),"pos_in_front_of_camera: ",pos_in_front_of_camera;
	//LogStream(LL_Trace),"camera_view.eye_pos: ",camera_view.eye_pos;
	//LogStream(LL_Trace),"camera_view.eye_pos_ws: ",camera_view.eye_pos_in_world_space;
}
#endif
	//
TbPrimitiveBatcher& prim_batcher = game_app.renderer._render_system->_debug_renderer;


	//
#if 1
	CulledObjectID	hit_obj;
	TempNodeID		hit_node_id;

	const bool ray_hit_anything = game_app.physics_mgr._aabb_tree->CastRay(
		point_on_img_plane
		, camera_view.look_dir
		, hit_obj
		, &hit_node_id
		);

	if(ray_hit_anything)
	{
DBGOUT("HIT!!!!");

		SimdAabb	hit_node_aabb;
		game_app.physics_mgr._aabb_tree->GetNodeAABB(
			hit_node_id
			, hit_node_aabb
			);

		prim_batcher.DrawAABB(
			SimdAabb_Get_AABBf(hit_node_aabb)
			, RGBAf::GREEN
			);
	}
#endif

	return ContinueFurtherProcessing;
}

void SgAppState_InGameUI::DrawUI(
								  const NwTime& game_time
								  , NwAppI* game_app
								  )
{
	//
	//_draw_actual_ImGui( delta_time_in_seconds, game_app );
}

void SgAppState_InGameUI::_draw_actual_ImGui(
	const NwTime& game_time
	, NwAppI* game_app
	)
{
	SgGameApp* app = checked_cast< SgGameApp* >( game_app );

	SgAppSettings & game_settings = app->settings;

	ImGui::Button("Clear Debug Lines");
}
