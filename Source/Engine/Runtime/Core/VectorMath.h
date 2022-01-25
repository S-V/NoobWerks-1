/*
=============================================================================
	File:	VectorMath.h
	Desc:	
	ToDo:	cleanup and move code to other place
=============================================================================
*/
#pragma once

#include <Input/Input.h>

/*
-----------------------------------------------------------------------------
	NwView3D
	contains common view parameters:
	the data necessary for constructing the view and projection matrices.
-----------------------------------------------------------------------------
*/
mxPREALIGN(16) struct NwView3D: CStruct
{
	/// Eye position - Location of the camera, usually expressed in 'floating origin' space
	/// (or in world space, if scene is small enough).
	V3f		eye_pos;			//12
	U32		unused_padding[1];

	// Basis vectors, expressed in world space:
	// X(right), Y(lookDir) and Z(up) vectors, must be orthonormal (they form the camera coordinate system).
	V3f		right_dir;			//12 Right direction in world space.
	V3f		look_dir;			//12 Look direction in world space.

	float	near_clip;		//4 Near clipping plane.
	float	far_clip;		//4 Far clipping plane.

	float	half_FoV_Y;		//4 Vertical field of view angle, in radians (default = PI/4).
	float	aspect_ratio;	//4 projection ratio (ratio between width and height of view surface)
	float	screen_width;	//4 viewport width
	float	screen_height;	//4 viewport height
	// 64 bytes

public:
	mxDECLARE_CLASS(NwView3D,CStruct);
	mxDECLARE_REFLECTION;
	NwView3D();

	void setLookTarget( const V3f& look_at );

	/// Up direction == cross( right, up )
	V3f getUpDir() const { return V3_Cross( right_dir, look_dir ); }
} mxPOSTALIGN(16);

mxDECLARE_POD_TYPE(NwView3D);
ASSERT_SIZEOF(NwView3D,64);
