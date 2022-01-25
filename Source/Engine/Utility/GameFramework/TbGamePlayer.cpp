#include <Base/Base.h>
#pragma hdrstop

//#include <Input/Input.h>
//#include <Engine/WindowsDriver.h>	// InputState

#include <Utility/GameFramework/TbGamePlayer.h>

#if 0
TbGamePlayer::TbGamePlayer()
{
	_current_weapon_index = INDEX_NONE;
}

ERet TbGamePlayer::load(
	NwClump & storage_clump
	)
{

#if 0
	//
	mxDO(NwPlayerWeapon::loadWeapon_GrenadeLauncher(
		_weapon_grenade_launcher
		, storage_clump
		));

	//
	mxDO(NwPlayerWeapon::loadWeapon_Plasmagun(
		_weapon_plasmagun
		, storage_clump
		));

	//
	_weapons_in_inventory.add( &_weapon_grenade_launcher );
	_weapons_in_inventory.add( &_weapon_plasmagun );

	_current_weapon_index = 0;

#else

	_current_weapon_index = -1;

#endif
	
	return ALL_OK;
}

void TbGamePlayer::unload()
{
	//HACK to avoid crash when releasing weapon tables
	for( UINT i = 0; i < _weapons_in_inventory.num(); i++ )
	{
		NwPlayerWeapon* weapon_in_inventory =  _weapons_in_inventory[i];
		weapon_in_inventory->def._ptr = nil;
	}
}

ERet TbGamePlayer::renderFirstPersonView(
	const NwCameraView& camera_view
	, NGpu::NwRenderContext & render_context
	, const NwWeaponDef& weapon_def
	)
{
	if( _current_weapon_index != -1 )
	{
		NwPlayerWeapon* current_weapon = _weapons_in_inventory[ _current_weapon_index ];

		current_weapon->renderFirstPersonView(
			camera_view
			, render_context
			);
	}

	return ALL_OK;
}

void TbGamePlayer::update(
	 const InputState& input_state
	 , const M44f& camera_world_matrix	// camera_view.inverse_view_matrix
	 , const RrGlobalSettings& renderer_settings
	 , ALineRenderer & dbgLineDrawer
	)
{
	if( _current_weapon_index != -1 )
	{
		NwPlayerWeapon* current_weapon = _weapons_in_inventory[ _current_weapon_index ];
UNDONE;
		//current_weapon->update(
		//	input_state.
		//	, camera_world_matrix
		//	//, renderer_settings
		//	//, dbgLineDrawer
		//	);
	}
}

int TbGamePlayer::findWeaponIndexInInventory( TbPlayerWeaponId weapon_id ) const
{
	for( UINT i = 0; i < _weapons_in_inventory.num(); i++ )
	{
		NwPlayerWeapon* weapon_in_inventory =  _weapons_in_inventory[i];
		if( weapon_in_inventory->weapon_id == weapon_id ) {
			return i;
		}
	}

	return INDEX_NONE;
}

NwPlayerWeapon* TbGamePlayer::selectWeapon( TbPlayerWeaponId weapon_id )
{
	int weapon_index_in_inventory = this->findWeaponIndexInInventory( weapon_id );

	if( INDEX_NONE != weapon_index_in_inventory )
	{
		// if selected weapon will change
		if( _current_weapon_index != weapon_index_in_inventory )
		{
			_current_weapon_index = weapon_index_in_inventory;

			//
		}

		return _weapons_in_inventory[ weapon_index_in_inventory ];
	}

	return nil;
}

NwPlayerWeapon* TbGamePlayer::currentWeapon()
{
	return _weapons_in_inventory.isValidIndex( _current_weapon_index )
		? _weapons_in_inventory[ _current_weapon_index ]
		: nil
		;
}
#endif
//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
