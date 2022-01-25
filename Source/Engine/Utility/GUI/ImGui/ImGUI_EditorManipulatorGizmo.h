#pragma once


namespace Rendering
{
	struct NwCameraView;
}

struct InputState;
struct ImGuiIO;


namespace GizmoOperation
{
	enum Enum
	{
		Translate,
		Scale,
		Rotate,
		Bounds,
	};
};
mxDECLARE_ENUM( GizmoOperation::Enum, U8, GizmoOperationT );

namespace GizmoOperation
{
	bool handle_InputEvent_to_change_GizmoOperation(
		GizmoOperationT * gizmo_op
		, const InputState& input_state
		);
}//namespace GizmoOperation


/*
======================================================================
	EditorManipulator
======================================================================
*/
struct EditorManipulator: NonCopyable
{
	bool	_isGizmoEnabled;

	M44f	gizmo_delta_matrix;

	GizmoOperationT		gizmo_operation;

public:
	EditorManipulator();


	void setGizmoEnabled( bool enable_gizmo );
	bool isGizmoEnabled() const { return _isGizmoEnabled; }

	void setGizmoOperation( GizmoOperation::Enum op );

public:

	/// Call right after ImGui_XXXX_NewFrame()!
	void beginDrawing();

	void drawGizmoAndManipulate(
		M44f & gizmo_matrix
		, const Rendering::NwCameraView& camera_view
		, ImGuiIO& imGui_IO
		);
};
