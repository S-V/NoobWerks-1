/*
=============================================================================
	File:	VectorMath.cpp
	Desc:
=============================================================================
*/

#include <Core/Core_PCH.h>
#pragma hdrstop
#include <Core/Core.h>
#include <Core/VectorMath.h>


/*
-----------------------------------------------------------------------------
	NwView3D
-----------------------------------------------------------------------------
*/
#define rxDEFAULT_ASPECT_RATIO		1.333333F

#define rxCAMERA_DEFAULT_NEAR_PLANE		1.0f
#define rxCAMERA_DEFAULT_FAR_PLANE		5000.0f
#define rxCAMERA_WIDESCREEN_ASPECT		(16.0f/9.0f)
#define rxCAMERA_DEFAULT_ASPECT			(rxDEFAULT_ASPECT_RATIO)
#define rxCAMERA_DEFAULT_FIELD_OF_VIEW	(mxPI_DIV_4)

mxDEFINE_CLASS(NwView3D);
mxBEGIN_REFLECTION(NwView3D)
	mxMEMBER_FIELD( right_dir ),
//	mxMEMBER_FIELD( up ),
	mxMEMBER_FIELD( look_dir ),
	mxMEMBER_FIELD( eye_pos ),
	mxMEMBER_FIELD( near_clip ),
	mxMEMBER_FIELD( far_clip ),
	mxMEMBER_FIELD( half_FoV_Y ),
	mxMEMBER_FIELD( aspect_ratio ),
mxEND_REFLECTION;
NwView3D::NwView3D()
{
	right_dir	= V3_RIGHT;
	look_dir	= V3_FORWARD;
//	up		= V3_UP;
	eye_pos	= V3_Zero();
	near_clip	= 0.1f;
	far_clip	= 10000.0f;
	half_FoV_Y	= rxCAMERA_DEFAULT_FIELD_OF_VIEW;
	aspect_ratio = rxDEFAULT_ASPECT_RATIO;
}

void NwView3D::setLookTarget( const V3f& look_at )
{
	V3f diff = look_at - eye_pos;
	float len = diff.normalize();
	mxASSERT(len > 1e-3f);

	look_dir = diff;
}
