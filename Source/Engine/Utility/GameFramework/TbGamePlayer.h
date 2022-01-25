#pragma once

#include <Utility/GameFramework/TbPlayerWeapon.h>


mxREMOVE_THIS

#if 0

class TbGamePlayer
{
public:
	NwPlayerWeapon		_weapon_grenade_launcher;
	NwPlayerWeapon		_weapon_plasmagun;

	enum { MAX_WEAPONS_IN_INVENTORY = 32 };

	TFixedArray< NwPlayerWeapon*, MAX_WEAPONS_IN_INVENTORY >	_weapons_in_inventory;
	I32															_current_weapon_index;

public:
	TbGamePlayer();

	ERet load(
		NwClump & storage_clump
		);

	void unload();

	ERet renderFirstPersonView(
		const NwCameraView& camera_view
		, GL::NwRenderContext & render_context
		, const NwWeaponDef& weapon_def
		);

	void update(
		 const InputState& input_state
		 , const M44f& camera_world_matrix	// camera_view.inverse_view_matrix
		 , const RrGlobalSettings& renderer_settings
		 , ALineRenderer & dbgLineDrawer
		);

public:	// Weapon control

	int findWeaponIndexInInventory( TbPlayerWeaponId weapon_id ) const;

	NwPlayerWeapon* selectWeapon( TbPlayerWeaponId weapon_id );

	NwPlayerWeapon* currentWeapon();
};
#endif

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
