/*
=============================================================================
	idTech4 (Doom 3) shader/material support
=============================================================================
*/
#include <Base/Base.h>
#pragma hdrstop
#include <Core/Assets/AssetManagement.h>
#include <Core/Memory/MemoryHeaps.h>
#include <Core/Serialization/Serialization.h>
#include <GPU/Public/graphics_device.h>

#include <Sound/SoundResources.h>

#include <Rendering/BuildConfig.h>
#include <Rendering/Public/Core/Mesh.h>
#include <Rendering/Private/Modules/Animation/AnimClip.h>
//
#include <Rendering/Private/Modules/idTech4/idEntity.h>


namespace Rendering
{
mxDEFINE_CLASS( idEntityDef );
mxBEGIN_REFLECTION( idEntityDef )
	mxMEMBER_FIELD( skinned_mesh ),
	mxMEMBER_FIELD( sounds ),
mxEND_REFLECTION
idEntityDef::idEntityDef()
	: sounds( MemoryHeaps::renderer() )
{
}

mxDEFINE_CLASS( idEntity );
mxBEGIN_REFLECTION( idEntity )
	mxMEMBER_FIELD( def ),
	mxMEMBER_FIELD( anim_model ),
mxEND_REFLECTION
idEntity::idEntity()
{
}

}//namespace Rendering
