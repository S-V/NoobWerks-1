/*
=============================================================================
	idTech4 (Doom 3) render model
=============================================================================
*/
#pragma once

//
#include <Base/Template/Containers/Dictionary/TSortedMap.h>
//
#include <Core/Assets/AssetReference.h>	// TResPtr<>
//
#include <Rendering/BuildConfig.h>
#include <Rendering/ForwardDecls.h>
#include <Rendering/Public/Globals.h>	// HRenderEntity
#include <Rendering/Private/Modules/Animation/AnimatedModel.h>
//#include <Rendering/Private/Modules/idTech4/idMaterial.h>


struct NwAnimClip;
struct NwSoundSource;

namespace Rendering
{
mxPREALIGN(16) struct idEntityDef: NwSharedResource
{
	/// Anim set: holds all the animation data which belongs to one target object
	/// (for instance, all the animation data for a character)

	/// maps a name hash to the animation's index
	//TSortedMap< NameHash32, TResPtr< NwAnimClip > >	anims;

	TResPtr< NwSkinnedMesh >	skinned_mesh;

	///
	TSortedMap< NameHash32, TResPtr< NwSoundSource > >	sounds;

public:
	mxDECLARE_CLASS( idEntityDef, NwSharedResource );
	mxDECLARE_REFLECTION;
	idEntityDef();
};


mxREFACTOR("remove NwAnimatedModel");
/// high-level object with animations and sounds, e.g. an enemy NPC
mxPREALIGN(16) struct idEntity: CStruct
{
	//
	TPtr< idEntityDef >	def;

	// Rendering
	NwAnimatedModel	anim_model;

public:
	mxDECLARE_CLASS( idEntity, CStruct );
	mxDECLARE_REFLECTION;
	idEntity();

	ERet loadAssets();

	const NwAnimClip* findAnimByNameHash( const NameHash32 name_hash ) const;
	const NwAnimClip* getAnimByNameHash( const NameHash32 name_hash ) const;

public:
	void DbgPrintAnims() const;

} mxPOSTALIGN(16);

}//namespace Rendering
