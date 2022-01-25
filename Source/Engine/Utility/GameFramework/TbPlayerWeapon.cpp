//#include "stdafx.h"
#pragma hdrstop

#include <Base/Base.h>

#include <Rendering/Public/Settings.h>

#include <Sound/SoundSystem.h>

#include <Utility/GameFramework/TbPlayerWeapon.h>


mxDEFINE_CLASS(NwPlayerWeaponViewOffset);
mxBEGIN_REFLECTION(NwPlayerWeaponViewOffset)
	mxMEMBER_FIELD( gun_angles_deg ),
	mxMEMBER_FIELD( gun_scale ),
	mxMEMBER_FIELD( gun_offset ),
mxEND_REFLECTION;
NwPlayerWeaponViewOffset::NwPlayerWeaponViewOffset()
{
	/*
	viewrocketlauncher.md5mesh coord system:
	X - forward, Y - left, Z - up

	Our coord system:
	X - right, Y - forward, Z - up

	I.e. for the rocket launcher, we need to rotate the weapon clockwise by 90 degrees around Z.
	*/
	gun_angles_deg = CV3f(0,0,0);
	gun_offset = CV3f(0);
	gun_scale = 1;
}

M44f NwPlayerWeaponViewOffset::toMatrix() const
{
	return //M44_RotationX(DEG2RAD(gun_angles_deg.x))
		//* M44_RotationY(DEG2RAD(gun_angles_deg.y))
		 M44_RotationZ(DEG2RAD(gun_angles_deg.z))
		* M44_uniformScaling( gun_scale )
		* M44_buildTranslationMatrix( gun_offset )
		;
}


mxDEFINE_CLASS(NwWeaponMuzzleFlashDef);
mxBEGIN_REFLECTION(NwWeaponMuzzleFlashDef)
	mxMEMBER_FIELD( light_color ),
	mxMEMBER_FIELD( light_radius ),
	mxMEMBER_FIELD( light_fadetime ),
mxEND_REFLECTION;
NwWeaponMuzzleFlashDef::NwWeaponMuzzleFlashDef()
{
	light_color = CV3f(1.0, 0.8f, 0.4f);
	light_radius = 4.0f;
	light_fadetime = 0;
}

mxDEFINE_CLASS(NwWeaponParamsForHidingDef);
mxBEGIN_REFLECTION(NwWeaponParamsForHidingDef)
	mxMEMBER_FIELD( hide_distance ),
	mxMEMBER_FIELD( hide_time_msec ),
mxEND_REFLECTION;
NwWeaponParamsForHidingDef::NwWeaponParamsForHidingDef()
{
	hide_distance = 1;
	hide_time_msec = 300;
}


mxDEFINE_CLASS(NwWeaponProjectileDef);
mxBEGIN_REFLECTION(NwWeaponProjectileDef)
	mxMEMBER_FIELD( initial_velocity ),
	mxMEMBER_FIELD( life_span_seconds ),
	mxMEMBER_FIELD( mass ),
	mxMEMBER_FIELD( collision_radius ),
mxEND_REFLECTION;
NwWeaponProjectileDef::NwWeaponProjectileDef()
{
	initial_velocity = 0;
	life_span_seconds = 1;
	mass = 1;
	collision_radius = 0;
}

mxDEFINE_CLASS(NwWeaponDef);
mxBEGIN_REFLECTION(NwWeaponDef)
	mxMEMBER_FIELD( FoV_Y_in_degrees ),

	//
	mxMEMBER_FIELD( view_offset ),
	mxMEMBER_FIELD( move_with_camera ),

	//
	mxMEMBER_FIELD( horiz_sway ),
	mxMEMBER_FIELD( vert_sway ),

	//
	mxMEMBER_FIELD( muzzle_flash ),

	//
	mxMEMBER_FIELD( params_for_hiding ),

	//
	mxMEMBER_FIELD( projectile ),

	//
	mxMEMBER_FIELD( clip_size ),
	mxMEMBER_FIELD( max_recharge_ammo ),

	mxMEMBER_FIELD( damage ),
	mxMEMBER_FIELD( damage_mp ),
mxEND_REFLECTION;
NwWeaponDef::NwWeaponDef()
{
	FoV_Y_in_degrees = 74.0f;

	//
	move_with_camera = true;

	//
	horiz_sway = 1;
	vert_sway = 1;

	//
	clip_size = 0;
	max_recharge_ammo = 0;

	damage = 0;
	damage_mp = 0;
}
