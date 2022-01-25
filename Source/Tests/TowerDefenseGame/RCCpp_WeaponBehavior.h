#pragma once

class NwPlayerWeapon;
class TbWeaponBehaviorI;

//struct SWeaponBehaviors
//{
//	TPtr< TbWeaponBehaviorI >	plasmagun;
//	TPtr< TbWeaponBehaviorI >	grenade_launcher;
//};
//extern SWeaponBehaviors		g_weapon_behaviors;
//namespace WeaponBehaviors
//{
//	extern TPtr< TbWeaponBehaviorI >	grenade_launcher;
//}//namespace WeaponBehaviors

struct SystemTable
{
	TPtr< TbWeaponBehaviorI >	plasmagun;
	TPtr< TbWeaponBehaviorI >	grenade_launcher;
};

extern SystemTable              g_SystemTable;
