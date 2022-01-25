/*
=============================================================================
	Graphics model used for rendering.
=============================================================================
*/
#include <Base/Base.h>
#pragma hdrstop

#include <Base/Template/Containers/Blob.h>

#include <Core/Assets/AssetManagement.h>
#include <Core/ObjectModel/Clump.h>

#include <Rendering/Public/Scene/MeshInstance.h>


namespace Rendering
{

mxBEGIN_FLAGS( BModelFlags )
	//mxREFLECT_BIT( Visible, EModelFlags::MF_Visible ),
	mxREFLECT_BIT( Unused, EModelFlags::MF_Unused ),
mxEND_FLAGS

mxBEGIN_STRUCT(BoneMatrix)
	mxMEMBER_FIELD(r),
mxEND_REFLECTION

/*
-----------------------------------------------------------------------------
	MeshInstance
-----------------------------------------------------------------------------
*/
mxDEFINE_CLASS( MeshInstance );
mxBEGIN_REFLECTION( MeshInstance )
	mxMEMBER_FIELD( mesh ),
	mxMEMBER_FIELD( materials ),
	mxMEMBER_FIELD( transform ),
	mxMEMBER_FIELD( joint_matrices ),
	mxMEMBER_FIELD( _flags ),
mxEND_REFLECTION
MeshInstance::MeshInstance()
{
	_flags.raw_value = 0;
	render_entity_handle.SetNil();
	Arrays::initializeWithExternalStorage( materials, z__materials_storage );
}

ERet MeshInstance::setupFromMesh(
								   NwMesh* mesh,
								   NwClump & clump,
								   const M44f& _localToWorld
								   , Material* fallback_material /*= nil*/
								   )
{
	this->mesh = mesh;

	// Create a new transform.
	{
		TPtr< RrTransform > modelTransform;
		mxDO(clump.New( modelTransform._ptr ));
		modelTransform->setLocalToWorldMatrix( _localToWorld );
		this->transform = modelTransform;
	}

	// Copy default materials.
	mxDO(this->materials.setNum( mesh->submeshes.num() ));


	for( UINT i = 0; i < mesh->default_materials_ids.num(); i++ )
	{
		Material* material;
		mxDO(Resources::Load(
			material,
			mesh->default_materials_ids[i],
			&clump,
			LoadFlagsT(0),
			fallback_material
			));
		materials[i] = material;
	}

	return ALL_OK;
}

ERet MeshInstance::ñreateFromMesh(
	MeshInstance *&new_model_
	, NwMesh* mesh
	, NwClump & storage
	, const M44f* local_to_world_transform
	, Material* override_material	// = nil
	)
{
	// Create a new model.
	MeshInstance* model = nil;
	mxDO(storage.New( model ));

	//
	model->mesh = mesh;


	// Create a new transform.

	TPtr< RrTransform > model_transform;
	mxDO(storage.New( model_transform._ptr ));

	if( local_to_world_transform ) {
		model_transform->setLocalToWorldMatrix( *local_to_world_transform );
	} else {
		model_transform->setLocalToWorldMatrix( g_MM_Identity );
	}

	model->transform = model_transform;


	//

	const U32 num_submeshes = mesh->default_materials_ids.num();

	mxDO(model->materials.setNum( num_submeshes ));

	if( override_material )
	{
		for( UINT i = 0; i < mesh->default_materials_ids.num(); i++ )
		{
			model->materials[i] = override_material;
		}
	}
	else
	{
		// TODO: Copy default materials.

#if 0
		for( UINT i = 0; i < mesh->_default_materials_ids.num(); i++ )
		{
			//UNDONE;
//			model->materials[i] = mesh->_default_materials_ids[i]->getDefaultInstance();
			model->materials[i] = Material::;
		}

#else
		Material* fallback_mat = Material::getFallbackMaterial();
		
		for( UINT i = 0; i < mesh->default_materials_ids.num(); i++ )
		{
			model->materials[i] = fallback_mat;
		}
#endif
	}

	//
	new_model_ = model;

	return ALL_OK;
}

}//namespace Rendering
