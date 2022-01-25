//
#include "stdafx.h"
#pragma hdrstop

#include <Base/Math/Utils.h>	// CalcDamping
#include <Core/Util/Tweakable.h>


#define XM_NO_OPERATOR_OVERLOADS
#include <XNAMath/xnamath.h>
////
//#define B3_USE_SSE
//#define B3_USE_SSE_IN_API
////#define B3_NO_SIMD_OPERATOR_OVERLOADS
//
//#include <Bullet3Common/shared/b3Float4.h>
//#include <Bullet3Common/shared/b3Quat.h>
//#include <Physics/Bullet_Wrapper.h>


#include "gameplay/spaceship_controller.h"
#include "game_app.h"
#include <gainput/gainput.h>




/*
-----------------------------------------------------------------------------
	SgFollowingCamera

	based on https://catlikecoding.com/unity/tutorials/movement/orbit-camera/
-----------------------------------------------------------------------------
*/
static const float DEFAULT_ORBIT_DISTANCE = 10.0f;

mxDEFINE_CLASS(SgFollowingCamera);
mxBEGIN_REFLECTION(SgFollowingCamera)
	mxMEMBER_FIELD(focus_pivot_position),
	mxMEMBER_FIELD(focus_relaxation_radius),
	mxMEMBER_FIELD(relaxed_focus_pivot_position),
	mxMEMBER_FIELD(focus_centering_factor),

	mxMEMBER_FIELD(orbit_distance),
	mxMEMBER_FIELD(min_allowed_orbit_distance),
	mxMEMBER_FIELD(max_allowed_orbit_distance),

	mxMEMBER_FIELD(yaw),
	mxMEMBER_FIELD(pitch),
	mxMEMBER_FIELD(rotation_speed_rad_per_sec),

	mxMEMBER_FIELD(constrain_vertical_angle),
	mxMEMBER_FIELD(min_vertical_angle_rad),
	mxMEMBER_FIELD(max_vertical_angle_rad),

	mxMEMBER_FIELD(cam_eye_pos),
	mxMEMBER_FIELD(cam_up_dir),
	mxMEMBER_FIELD(cam_look_dir),
	mxMEMBER_FIELD(cam_right_dir),
mxEND_REFLECTION

SgFollowingCamera::SgFollowingCamera()
{
	camera_mode = ShipCam_Follow;

	focus_pivot_position = CV3f(0);
	focus_relaxation_radius = 0.0;// 1.0f;
	relaxed_focus_pivot_position = CV3f(0);
	focus_centering_factor = 0.5f;

	orbit_distance = DEFAULT_ORBIT_DISTANCE;
	min_allowed_orbit_distance = 1.0f;
	max_allowed_orbit_distance = 1000.0f;

	yaw = 0;
	pitch = 0;

	rotation_speed_rad_per_sec = DEG2RAD(90);

	constrain_vertical_angle = true;
	min_vertical_angle_rad = DEG2RAD(-80.f);
	max_vertical_angle_rad = DEG2RAD(80.f);


	desired_orient = Q4f::identity();
	desired_look_dir = V3_FORWARD;
	desired_look_dir_weight = 1.0f;
	desired_cam_height_in_up_dir = 1;


	cam_eye_pos = CV3f(0);
	cam_up_dir = V3_UP;
	cam_look_dir = V3_FORWARD;
	cam_right_dir = V3_RIGHT;
}

