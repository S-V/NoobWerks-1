#pragma once

//
#include <Core/Tasking/TinyCoroutines.h>
//
#include <Engine/WindowsDriver.h>
//
#include <Rendering/Private/Modules/Animation/SkinnedMesh.h>
#include <Rendering/Private/Modules/Animation/AnimPlayback.h>	// MillisecondsUInt16


class NwSoundSystemI;
class NwWeaponBehaviorI;

/// This transform is post-multiplied with the camera's world matrix
/// to obtain the world matrix for rendering the weapon in first-person view.
///
struct NwPlayerWeaponViewOffset: CStruct
{
	// Euler angles
	V3f		gun_angles_deg;
	float	gun_scale;
	V3f		gun_offset;

public:
	mxDECLARE_CLASS(NwPlayerWeaponViewOffset, CStruct);
	mxDECLARE_REFLECTION;
	NwPlayerWeaponViewOffset();

	M44f toMatrix() const;
};


struct NwWeaponMuzzleFlashDef: CStruct
{
	V3f			light_color;
	float		light_radius;	// in meters
	SecondsF	light_fadetime;
public:
	mxDECLARE_CLASS(NwWeaponMuzzleFlashDef, CStruct);
	mxDECLARE_REFLECTION;
	NwWeaponMuzzleFlashDef();
};

///
struct NwWeaponParamsForHidingDef: CStruct
{
	// Hiding (when holstering or approaching friendly NPCs)
	float				hide_distance;
	MillisecondsUInt16	hide_time_msec;

public:
	mxDECLARE_CLASS(NwWeaponParamsForHidingDef, CStruct);
	mxDECLARE_REFLECTION;
	NwWeaponParamsForHidingDef();
};


struct NwWeaponProjectileDef: CStruct
{
	F32			initial_velocity;
	SecondsF	life_span_seconds;
	F32			mass;
	F32			collision_radius;

public:
	mxDECLARE_CLASS(NwWeaponProjectileDef, CStruct);
	mxDECLARE_REFLECTION;
	NwWeaponProjectileDef();
};

/// specifies weapon characteristics
///
struct NwWeaponDef: NwSharedResource
{
	// Appearance

	// You can set a custom field of view per weapon.
	// http://imaginaryblend.com/2018/10/16/weapon-fov/
	float	FoV_Y_in_degrees;	// default = 74 degrees

	// used to tweak weapon position relative to eye pos
	NwPlayerWeaponViewOffset	view_offset;
	bool						move_with_camera;

	// bouncing due to player movement
	float	horiz_sway;
	float	vert_sway;

	//
	NwWeaponMuzzleFlashDef	muzzle_flash;

	//
	NwWeaponParamsForHidingDef	params_for_hiding;

	NwWeaponProjectileDef	projectile;

	// Stats

	U32			clip_size;	// Ammo Capacity
	U32			max_recharge_ammo;	// Max. Ammo

	F32			damage;
	F32			damage_mp;

public:
	mxDECLARE_CLASS(NwWeaponDef, NwSharedResource);
	mxDECLARE_REFLECTION;
	NwWeaponDef();

	static const SerializationMethod SERIALIZATION_METHOD = Serialize_as_Text;
};
