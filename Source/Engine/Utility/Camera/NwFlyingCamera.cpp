#include <bx/debug.h>
#include <Base/Base.h>
#pragma hdrstop
#include <Engine/WindowsDriver.h>
#include <Utility/Camera/NwFlyingCamera.h>

/*
-----------------------------------------------------------------------------
	NwFlyingCamera
-----------------------------------------------------------------------------
*/
mxDEFINE_CLASS(NwFlyingCamera);
mxBEGIN_REFLECTION(NwFlyingCamera)
	mxMEMBER_FIELD(_eye_position),
	mxMEMBER_FIELD(m_upDirection),
	mxMEMBER_FIELD(m_look_direction),
	mxMEMBER_FIELD(m_rightDirection),
	
	mxMEMBER_FIELD(m_linearVelocity),
	mxMEMBER_FIELD(m_linearFriction),
	mxMEMBER_FIELD(m_angularFriction),

	mxMEMBER_FIELD(m_linAcceleration),
	mxMEMBER_FIELD(m_movementSpeed),
	mxMEMBER_FIELD(m_initialAccel),
	//mxMEMBER_FIELD(m_strafingSpeed),
	mxMEMBER_FIELD(m_rotationSpeed),

	//mxMEMBER_FIELD(m_hasLinearMomentum),
	//mxMEMBER_FIELD(m_hasAngularMomentum),
	mxMEMBER_FIELD(m_invertPitch),
	mxMEMBER_FIELD(m_invertYaw),

	mxMEMBER_FIELD(m_pitchVelocity),
	mxMEMBER_FIELD(m_yawVelocity),
	mxMEMBER_FIELD(m_pitchAngle),
	mxMEMBER_FIELD(m_yawAngle),

	mxMEMBER_FIELD(m_pitchSensitivity),
	mxMEMBER_FIELD(m_yawSensitivity),

	mxMEMBER_FIELD(m_minPitch),
	mxMEMBER_FIELD(m_maxPitch),
mxEND_REFLECTION

NwFlyingCamera::NwFlyingCamera()
{
	_eye_position		= V3d::zero();
	m_upDirection		= V3_Set(0,0,1);
	m_look_direction		= V3_Set(0,1,0);
	m_rightDirection	= V3_Set(1,0,0);

	m_linearVelocity	= V3_Zero();

	//m_initialVelocity	= 1;
	//m_maximumVelocity	= MAX_FLOAT32;
	m_initialAccel	= 10;

	m_linearFriction	= 0.25f;
	m_angularFriction	= 0.25f;

	m_linAcceleration	= CV3f(0);	// constant velocity by default
	m_movementSpeed		= 1.0f;
	//m_strafingSpeed		= 1.0f;
	m_rotationSpeed		= 0.01f;
	_dbg_speed_multiplier = 1.0f;

	//m_hasLinearMomentum	= false;
	//m_hasAngularMomentum= false;
	m_invertPitch		= false;
	m_invertYaw			= false;

	m_pitchVelocity	= 0.0f;
	m_yawVelocity	= 0.0f;
	m_pitchAngle	= 0.0f;
	m_yawAngle		= 0.0f;

	m_pitchSensitivity = 1.0f;
	m_yawSensitivity = 1.0f;

	m_minPitch = mxHALF_PI * -0.99f;
	m_maxPitch = mxHALF_PI * 0.99f;

	m_movementFlags = 0;
}

void NwFlyingCamera::setPosAndLookDir(
									const V3d& position,
									const V3f& look_direction
									)
{
	mxASSERT(V3_Length(look_direction) > 1e-5f);
	// Remember, camera points down +Y of local axes!
	V3f	upDirection = V3_Set(0,0,1);

	V3f	axisX;	// camera right vector
	V3f	axisY;	// camera forward vector
	V3f	axisZ;	// camera up vector

	axisY = V3_Normalized( look_direction );

	axisX = V3_Cross( axisY, upDirection );
	axisX = V3_Normalized( axisX );

	axisZ = V3_Cross( axisX, axisY );
	axisZ = V3_Normalized( axisZ );

	_eye_position = position;
	m_upDirection = axisZ;
	m_look_direction = axisY;
	m_rightDirection = axisX;

	// figure out the yaw/pitch of the camera

	// For any given angular displacement, there are an infinite number of Euler angle representations
	// due to Euler angle aliasing. The technique we are using here will always return 'canonical' Euler angles,
	// with heading and bank in range +/- 180° and pitch in range +/- 90°.

    // To figure out the yaw/pitch of the camera, we just need the Y basis vector.
	// An axis vector can be rotated into the direction
	// by first rotating along the world X axis by the pitch, then by the Z by the yaw.
	// The two-argument form of Arc Tangent is very useful to convert rectangular to polar coordinates.
	// Here we assume that the forward (Y axis) vector is normalized.
	m_pitchAngle = mmASin( axisY.z );	// [–Pi/2 .. +Pi/2] radians
	m_yawAngle = -mmATan2( axisY.x, axisY.y );	// [–Pi .. +Pi] radians
}

