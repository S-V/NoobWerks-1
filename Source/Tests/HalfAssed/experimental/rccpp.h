//

// IID_ENDInterfaceID
#include <RuntimeCompiledCPlusPlus/RuntimeObjectSystem/IObject.h>


struct NwWeaponBehaviorI;
struct NPC_BehaviorI;



// RCC++ uses interface Id's to distinguish between different classes
// here we have only one, so we don't need a header for this enum and put it in the same
// source code file as the rest of the code
enum InterfaceIDEnumConsoleExample
{
    IID_IRCCPP_MAIN_LOOP = IID_ENDInterfaceID, // IID_ENDInterfaceID from IObject.h InterfaceIDEnum

    IID_ENDInterfaceIDEnumConsoleExample
};



/// NOTE: must be called exactly SystemTable as declared in RuntimeCompiledCPlusPlus library!
struct SystemTable
{
	//TPtr< NwWeaponBehaviorI >	fists;
	//TPtr< NwWeaponBehaviorI >	shotgun;
	//TPtr< NwWeaponBehaviorI >	chaingun;
	//TPtr< NwWeaponBehaviorI >	plasmagun;
	//TPtr< NwWeaponBehaviorI >	grenade_launcher;

	// cannot use PLAYER_WEAPON_MAX, because cannot include the Weapons header
	TStaticArray< TPtr< NwWeaponBehaviorI >, 8 >	weapon_behaviors;

	// EMonsterType
	TStaticArray< TPtr< NPC_BehaviorI >, 8 >	npc_behaviors;
};

extern SystemTable		g_SystemTable;




namespace RCCPP
{

	//

}//namespace RCCPP
