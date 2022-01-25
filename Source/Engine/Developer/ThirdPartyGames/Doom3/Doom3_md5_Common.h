/*

MD5Mesh and MD5Anim files formats
(Doom 3's models)
http://tfc.duke.free.fr/coding/md5-specs-en.html

MD5 (file format)
https://modwiki.dhewm3.org/MD5_(file_format)

Making DOOM 3 Mods : Models
https://www.iddevnet.com/doom3/models.html

*/
#pragma once


#define MD5_VERSION_STRING		"MD5Version"
// Doom 3, including BFG edition
#define MD5_VERSION_DOOM3		10
// Enemy Territory: Quake Wars
#define MD5_VERSION_ETQW		11


namespace Doom3
{


static const int MD5_ROOT_JOINT_INDEX = 0;
static const int MD5_NO_PARENT_JOINT = -1;

// bind-pose skeleton's joint, joints make up a skeleton
struct md5Joint
{
	Q4f			orientation;	// joint's orientation quaternion in object space
	V3f			position;		// joint's position in object space
	int			parentNum;		// index of the parent joint; this is the root joint if (-1)
	String64	name;

	// For converting into ozz animation. These are relative to the parent bone.
	Q4f		ozz_orientation;
	V3f		ozz_position;
};




inline
V3f LerpVectors( const V3f &v1, const V3f &v2, const float l ) {
	if ( l <= 0.0f ) {
		return v1;
	} else if ( l >= 1.0f ) {
		return v2;
	} else {
		return v1 + ( v2 - v1 ) * l;
	}
}


struct MD5LoadOptions
{
	V3f		root_joint_offset;

	/// scale from Doom 3 units to our meters
	bool	scale_to_meters;

public:
	MD5LoadOptions()
	{
		root_joint_offset = CV3f(0);
		scale_to_meters = false;
	}
};



}//namespace Doom3