void NwFlyingCamera::setPosAndLookTarget(
	const V3d& position,
	const V3d& look_target
	)
{
	V3d look_dir = look_target - position;
	float len = look_dir.normalize();
	mxASSERT(len > 1e-3f);

	this->setPosAndLookDir(
		position,
		V3f::fromXYZ(look_dir)
		);
}

void NwFlyingCamera::Update( float delta_seconds )
{
	// Update the camera orientation.

	// Update the pitch and yaw angle based on mouse movement
	float pitchDelta = m_pitchVelocity * (delta_seconds * m_rotationSpeed);
	float yawDelta = m_yawVelocity * (delta_seconds * m_rotationSpeed);	

	//if( m_hasAngularMomentum ) {
	//	m_pitchVelocity *= (1.0f - m_angularFriction) * delta_seconds;
	//	m_yawVelocity *= (1.0f - m_angularFriction) * delta_seconds;
	//} else {
	//	m_pitchVelocity = 0.0f;
	//	m_yawVelocity = 0.0f;
	//}
	m_pitchVelocity = 0.0f;
	m_yawVelocity = 0.0f;

	// Invert pitch if requested
	if( m_invertPitch ) {
		pitchDelta = -pitchDelta;
	}
	if( m_invertYaw ) {
		yawDelta = -yawDelta;
	}
	m_pitchAngle += pitchDelta;
	m_yawAngle += yawDelta;

	// Limit pitch to straight up or straight down
//UPD: don't limit the pitch for easier debugging
//	m_pitchAngle = clampf( m_pitchAngle, m_minPitch, m_maxPitch );

	// Wrap around to prevent accumulation of too large numbers.
	m_yawAngle = mmModulo(m_yawAngle, mxTWO_PI);

	// Make a rotation matrix based on the camera's yaw and pitch.

	// An axis vector can be rotated into the direction
	// by first rotating along the world X axis by the pitch, then by the Z by the yaw.
	const M33f	R = M33_Multiply(
		M33_RotationX(-m_pitchAngle),	// negate pitch according to our conventions (inverted Y axis)
		M33_RotationZ(m_yawAngle)
	);
	mxTODO(debug matrix);
	//const M33f	R = M33_RotationPitchRollYaw( m_pitchAngle, 0.0f, m_yawAngle );

	m_rightDirection	= M33_Transform( R, V3_RIGHT );
	m_look_direction	= M33_Transform( R, V3_FORWARD );
	m_upDirection		= M33_Transform( R, V3_UP );	

	{
		// calculate right and up vectors for building a view matrix
		m_rightDirection = V3_Cross( m_look_direction, m_upDirection );
		float len = V3_Length( m_rightDirection );
		// prevent singularities when light direction is nearly parallel to 'up' direction
		if( len < 1e-3f ) {
			m_rightDirection = V3_RIGHT;	// light direction is too close to the UP vector
		} else {
			m_rightDirection /= len;
		}

		m_upDirection = V3_Cross( m_rightDirection, m_look_direction );
		m_upDirection = V3_Normalized( m_upDirection );
	}

	mxASSERT(V3_IsNormalized(m_rightDirection));
	mxASSERT(V3_IsNormalized(m_look_direction));
	mxASSERT(V3_IsNormalized(m_upDirection));
}

void NwFlyingCamera::UpdateVelocityAndPositionFromInputs( SecondsF delta_seconds )
{
	// Update the camera position.
	m_linearVelocity += m_linAcceleration * delta_seconds;	// v = v0 + a*dt
	_eye_position += V3d::fromXYZ( m_linearVelocity * delta_seconds );	// x = x0 + v*dt

	m_linAcceleration *= 0.95f;	// apply damping
	m_linearVelocity *= 0.8f;	// apply damping

	// get local-space normalized 'acceleration' from input controls
	CV3f vInputAcceleration(0);
	if( m_movementFlags & NwFlyingCamera::FORWARD ) {
		vInputAcceleration += m_look_direction;
	}
	if( m_movementFlags & NwFlyingCamera::BACKWARD ) {
		vInputAcceleration -= m_look_direction;
	}
	if( m_movementFlags & NwFlyingCamera::LEFT ) {
		vInputAcceleration -= m_rightDirection;
	}
	if( m_movementFlags & NwFlyingCamera::RIGHT ) {
		vInputAcceleration += m_rightDirection;
	}
	if( m_movementFlags & NwFlyingCamera::UP ) {
		vInputAcceleration += m_upDirection;
	}
	if( m_movementFlags & NwFlyingCamera::DOWN ) {
		vInputAcceleration -= m_upDirection;
	}

	if( vInputAcceleration.lengthSquared() > 0 )
	{
		vInputAcceleration.normalize();

		if( m_movementFlags & NwFlyingCamera::ACCELERATE ) {
			vInputAcceleration *= _dbg_speed_multiplier;
		}

		m_linAcceleration += vInputAcceleration * m_initialAccel;
		m_linearVelocity += vInputAcceleration * m_movementSpeed;		
	}
}

