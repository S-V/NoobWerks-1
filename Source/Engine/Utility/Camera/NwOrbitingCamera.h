#pragma once

#include <Core/Client.h>
#include <Core/VectorMath.h>


//--------------------------------------------------------------------------------------
//	Simple model viewing camera class that rotates around the object.
//--------------------------------------------------------------------------------------
class NwOrbitingCamera: public CStruct
{
public:
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


	// derived values:
	V3f		cam_eye_pos;
	V3f		cam_up_dir;
	V3f		cam_look_dir;
	V3f		cam_right_dir;

public:
	mxDECLARE_CLASS(NwOrbitingCamera,CStruct);
	mxDECLARE_REFLECTION;

	NwOrbitingCamera();


	void UpdateOncePerFrame( const NwTime& game_time );

	void UpdateAnglesFromAxisInput(
		float mouse_delta_x, float mouse_delta_y
		, const NwTime& game_time
		);


	const M44f BuildViewMatrix() const;

	void SetFocusPivotPosition(const V3f& new_focus_pivot_position);

	void ZoomIn();
	void ZoomOut();

private:
	void _RecalcCameraParams();


	//void Initialize( int width, int height );	// must be done everytime after viewport resizing
	//void BeginDrag( int x, int y );	// start the rotation (pass current mouse position)
	//void MoveDrag( int x, int y );	// continue the rotation (pass current mouse position)
	//void EndDrag();					// stop the rotation 

	//void SetOrigin( const glm::vec3& center );
	//void SetRadius( float radius );

	void OnInputEvent( const SInputEvent& _event );
};



/*
-----------------------------------------------------------------------------
	ArcBall
	Arcball is a method to manipulate and rotate objects in 3D intuitively.
	The motivation behind the trackball (aka arcball) is to provide an intuitive user interface for complex 3D
	object rotation via a simple, virtual sphere - the screen space analogy to the familiar input device bearing
	the same name.
	The sphere is a good choice for a virtual trackball because it makes a good enclosure for most any object;
	and its surface is smooth and continuous, which is important in the generation of smooth rotations in response
	to smooth mouse movements.
	Any smooth, continuous shape, however, could be used, so long as points on its surface can be generated
	in a consistent way.
-----------------------------------------------------------------------------
*/
class ArcBall: public CStruct
{
public:
	mxDECLARE_CLASS(ArcBall,CStruct);
	mxDECLARE_REFLECTION;

	ArcBall();

	void Initialize( int width, int height );	//!< must be done after resizing the viewport

	void BeginDrag( int x, int y );	//!< start the rotation (pass current mouse position)
	void MoveDrag( int x, int y );	//!< continue the rotation (pass current mouse position)
	void EndDrag();					//!< stop the rotation 

	Vector4 GetQuaternion() const;
	M44f GetRotationMatrix() const;

private:
	V3f GetArcballVector( int screenX, int screenY ) const;

private:
	V3f			m_downPt;	//!< saved click vector (starting point of rotation arc)
	V3f			m_currPt;	//!< saved click vector (current point of rotation arc)
	float		m_scaleX;	//!< for normalizing screen-space X coordinate
	float		m_scaleY;	//!< for normalizing screen-space Y coordinate
	//float		m_radius;	//!< arc ball's radius in screen coords (default = 1.0f)
	bool		m_isDown;	//!< is the user dragging the mouse?
	Vector4		m_qDown;	//!< Quaternion before button down
	Vector4		m_qNow;		//!< Composite quaternion for current drag
};

