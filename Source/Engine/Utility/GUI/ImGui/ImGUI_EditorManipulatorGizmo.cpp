#include <Base/Base.h>
#pragma hdrstop

//
#include <ImGui/imgui.h>
//#include <ImGui/imgui_internal.h>
#include <ImGuizmo/ImGuizmo.h>
//
#include <Engine/WindowsDriver.h>
//
#include <Rendering/Public/Scene/CameraView.h>	// NwCameraView
//
#include <Utility/GUI/ImGui/ImGUI_EditorManipulatorGizmo.h>


mxBEGIN_REFLECT_ENUM( GizmoOperationT )
	mxREFLECT_ENUM_ITEM( Translate, GizmoOperation::Translate ),
	mxREFLECT_ENUM_ITEM( Scale,		GizmoOperation::Scale ),
	mxREFLECT_ENUM_ITEM( Rotate,	GizmoOperation::Rotate ),
	mxREFLECT_ENUM_ITEM( Bounds,	GizmoOperation::Bounds ),
mxEND_REFLECT_ENUM

namespace GizmoOperation
{
	bool handle_InputEvent_to_change_GizmoOperation(
		GizmoOperationT * gizmo_op
		, const InputState& input_state
		)
	{
		if( input_state.modifiers & KeyModifier_Ctrl )
		{
			if( input_state.keyboard.held[KEY_T] )
			{
				*gizmo_op = GizmoOperation::Translate;
				return true;
			}
			if( input_state.keyboard.held[KEY_R] )
			{
				*gizmo_op = GizmoOperation::Rotate;
				return true;
			}
			if( input_state.keyboard.held[KEY_S] )
			{
				*gizmo_op = GizmoOperation::Scale;
				return true;
			}
		}

		return false;
	}
}//namespace GizmoOperation

EditorManipulator::EditorManipulator()
{
	_isGizmoEnabled = false;

	gizmo_delta_matrix = M44_Identity();

	gizmo_operation = GizmoOperation::Translate;
}

void EditorManipulator::setGizmoEnabled( bool enable_gizmo )
{
	_isGizmoEnabled = enable_gizmo;
}

void EditorManipulator::setGizmoOperation( GizmoOperation::Enum op )
{
	gizmo_operation = op;
}

void EditorManipulator::beginDrawing()
{
	//
	ImGuizmo::BeginFrame();
}

static ImGuizmo::OPERATION ToImGuizmoOp(GizmoOperation::Enum op)
{
	switch( op )
	{
	case GizmoOperation::Translate:
		return ImGuizmo::OPERATION::TRANSLATE;

	case GizmoOperation::Scale:
		return ImGuizmo::OPERATION::SCALE;

	case GizmoOperation::Rotate:
		return ImGuizmo::OPERATION::ROTATE;

	case GizmoOperation::Bounds:
		return ImGuizmo::OPERATION::BOUNDS;

	mxNO_SWITCH_DEFAULT;
	}

	return ImGuizmo::OPERATION::TRANSLATE;
}

void EditorManipulator::drawGizmoAndManipulate(
	M44f & gizmo_matrix
	, const Rendering::NwCameraView& camera_view
	, ImGuiIO& imGui_IO
	)
{
	//
	ImGuizmo::BeginFrame();
	ImGuizmo::Enable(true);

	ImGuizmo::SetRect( 0, 0, imGui_IO.DisplaySize.x, imGui_IO.DisplaySize.y );

	//
	ImGuizmo::SetDrawlist();

	//

#if nwRENDERER_USE_REVERSED_DEPTH

	// Pass standard projection matrix to ImGuizmo,
	// because reversed projection matrix produced NaNs.

	M44f projection_matrix, inverse_projection_matrix;
	M44_ProjectionAndInverse_D3D(
		&projection_matrix, &inverse_projection_matrix
		, camera_view.half_FoV_Y*2.0f
		, camera_view.aspect_ratio
		, camera_view.near_clip
		, camera_view.far_clip
		);
#else

	// no reverse z/depth
	const M44f& projection_matrix = camera_view.projection_matrix;

#endif

	//
	ImGuizmo::Manipulate(
		camera_view.view_matrix.a
		, projection_matrix.a
		, ToImGuizmoOp(gizmo_operation)
		, ImGuizmo::MODE::WORLD
		, gizmo_matrix.a
		, gizmo_delta_matrix.a
		, NULL
		);
}