//void NwFlyingCamera::Forward( float units )
//{
//	m_linearVelocity += m_look_direction * units;
//}
//void NwFlyingCamera::Strafe( float units )
//{
//	m_linearVelocity += m_rightDirection * units;
//}
void NwFlyingCamera::Pitch( float units )
{
	m_pitchVelocity += units * m_pitchSensitivity;
}
//void NwFlyingCamera::Down( float units )
//{
//	m_linearVelocity -= m_upDirection * units;
//}
void NwFlyingCamera::Yaw( float units )
{
	m_yawVelocity += units * m_yawSensitivity;
}
//void NwFlyingCamera::Up( float units )
//{
//	m_linearVelocity += m_upDirection * units;
//}
M44f NwFlyingCamera::BuildViewMatrix() const
{
	return M44_BuildView(
		m_rightDirection,
		m_look_direction,
		m_upDirection,
		V3f::fromXYZ( _eye_position )
	);
}

//void NwFlyingCamera::processInput( const struct InputState& _inputState )
//{
//	setbit_cond(m_movementFlags, _inputState.keyboard.held[KEY_W], NwFlyingCamera::FORWARD);
//	setbit_cond(m_movementFlags, _inputState.keyboard.held[KEY_A], NwFlyingCamera::LEFT);
//	setbit_cond(m_movementFlags, _inputState.keyboard.held[KEY_S], NwFlyingCamera::BACKWARD);
//	setbit_cond(m_movementFlags, _inputState.keyboard.held[KEY_D], NwFlyingCamera::RIGHT);
//
//	setbit_cond(m_movementFlags, _inputState.keyboard.held[KEY_Space], NwFlyingCamera::UP);
//	setbit_cond(m_movementFlags, _inputState.keyboard.held[KEY_C], NwFlyingCamera::DOWN);
//}

void NwFlyingCamera::move_forward( GameActionID action, EInputState status, float value )
{
	//DBGOUT("move, status: %s", GetTypeOf_EInputStateT().FindString(status));
	setbit_cond(m_movementFlags, ( status == IS_Pressed || status == IS_HeldDown ), NwFlyingCamera::FORWARD);
}
void NwFlyingCamera::move_back( GameActionID action, EInputState status, float value )
{
	setbit_cond(m_movementFlags, ( status == IS_Pressed || status == IS_HeldDown ), NwFlyingCamera::BACKWARD);
}
void NwFlyingCamera::strafe_left( GameActionID action, EInputState status, float value )
{
	setbit_cond(m_movementFlags, ( status == IS_Pressed || status == IS_HeldDown ), NwFlyingCamera::LEFT);
}
void NwFlyingCamera::strafe_right( GameActionID action, EInputState status, float value )
{
	setbit_cond(m_movementFlags, ( status == IS_Pressed || status == IS_HeldDown ), NwFlyingCamera::RIGHT);
}
void NwFlyingCamera::move_up( GameActionID action, EInputState status, float value )
{
	setbit_cond(m_movementFlags, ( status == IS_Pressed || status == IS_HeldDown ), NwFlyingCamera::UP);
}
void NwFlyingCamera::move_down( GameActionID action, EInputState status, float value )
{
	setbit_cond(m_movementFlags, ( status == IS_Pressed || status == IS_HeldDown ), NwFlyingCamera::DOWN);
}
void NwFlyingCamera::rotate_pitch( GameActionID action, EInputState status, float value )
{
//	DBGOUT("rotate_pitch: %f", value);
	Pitch(value);
}
void NwFlyingCamera::rotate_yaw( GameActionID action, EInputState status, float value )
{
//	DBGOUT("rotate_yaw: %f", value);
	Yaw(value);
}
void NwFlyingCamera::accelerate( GameActionID action, EInputState status, float value )
{
	m_linAcceleration *= 2.0f;
}
void NwFlyingCamera::stop( GameActionID action, EInputState status, float value )
{
	m_linearVelocity = CV3f(0);
	m_linAcceleration = CV3f(0);
}