void SgFollowingCamera::UpdateOncePerFrame( const NwTime& game_time )
{
	switch(camera_mode)
	{
	case ShipCam_Follow:
		{
			const Q4f	look_rotation_quat = desired_orient.inverse();//Q4f::identity();// desired_orient;

			const V3f rotated_look_direction = look_rotation_quat.rotateVector(
				V3_FORWARD
				);

			const V3f rotated_up_direction = look_rotation_quat.rotateVector(
				V3_UP
				);
			//const V3f rotated_look_direction = desired_look_dir;

			//nwTWEAKABLE_CONST(float, CAM_HEIGHT, 1.0f);

			const V3f look_position = relaxed_focus_pivot_position
				- rotated_look_direction * orbit_distance
				+ rotated_up_direction * desired_cam_height_in_up_dir
				;

			//LogStream(LL_Info),"rotated_look_direction: ",rotated_look_direction;


			cam_eye_pos = look_position;
			cam_look_dir = rotated_look_direction;
			cam_up_dir = look_rotation_quat.rotateVector(
				V3_UP
				);
		}
		break;

	case ShipCam_Orbit:
		{
			const Q4f	look_rotation_quat = Q4f::FromAngles_YawPitchRoll(yaw, pitch);
			const V3f rotated_look_direction = look_rotation_quat.rotateVector(
				V3_FORWARD
				);

			const V3f look_position = relaxed_focus_pivot_position - rotated_look_direction * orbit_distance;

			cam_eye_pos = look_position;
			cam_look_dir = rotated_look_direction;
			cam_up_dir = look_rotation_quat.rotateVector(
				V3_UP
				);
		}
		break;
	}

	//
	if(focus_relaxation_radius > 0)
	{
		const float distance_between_actual_focus_and_relaxed_focus = (relaxed_focus_pivot_position - focus_pivot_position).length();

		float focus_relaxation_lerp_factor = 1;
		if( distance_between_actual_focus_and_relaxed_focus > 0.01f && focus_centering_factor > 0 )
		{
			focus_relaxation_lerp_factor = mmPow(
				1.0f - focus_centering_factor,
				game_time.real.delta_seconds
				);
		}

		if(distance_between_actual_focus_and_relaxed_focus > focus_relaxation_radius)
		{
			focus_relaxation_lerp_factor = minf(
				focus_relaxation_lerp_factor,
				focus_relaxation_radius / distance_between_actual_focus_and_relaxed_focus
				);
		}

		relaxed_focus_pivot_position = V3_Lerp(
			focus_pivot_position,
			relaxed_focus_pivot_position,
			focus_relaxation_lerp_factor
			);

		//LogStream(LL_Info)
		//	,"relaxed_focus_pivot_position: ",relaxed_focus_pivot_position
		//	,", focus_relaxation_lerp_factor: ",focus_relaxation_lerp_factor
		//	;
	}
	else
	{
		relaxed_focus_pivot_position = focus_pivot_position;
	}

	//
	_RecalcCameraParams();
}

void SgFollowingCamera::UpdateAnglesFromAxisInput(
					  float mouse_delta_x, float mouse_delta_y
					  , const NwTime& game_time
					  )
{
	const float e = 0.001f;
	if (mouse_delta_x < -e || mouse_delta_x > e || mouse_delta_y < -e || mouse_delta_y > e)
	{
		float input_scale = game_time.real.delta_seconds * rotation_speed_rad_per_sec;
		yaw += mouse_delta_x * input_scale;
		pitch += mouse_delta_y * input_scale;
	}

	//
	if (yaw < 0) {
		yaw += mxTWO_PI;
	}
	else if (yaw >= mxTWO_PI) {
		yaw -= mxTWO_PI;
	}

	//
	if(constrain_vertical_angle) {
		pitch = clampf(pitch, min_vertical_angle_rad, max_vertical_angle_rad);
	}
}

void SgFollowingCamera::_RecalcCameraParams()
{
	// calculate right and up vectors for building a view matrix
	cam_right_dir = V3_Cross( cam_look_dir, cam_up_dir );
	float len = V3_Length( cam_right_dir );
	// prevent singularities when light direction is nearly parallel to 'up' direction
	if( len < 1e-3f ) {
		cam_right_dir = V3_RIGHT;	// light direction is too close to the UP vector
	} else {
		cam_right_dir /= len;
	}

	cam_up_dir = V3_Cross( cam_right_dir, cam_look_dir );
	cam_up_dir = V3_Normalized( cam_up_dir );
}

const M44f SgFollowingCamera::BuildViewMatrix() const
{
	return M44_BuildView(
		cam_right_dir,
		cam_look_dir,
		cam_up_dir,
		V3f::fromXYZ( cam_eye_pos )
	);
}

void SgFollowingCamera::SetCameraMode(const EShipCameraMode new_camera_mode)
{
	camera_mode = new_camera_mode;
}

