#include <bx/debug.h>
#include <Base/Base.h>
#pragma hdrstop
#include <Engine/WindowsDriver.h>
#include <Utility/Camera/NwOrbitingCamera.h>


/*
-----------------------------------------------------------------------------
	NwOrbitingCamera

	based on https://catlikecoding.com/unity/tutorials/movement/orbit-camera/
-----------------------------------------------------------------------------
*/
static const float DEFAULT_ORBIT_DISTANCE = 10.0f;

mxDEFINE_CLASS(NwOrbitingCamera);
mxBEGIN_REFLECTION(NwOrbitingCamera)
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

NwOrbitingCamera::NwOrbitingCamera()
{
	focus_pivot_position = CV3f(0);
	focus_relaxation_radius = 1.0f;
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

	cam_eye_pos = CV3f(0);
	cam_up_dir = V3_UP;
	cam_look_dir = V3_FORWARD;
	cam_right_dir = V3_RIGHT;
}

void NwOrbitingCamera::UpdateOncePerFrame( const NwTime& game_time )
{
	const Q4f	look_rotation_quat = Q4f::FromAngles_YawPitchRoll(yaw, pitch);

	const V3f rotated_look_direction = look_rotation_quat.rotateVector( V3_FORWARD );
	const V3f look_position = relaxed_focus_pivot_position - rotated_look_direction * orbit_distance;

	cam_eye_pos = look_position;
	cam_look_dir = rotated_look_direction;
	cam_up_dir = look_rotation_quat.rotateVector( V3_UP );

	//
	if(focus_relaxation_radius > 0) {
		//
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

	} else {
		relaxed_focus_pivot_position = focus_pivot_position;
	}


	//
	_RecalcCameraParams();
}

void NwOrbitingCamera::UpdateAnglesFromAxisInput(
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

void NwOrbitingCamera::_RecalcCameraParams()
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

const M44f NwOrbitingCamera::BuildViewMatrix() const
{
	return M44_BuildView(
		cam_right_dir,
		cam_look_dir,
		cam_up_dir,
		V3f::fromXYZ( cam_eye_pos )
	);
}

void NwOrbitingCamera::SetFocusPivotPosition(const V3f& new_focus_pivot_position)
{
	focus_pivot_position = new_focus_pivot_position;
}

void NwOrbitingCamera::ZoomIn()
{
	orbit_distance *= 0.9f;
	orbit_distance = maxf(orbit_distance, min_allowed_orbit_distance);
}

void NwOrbitingCamera::ZoomOut()
{
	orbit_distance *= 1.1f;
	orbit_distance = minf(orbit_distance, max_allowed_orbit_distance);
}

void NwOrbitingCamera::OnInputEvent( const SInputEvent& _event )
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


/*
-----------------------------------------------------------------------------
	ArcBall
-----------------------------------------------------------------------------
*/
mxDEFINE_CLASS(ArcBall);
mxBEGIN_REFLECTION(ArcBall)
	mxMEMBER_FIELD(m_scaleX),
	mxMEMBER_FIELD(m_scaleY),
	//mxMEMBER_FIELD(m_radius),
	mxMEMBER_FIELD(m_qDown),
	mxMEMBER_FIELD(m_qNow),
mxEND_REFLECTION

ArcBall::ArcBall()
{
	m_downPt = V3_Zero();
	m_currPt = V3_Zero();
	m_scaleX = 1.0f;
	m_scaleY = 1.0f;
	//m_radius = 1.0f;
	m_isDown = false;
	m_qDown = Quaternion_Identity();
	m_qNow = Quaternion_Identity();
}

void ArcBall::Initialize( int width, int height )
{
	m_scaleX = 2.0f / width;
	m_scaleY = 2.0f / height;
}

void ArcBall::BeginDrag( int x, int y )
{
	m_qDown = m_qNow;
	m_downPt = GetArcballVector( x, y );
	m_isDown = true;
}

static Vector4 QuatFromBallPoints( const V3f& vFrom, const V3f& vTo )
{
	float	fDot = V3_Dot( vFrom, vTo );
	V3f	vPart = V3_Cross( vFrom, vTo );
	Vector4	result = Vector4_Set(
		vPart.x,
		vPart.y,
		vPart.z,
		fDot
		);
	result = Quaternion_Normalize(result);
	return result;
}

void ArcBall::MoveDrag( int x, int y )
{
	if( m_isDown )
	{
		m_currPt = GetArcballVector( x, y );
		const Vector4 qCurr = QuatFromBallPoints( m_downPt, m_currPt );
		m_qNow = Quaternion_Multiply( m_qDown, qCurr );	// qDown -> qCurr
		m_qNow = Quaternion_Normalize( m_qNow );
	}
}

void ArcBall::EndDrag()
{
	m_isDown = false;
}

Vector4 ArcBall::GetQuaternion() const
{
	return m_qNow;
}

M44f ArcBall::GetRotationMatrix() const
{
	return M44_FromQuaternion(m_qNow);
}

// The ArcBall works by mapping click coordinates in a window
// directly into the ArcBall's sphere coordinates, as if it were directly in front of you.
// To accomplish this, first we simply scale down the mouse coordinates
// from the range of [0...width), [0...height) to [-1...1], [1...-1].
//ArcBall sphere constants:
//Diameter is       2.0f
//Radius is         1.0f
//Radius squared is 1.0f

/**
 * Get a normalized vector from the center of the virtual ball O to a
 * point P on the virtual ball surface, such that P is aligned on
 * screen's (X,Y) coordinates.  If (X,Y) is too far away from the
 * sphere, return the nearest point on the virtual ball surface.
 */
V3f ArcBall::GetArcballVector( int screenX, int screenY ) const
{
	//Adjust point coords and scale down to range of [-1 ... 1]
	float x = 1.0f - (screenX * m_scaleX);
	float y = 1.0f - (screenY * m_scaleY);

	mxASSERT(x >= -1.0f && x <= 1.0f);
	mxASSERT(y >= -1.0f && y <= 1.0f);

	float lengthSquared = x * x + y * y;

	// NOTE: y and z axes are swapped according to our coordinate system:

	//If the point is mapped outside of the sphere... (length_squared > radius_squared)
	if( lengthSquared > 1.0f ) {
		//Compute a normalizing factor (radius / sqrt(length))
		float norm = mmInvSqrt(lengthSquared);
		//Return the "normalized" vector, a point on the sphere
		return V3_Set( x*norm, 0.0f, y*norm );
	} else {
		//Return a vector to a point mapped inside the sphere sqrt(radius squared - length)
		return V3_Set( x, mmSqrt(1.0f - lengthSquared), y );
	}
}
