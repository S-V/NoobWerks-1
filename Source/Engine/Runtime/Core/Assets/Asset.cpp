/*
=============================================================================
	Base class for resource objects.
=============================================================================
*/
#include <Core/Core_PCH.h>
#pragma hdrstop
#include <Core/Assets/Asset.h>

mxDEFINE_CLASS(NwResource);
mxBEGIN_REFLECTION(NwResource)
mxEND_REFLECTION

NwSharedResource::NwSharedResource()
	//: _reference_count( 0 )
{
}

mxDEFINE_CLASS(NwSharedResource);
mxBEGIN_REFLECTION(NwSharedResource)
	//mxMEMBER_FIELD(_reference_count),
mxEND_REFLECTION

NwSharedResource::~NwSharedResource()
{
	//mxASSERT(AtomicLoad(_reference_count) == 0);
}

void NwSharedResource::Grab()
{
	//AtomicIncrement( &_reference_count );
}

void NwSharedResource::Drop(
							const AssetID& asset_id
							, const TbMetaClass& asset_class
							)
{
	//mxASSERT(AtomicLoad(_reference_count) > 0);

	//if( AtomicDecrement( &_reference_count ) == 0 )
	//{
	//	//Resources::unload(this);
	//}
}
