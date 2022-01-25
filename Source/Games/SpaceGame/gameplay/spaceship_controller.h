//
#pragma once

#include "common.h"
#include "gameplay/game_entities.h"



enum EShipCameraMode
{
	/// convenient for controlling small fighter ships
	ShipCam_Follow,

	/// should be used for big heavy battleships
	ShipCam_Orbit,
};

//--------------------------------------------------------------------------------------
//	Simple model viewing camera class that rotates around the object.
//--------------------------------------------------------------------------------------
class SgFollowingCamera: public CStruct
{
	EShipCameraMode	camera_mode;

	/// 
	V3f		focus_pivot_position;

	// for relaxed camera movement
	float	focus_relaxation_radius;	// >= 0

	V3f		relaxed_focus_pivot_position;

	/// how much to pull the camera to focus position each second
	float01_t	focus_centering_factor;	// [0..1], default = 0.5


	/// offset/back-off applied in world space, > 0
	float	orbit_distance;
	float	min_allowed_orbit_distance;
	float	max_allowed_orbit_distance;


	// orbit angles, in radians
	float	yaw;
	float	pitch;
	float	rotation_speed_rad_per_sec;

	//
	bool	constrain_vertical_angle;
	float	min_vertical_angle_rad;
	float	max_vertical_angle_rad;




public:	// derived values, don't touch:
	V3f		cam_eye_pos;
	V3f		cam_up_dir;
	V3f		cam_look_dir;
	V3f		cam_right_dir;






	Q4f		desired_orient;

	V3f		desired_look_dir;
	float	desired_look_dir_weight;
	V3f		desired_right_dir;

	// +1 or -1
	float	desired_cam_height_in_up_dir;




public:
	mxDECLARE_CLASS(SgFollowingCamera,CStruct);
	mxDECLARE_REFLECTION;

	SgFollowingCamera();


	void UpdateOncePerFrame( const NwTime& game_time );

	void UpdateAnglesFromAxisInput(
		float mouse_delta_x, float mouse_delta_y
		, const NwTime& game_time
		);


	const M44f BuildViewMatrix() const;

public:
	void SetCameraMode(const EShipCameraMode new_camera_mode);

	void SetFocusPivotPosition(const V3f& new_focus_pivot_position);

	void SetMinAndMaxOrbitDistances(
		float new_min_allowed_orbit_distance,
		float new_max_allowed_orbit_distance
		);

	void SetOrbitDistance(
		float new_orbit_distance
		);

	void SetDesiredLookDirection(
		const V3f& desired_look_dir
		//const V3f& desired_right_dir
		, const float weight = 1.0f
		);

	void ZoomIn();
	void ZoomOut();

private:
	void _RecalcCameraParams();

	void OnInputEvent( const SInputEvent& _event );
};


class SgSpaceshipController: NwNonCopyable
{
public:
	SgShipHandle	_spaceship_handle;

	SgFollowingCamera	_following_camera;

public:
	SgSpaceshipController();

public:	// API

	bool IsControllingSpaceship() const;

	void AssumeControlOfSpaceship(
		const SgShip& ship_to_control
		);
	void RelinquishControlOfSpaceship();

public:
	ERet Update(
		const NwTime& game_time
		, SgGameplayMgr& gameplay
		, SgPhysicsMgr& physics_mgr
		);

	//
	ERet ProcessPlayerInput(
		const NwTime& game_time
		, const gainput::InputMap& input_map
		, float delta_yaw
		, float delta_pitch
		);
};
