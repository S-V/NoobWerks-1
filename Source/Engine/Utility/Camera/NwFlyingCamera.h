#pragma once

#include <Core/Client.h>
#include <Core/VectorMath.h>


// code and comments stolen from:
// https://github.com/slicedpan/NarrowPhase/blob/master/FPSCamera/FPSCamera.h
struct NwFlyingCamera: CStruct
{
	V3d	_eye_position;		// camera position; double precision to support planets
	V3f	m_upDirection;		// Z-axis of camera's world matrix (up direction)
	V3f	m_look_direction;	// Y-axis of camera's world matrix (forward direction)
	V3f	m_rightDirection;	// X-axis of camera's world matrix (right direction)

	U32	m_movementFlags;	// EMovementFlags

	V3f		m_linearVelocity;	//!< movement velocity accumulator
	float	m_linearFriction;	//!< linear damping coeff
	V3f		m_linAcceleration;	//!< acceleration accumulator
	float	m_angularFriction;	//!< angular damping coeff

	//float	m_initialVelocity;
	//float	m_maximumVelocity;
	float	m_movementSpeed;	//!< forward walking speed [units/s]
	float	m_initialAccel;
	//float	m_strafingSpeed;	//!< translation speed [units/s]
	float	m_rotationSpeed;	//!< rotation/look speed [radians/s]

	/// is applied when the SHIFT button is held; useful for debugging planetary LoD
	float	_dbg_speed_multiplier;

//	bool	m_hasLinearMomentum;
//	bool	m_hasAngularMomentum;
	bool	m_invertPitch;	// invert vertical mouse movement?
	bool	m_invertYaw;	// invert horizontal mouse movement?

	float	m_pitchVelocity;	// pitch velocity accumulator
	float	m_yawVelocity;		// yaw velocity accumulator
	float	m_pitchAngle;		// current pitch angle in radians
	float	m_yawAngle;			// current yaw angle in radians (heading)

	float	m_pitchSensitivity;
	float	m_yawSensitivity;

	// for clamping speed and angles
	//float	m_minSpeed;
	float	m_minPitch;	// usually -Half_PI
	float	m_maxPitch;	// usually +Half_PI

public:
	mxDECLARE_CLASS(NwFlyingCamera,CStruct);
	mxDECLARE_REFLECTION;
	NwFlyingCamera();

	void setPosAndLookDir(
		const V3d& position,
		const V3f& look_direction
		);

	void setPosAndLookTarget(
		const V3d& position,
		const V3d& look_target
		);

	// don't forget to periodically call this function
	void Update( float delta_seconds );

	/// should not be called if the camera is controlled by physics
	void UpdateVelocityAndPositionFromInputs( SecondsF delta_seconds );

	//void Forward( float units );
	//void Strafe( float units );
	//void Pitch( float units );
	//void Down( float units );
	//void Yaw( float units );
	//void Up( float units );

	void Pitch( float units );
	void Yaw( float units );

	M44f BuildViewMatrix() const;

	enum EMovementFlags {
		FORWARD		= BIT(0),
		BACKWARD	= BIT(1),
		LEFT		= BIT(2),
		RIGHT		= BIT(3),
		UP			= BIT(4),
		DOWN		= BIT(5),
		ACCELERATE	= BIT(6),	//!< useful for debugging planetary LoD
		RESET		= BIT(7),	//!< reset position and orientation
		//STOP_NOW	= BIT(7),	//!< 		
	};

	//void processInput( const struct InputState& _inputState );

	// Input bindings
	void move_forward( GameActionID action, EInputState status, float value );
	void move_back( GameActionID action, EInputState status, float value );
	void strafe_left( GameActionID action, EInputState status, float value );
	void strafe_right( GameActionID action, EInputState status, float value );
	void move_up( GameActionID action, EInputState status, float value );
	void move_down( GameActionID action, EInputState status, float value );
	void rotate_pitch( GameActionID action, EInputState status, float value );
	void rotate_yaw( GameActionID action, EInputState status, float value );
	void accelerate( GameActionID action, EInputState status, float value );
	void stop( GameActionID action, EInputState status, float value );
};