void SgFollowingCamera::SetFocusPivotPosition(const V3f& new_focus_pivot_position)
{
	focus_pivot_position = new_focus_pivot_position;
}

void SgFollowingCamera::SetMinAndMaxOrbitDistances(
	float new_min_allowed_orbit_distance,
	float new_max_allowed_orbit_distance
	)
{
	min_allowed_orbit_distance = new_min_allowed_orbit_distance;
	max_allowed_orbit_distance = new_max_allowed_orbit_distance;
}

void SgFollowingCamera::SetOrbitDistance(
	float new_orbit_distance
	)
{
	orbit_distance = new_orbit_distance;
}

void SgFollowingCamera::SetDesiredLookDirection(
	const V3f& new_desired_look_dir
//	, const V3f& new_desired_right_dir
	, const float weight
	)
{
	desired_look_dir = new_desired_look_dir;
	desired_look_dir_weight = weight;
//	desired_right_dir = new_desired_right_dir;
}

void SgFollowingCamera::ZoomIn()
{
	orbit_distance *= 0.9f;
	orbit_distance = maxf(orbit_distance, min_allowed_orbit_distance);
}

void SgFollowingCamera::ZoomOut()
{
	orbit_distance *= 1.1f;
	orbit_distance = minf(orbit_distance, max_allowed_orbit_distance);
}

void SgFollowingCamera::OnInputEvent( const SInputEvent& _event )
{
	UNDONE;
#if 0
	if( _event.type == Event_MouseButtonEvent )
	{
		if( _event.mouseButton.action == IA_Pressed )
		{
			arcBall.BeginDrag(_event.mouse.x, _event.mouse.y);
		}
		if( _event.mouseButton.action == IA_Released )
		{
			arcBall.EndDrag();
		}
	}
	if( _event.type == Event_MouseCursorMoved )
	{
		arcBall.MoveDrag(_event.mouse.x, _event.mouse.y);
	}
#endif
}





SgSpaceshipController::SgSpaceshipController()
{
	_spaceship_handle.setNil();

	_following_camera.SetMinAndMaxOrbitDistances(
		1.0f,
		10000.0f
		);
}

bool SgSpaceshipController::IsControllingSpaceship() const
{
	return _spaceship_handle.isValid();
}

void SgSpaceshipController::AssumeControlOfSpaceship(
	const SgShip& ship_to_control
	)
{
	mxASSERT(ship_to_control.my_handle.isValid());
	_spaceship_handle = ship_to_control.my_handle;
	DBGOUT("Assumed control of player_ship %d", ship_to_control.my_handle.id);

	//
	EShipCameraMode ship_camera_mode = ShipCam_Follow;

	switch(ship_to_control.type)
	{
	case obj_freighter:
	case obj_heavy_battleship_ship:
		ship_camera_mode = ShipCam_Orbit;
		break;

	default:
		break;
	}

	_following_camera.SetCameraMode(ship_camera_mode);

	//
	NwInputSystemI::Get().UnpressAllButtons();
}

void SgSpaceshipController::RelinquishControlOfSpaceship()
{
	_spaceship_handle.setNil();
}

ERet SgSpaceshipController::Update(
								   const NwTime& game_time
								   , SgGameplayMgr& gameplay
								   , SgPhysicsMgr& physics_mgr
								   )
{
	const SgShip* player_ship = gameplay.FindSpaceship(_spaceship_handle);
	if(!player_ship) {
		// the ship must have been destroyed
		_spaceship_handle.setNil();
		return ERR_OBJECT_NOT_FOUND;
	}

	////
	//Vector4	phys_ent_position;
	//physics_mgr.GetRigidBodyCenterOfMassPosition(
	//	&phys_ent_position
	//	, player_ship->rigid_body_handle
	//	);


	//
	_following_camera.UpdateOncePerFrame(
		game_time
		);

	//
	_following_camera.SetFocusPivotPosition(
		//Vector4_As_V3(phys_ent_position)
		physics_mgr.GetRigidBodyPredictedPosition(player_ship->rigid_body_handle, game_time)
		);

	return ALL_OK;
}

