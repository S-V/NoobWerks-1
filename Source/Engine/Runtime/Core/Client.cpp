/*
=============================================================================
	File:	Client.cpp
	Desc:	
=============================================================================
*/
#include <Core/Core_PCH.h>
#pragma hdrstop
#include <Core/Client.h>
#include <Core/Serialization/Serialization.h>

mxBEGIN_REFLECT_ENUM( GameStateID )
#define DECLARE_GAME_STATE( name, description )		mxREFLECT_ENUM_ITEM( name, GameStates::name ),
	#include <Core/GameStates.inl>
#undef DECLARE_GAME_STATE
mxREFLECT_ENUM_ITEM( ALL, GameStates::ALL ),
mxEND_REFLECT_ENUM

mxBEGIN_FLAGS( GameStateF )
#define DECLARE_GAME_STATE( name, description )		mxREFLECT_ENUM_ITEM( name, BIT(GameStates::name) ),
	#include <Core/GameStates.inl>
#undef DECLARE_GAME_STATE
//NOTE: it mustn't be defined:
//mxREFLECT_BIT( ALL, GameStates::ALL ),
mxEND_FLAGS

namespace GameActions {

}//namespace GameActions

mxBEGIN_REFLECT_ENUM( GameActionID )
#define DECLARE_GAME_ACTION( name, description )	mxREFLECT_ENUM_ITEM( name, GameActions::name ),
	#include <Core/GameActions.inl>
#undef DECLARE_GAME_ACTION
mxEND_REFLECT_ENUM

/*
-----------------------------------------------------------------------------
	KeyBind
-----------------------------------------------------------------------------
*/
mxDEFINE_CLASS(KeyBind);
mxBEGIN_REFLECTION(KeyBind)
	mxMEMBER_FIELD(key),
	mxMEMBER_FIELD(when),
	mxMEMBER_FIELD(with),
	mxMEMBER_FIELD(action),
	//mxMEMBER_FIELD(new_state),
	mxMEMBER_FIELD(command),
mxEND_REFLECTION;
KeyBind::KeyBind()
{
	key = EKeyCode::KEY_Unknown;
	when = IS_Pressed;
	with = KeyModifier_None;
	action = GameActions::NONE;
	//new_state = GameStates::none;
}

/*
-----------------------------------------------------------------------------
	AxisBind
-----------------------------------------------------------------------------
*/
mxDEFINE_CLASS(AxisBind);
mxBEGIN_REFLECTION(AxisBind)
	mxMEMBER_FIELD(axis),
	mxMEMBER_FIELD(action),
mxEND_REFLECTION;
AxisBind::AxisBind()
{
	axis = MouseAxisX;
	action = GameActions::NONE;
}

/*
-----------------------------------------------------------------------------
	InputContext
-----------------------------------------------------------------------------
*/
mxDEFINE_CLASS(InputContext);
mxBEGIN_REFLECTION(InputContext)
	mxMEMBER_FIELD(name),
	mxMEMBER_FIELD(state),
	mxMEMBER_FIELD(key_binds),
	mxMEMBER_FIELD(axis_binds),
	//mxMEMBER_FIELD(state_mask),
mxEND_REFLECTION;
InputContext::InputContext()
{
	state = GameStates::none;
	//state_mask = GameStates::ALL;
}


mxDEFINE_CLASS(NwInputBindings);
mxBEGIN_REFLECTION(NwInputBindings)
	mxMEMBER_FIELD(contexts),
mxEND_REFLECTION;
//NwInputBindings* NwInputBindings::Create( const CreateContext& _ctx )
//{
////	mxASSERT(nil == dynamic_cast<Clump*>(&_ctx.storage));
//	NwInputBindings * o = nil;
//	Serialization::LoadMemoryImage( _ctx.stream, o, _ctx.storage );
//	return o;
//}
//ERet NwInputBindings::Load( NwInputBindings *_o, const LoadContext& _ctx )
//{
//	return ALL_OK;
//}
//void NwInputBindings::Unload( NwInputBindings * _o )
//{
//
//}

void NwInputBindings::Dbg_Print() const
{
	for( UINT iInputContext = 0; iInputContext < contexts.num(); iInputContext++ )
	{
		const InputContext& inputContext = contexts[ iInputContext ];
		DBGOUT("InputContext[%u]: '%s', %u key binds, %u axis binds"
			, iInputContext, inputContext.name.c_str(), inputContext.key_binds.num(), inputContext.axis_binds.num());

		for( UINT iKeyBind = 0; iKeyBind < inputContext.key_binds.num(); iKeyBind++ )
		{
			const KeyBind& keyBind = inputContext.key_binds[ iKeyBind ];
			DBGOUT("\tKey '%s' ->: '%s'", EKeyCode_To_Chars(keyBind.key), GameActionID_Type().FindString(keyBind.action));
		}
	}
}

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
