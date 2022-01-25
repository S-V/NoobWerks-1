#pragma once


/// Enable 'clever' micro-optimizations?
/// WARNING: using bit hacks obscures programmer's intent
/// and good compilers can usually optimize the code better than humans
#define nwMATH_USE_BIT_TRICKS	(0)



mxSTOLEN("comments from CryEngine3 SDK, Cry_Camera.h");
//
// We are using a "right-handed" coordinate system where the positive X-Axis points 
// to the right, the positive Y-Axis points away from the viewer and the positive
// Z-Axis points up. The following illustration shows our coordinate system.
//
// <PRE>
//  Z-axis                                  
//    ^                               
//    |                               
//    |   Y-axis                   
//    |  /                         
//    | /                           
//    |/                             
//    +----------------> X-axis     
// </PRE>
//
// This same system is also used in Blender/3D-Studio-MAX and CryENGINE.
//
// Vector cross product:
// The direction of the cross product follows the right hand rule.
// Rotation direction is counter-clockwise when looking down the axis
// from the the positive end towards the origin.
//
// Matrix multiplication order: left-to-right (as in DirectX),
// i.e. scaling, followed by rotation, followed by translation:
// S * R * T.

// SRT - scale rotation translation
// SQT - scale quaternion translation

// PRY - Pitch/Roll/Yaw
// PRS - position rotation scale

// Euler angles:
// Pitch (Attitude) (Up / Down) - rotation around X axis (between -90 and 90 deg);
// Roll (Bank) (Fall over)      - rotation around Y axis (between -180 and 180 deg);
// Yaw (Heading) (Left / Right) - rotation around Z axis (yaw is also called 'heading') (between -180 and 180).

// Azimuth - horizontal angle (yaw)
// Elevation - vertical angle (pitch)
// Tilt - angle around the look direction vector (roll)

// The order of transformations is pitch first, then roll, then yaw.
// Relative to the object's local coordinate axis,
// this is equivalent to rotation around the x-axis,
// followed by rotation around the y-axis,
// followed by rotation around the z-axis.

// Angle units: radians.

// In our right-handed coordinate system front faces are counter-clockwise triangles.
// That is, a triangle will be considered front-facing if its vertices are counter-clockwise.

/// For referring to each axis by name rather than by an integer.
enum EAxis
{
	AXIS_X = 0,
	AXIS_Y = 1,
	AXIS_Z = 2,

	NUM_AXES,
};




///
struct NwOrthogonalAxes
{
	const EAxis	axis0;
	// the other two directions orthogonal to the current axis
	const EAxis axis1;
	const EAxis axis2;

public:
	// Pack four 2-bit values [1,2,0,1] into 32-bit value
	// { AXIS_Y, AXIS_Z, AXIS_X,  AXIS_Y };
	static const U32 LUT
		= (1     )
		| (2 << 2)
		| (0 << 4)
		| (1 << 6)
		;

	mxFORCEINLINE NwOrthogonalAxes( const EAxis axis )
		: axis0( axis )
#if !nwMATH_USE_BIT_TRICKS
		, axis1( EAxis( ( axis + 1 ) % 3 ) )
		, axis2( EAxis( ( axis + 2 ) % 3 ) )
#else
		, axis1( EAxis( (LUT >> (axis * 2)) & 3 ) )
		, axis2( EAxis( (LUT >> (axis * 2 + 2)) & 3 ) )
#endif
	{}
};