ERet SgSpaceshipController::ProcessPlayerInput(
	const NwTime& game_time
	, const gainput::InputMap& input_map
	, float delta_yaw
	, float delta_pitch
	)
{
	mxENSURE(this->IsControllingSpaceship(), ERR_INVALID_FUNCTION_CALL, "");

	//
	_following_camera.UpdateAnglesFromAxisInput(
		delta_yaw, delta_pitch, game_time
		);

	//
	SgGameApp& game_app = SgGameApp::Get();

	SgGameplayMgr& gameplay = game_app.gameplay;

	const SgInputMgr& input_mgr = game_app.input_mgr;
	
	SgPhysicsMgr& physics_mgr = game_app.physics_mgr;
	
	//
	const SgShip* player_ship = gameplay.FindSpaceship(_spaceship_handle);
	if(!player_ship) {
		// the ship has been destroyed
		_spaceship_handle.setNil();
		return ERR_OBJECT_NOT_FOUND;
	}

	//
	const RigidBodyHandle rb_handle = player_ship->rigid_body_handle;

	M44f	spaceship_world_matrix;
	physics_mgr.GetRigidBodyTransform(
		spaceship_world_matrix
		, rb_handle
		);

	const V3f& spaceship_forward_vec = spaceship_world_matrix.v1.asVec3();
	const V3f& spaceship_right_vec = spaceship_world_matrix.v0.asVec3();
	const V3f& spaceship_up_vec = spaceship_world_matrix.v2.asVec3();


#if 0 //MX_DEVELOPER
	TbPrimitiveBatcher& dbgdraw = SgGameApp::Get().renderer._render_system->_debug_renderer;
	dbgdraw.drawFrame(
		spaceship_world_matrix.GetTranslation()
		, spaceship_right_vec
		, spaceship_forward_vec
		, spaceship_up_vec
		, 4.0f
		);

	dbgdraw.DrawLine(
		spaceship_world_matrix.GetTranslation()
		, spaceship_world_matrix.GetTranslation() + _following_camera.cam_look_dir * 100.0f
		);

#endif


	_following_camera.SetDesiredLookDirection(spaceship_forward_vec);

	

	//
	SgRBInstanceData	rb_inst_data;
	physics_mgr.GetRigidBodyPosAndQuat(
		rb_handle
		, rb_inst_data
		);
	_following_camera.desired_orient.q = rb_inst_data.quat;




	//
	const InputState& input_state = input_mgr._input_system->getState();

	//
	const SgShipDef& ship_def = gameplay.GetShipDef(player_ship->type);

	nwTWEAKABLE_CONST(float, ShipLinearThrustImpulse, 2.0f);
	nwTWEAKABLE_CONST(float, ShipAngularThrustImpulse, 1.0f);


	float	engine_thrust = ShipLinearThrustImpulse * ship_def.linear_thrust_acceleration * ship_def.mass;

	//
	const bool is_right_mouse_button_held
		= (input_state.mouse.held_buttons_mask & BIT(MouseButton_Right)) != 0
		;


	//
	const bool is_accelerating_foward
		= input_mgr.GetBool(eGA_Spaceship_AccelerateForward)
		||
		is_right_mouse_button_held
		;

	const bool is_boosting
		= ((input_state.modifiers & KeyModifier_Shift) != 0)
		||
		is_right_mouse_button_held
		;

	//
	if(is_boosting) {
		engine_thrust *= 2.0f;
	}




	//
	float	turn_thrust = ShipAngularThrustImpulse * ship_def.angular_turn_acceleration * ship_def.mass;


	//
	CV3f	linear_acceleration(0);
	CV3f	angular_acceleration(0);




	//
	if(is_accelerating_foward)
	{
		linear_acceleration += spaceship_forward_vec * engine_thrust;
	}


	//
	if(input_mgr.GetBool(eGA_Spaceship_Brake))
	{
		const float velocity_damping = CalcDamping( 0.98f, game_time.real.delta_seconds );
		physics_mgr.DampLinearVelocity(
			rb_handle
			, velocity_damping
			);
		physics_mgr.DampAngularVelocity(
			rb_handle
			, velocity_damping
			);
	}


	//
	if(input_mgr.GetBool(eGA_Spaceship_YawLeft)) {
		// CCW, right-hand rule
		angular_acceleration.z += turn_thrust;
	}
	if(input_mgr.GetBool(eGA_Spaceship_YawRight)) {
		angular_acceleration.z -= turn_thrust;
	}

	//
	if(input_mgr.GetBool(eGA_Spaceship_PitchUp)) {
		angular_acceleration.x += turn_thrust;
	}
	if(input_mgr.GetBool(eGA_Spaceship_PitchDown)) {
		angular_acceleration.x -= turn_thrust;
	}

	if(input_mgr.GetBool(eGA_Spaceship_RollLeft)) {
		angular_acceleration.y -= turn_thrust * 2.0f;
	}
	if(input_mgr.GetBool(eGA_Spaceship_RollRight)) {
		angular_acceleration.y += turn_thrust * 2.0f;
	}


	//
	nwTWEAKABLE_CONST(float, MOUSE_TURN_DELTA_SCALE, 0.5f);

	// '-' for nicer controls
	angular_acceleration.z += -delta_yaw * MOUSE_TURN_DELTA_SCALE;
	angular_acceleration.x += -delta_pitch * MOUSE_TURN_DELTA_SCALE;


#if 0
	if(input_mgr.GetBool(eGA_Spaceship_AccelerateBackwards)) {
		linear_acceleration += -spaceship_forward_vec * engine_thrust;
	}
	if(input_mgr.GetBool(eGA_Spaceship_AccelerateLeft)) {
		linear_acceleration += -spaceship_right_vec * engine_thrust;
	}
	if(input_mgr.GetBool(eGA_Spaceship_AccelerateRight)) {
		linear_acceleration += spaceship_right_vec * engine_thrust;
	}


	if(input_mgr.GetBool(eGA_Spaceship_AccelerateUp)) {
		linear_acceleration += spaceship_up_vec * engine_thrust;
	}
	if(input_mgr.GetBool(eGA_Spaceship_AccelerateDown)) {
		linear_acceleration -= spaceship_up_vec * engine_thrust;
	}
#endif


	if(
		input_mgr.GetBool(eGA_Spaceship_FireCannon)
		||
		(input_state.mouse.held_buttons_mask & BIT(MouseButton_Left))
		)
	{
		gameplay.ShootCannonOfPlayerControlledShip(
			_spaceship_handle
			, spaceship_world_matrix.v3.asVec3()
			, spaceship_forward_vec
			, spaceship_right_vec
			, spaceship_up_vec
			, game_time
			, Vector4_As_V3(physics_mgr.GetRigidBodyLinearVelocity(rb_handle))
			, physics_mgr
			, game_app.sound
			);
	}

//#if MX_DEVELOPER
//	if(input_mgr.GetBool(eGA_Spaceship_Debug_DecelerateToStop)) {
//		physics_mgr.Dbg_ZeroOutVelocity(
//			rb_handle
//			);
//		physics_mgr.Dbg_ResetOrientation(
//			rb_handle
//			);
//	}
//#endif


	//
	if(linear_acceleration.lengthSquared() > 0)
	{
		physics_mgr.ApplyCentralImpulse(
			rb_handle
			, linear_acceleration * game_time.real.delta_seconds
			);
	}

	//
	physics_mgr.SetLinearVelocityDirection(
		rb_handle
		, spaceship_forward_vec
		);

	//
	if(angular_acceleration.lengthSquared() > 0)
	{
		physics_mgr.AddAngularVelocity(
			rb_handle
			, angular_acceleration * game_time.real.delta_seconds
			);
	}
	

	//
#if 0
	//SVD: TODO: why doesn't it work!!!
	if(input_mgr._mouse->GetBool(gainput::MouseButtonWheelUp)) {
		_orbit_camera.ZoomOut();
	}
	if(input_mgr._mouse->GetBool(gainput::MouseButtonWheelDown)) {
		_orbit_camera.ZoomIn();
	}

#else

	const MouseState& mouse = input_mgr._input_system->getState().mouse;
	if(mouse.wheel_scroll < 0) {
		_following_camera.ZoomOut();
	}
	if(mouse.wheel_scroll > 0) {
		_following_camera.ZoomIn();
	}

#endif

	return ALL_OK;
}
