#include <Base/Base.h>
#pragma hdrstop

#define TINYOBJLOADER_IMPLEMENTATION // define this in only *one* .cc
#include <tinyobjloader/tiny_obj_loader.h>

#include <iostream>

#include <Base/Template/Containers/Blob.h>
#include <Core/Memory/MemoryHeaps.h>
#include <Core/Serialization/Serialization.h>
#include <Rendering/Public/Core/VertexFormats.h>
#include <Rendering/Public/Scene/MeshInstance.h>
#include <Developer/Mesh/MeshImporter.h>
#include <Developer/ModelEditor/ModelEditor.h>



NwModelEditor::NwModelEditor()
	: _compiled_mesh_data_blob(MemoryHeaps::global())
{

}

NwModelEditor::~NwModelEditor()
{

}

ERet NwModelEditor::importMeshFromFile(
					   const char* filepath
					   , NwClump& storage_clump
					   )
{
	mxASSERT(!_mesh);

	this->clear();

	//
	AllocatorI & temp_allocator = MemoryHeaps::temporary();


	TcModel	tc_model( temp_allocator );

	mxDO(Meshok::ImportMeshFromFile( filepath, tc_model ));


	// Rotate the spaceship into our coord system.
	{
		struct SwapAxesUtil
		{
			static void fun( V3f & v )
			{
				v = CV3f( -v.x, v.z, v.y );
			}
		};

		for( UINT iMesh = 0; iMesh < tc_model.meshes.num(); iMesh++ )
		{
			TcTriMesh& mesh = *tc_model.meshes[ iMesh ];

			for( UINT i = 0; i < mesh.positions.num(); i++ ) {
				SwapAxesUtil::fun( mesh.positions[i] );
			}
			for( UINT i = 0; i < mesh.tangents.num(); i++ ) {
				SwapAxesUtil::fun( mesh.tangents[i] );
			}
			for( UINT i = 0; i < mesh.binormals.num(); i++ ) {
				SwapAxesUtil::fun( mesh.binormals[i] );
			}
			for( UINT i = 0; i < mesh.normals.num(); i++ ) {
				SwapAxesUtil::fun( mesh.normals[i] );
			}

		}//For each mesh.
	}

	//
	TbRawMeshData	raw_imported_mesh;

	//
	const TbVertexFormat& runtime_vertex_format = DrawVertex::metaClass();
	
	mxDO(MeshLib::CompileMesh( tc_model, runtime_vertex_format, raw_imported_mesh ));

	//
	for( UINT iSubmesh = 0; iSubmesh < raw_imported_mesh.parts.num(); iSubmesh++ )
	{
		raw_imported_mesh.parts[ iSubmesh ].material_id = MakeAssetID("game_spaceship_pbr");
	}

#if MX_DEVELOPER
	//
	mxDO(Rendering::NwMesh::CompileMesh(
		NwBlobWriter(_compiled_mesh_data_blob)
		, raw_imported_mesh
		, temp_allocator
		));
#else
	ptBREAK;
#endif

	//
	mxDO(_createMeshInstance(storage_clump));

	Str::CopyS( _loaded_mesh_filepath, filepath );

	return ALL_OK;
}

ERet NwModelEditor::loadRawMeshFromFile(
	const char* filepath
	, NwClump& storage_clump
	)
{
	DBGOUT("loadRawMeshFromFile: %s", filepath);

	this->clear();

	mxDO(NwBlob_::loadBlobFromFile(_compiled_mesh_data_blob, filepath));

	//
	mxDO(_createMeshInstance(storage_clump));

	return ALL_OK;
}

void NwModelEditor::clear()
{
	if( _mesh != nil )
	{
		_mesh->release();
		//_storage_clump->f
		_mesh = nil;
	}

	_mesh_inst = nil;

	//
	_loaded_mesh_filepath.clear();
	_compiled_mesh_data_blob.RemoveAll();
}

ERet NwModelEditor::_createMeshInstance(
										NwClump& storage_clump
										)
{
	_storage_clump = &storage_clump;

	//
	Rendering::NwMesh *	new_mesh;
	mxDO(storage_clump.New( new_mesh ));

	mxDO(Rendering::NwMesh::LoadFromStream(
		*new_mesh,
		NwBlobReader(_compiled_mesh_data_blob)
		));

	_mesh = new_mesh;

	//
	mxDO(Rendering::MeshInstance::ñreateFromMesh(
		_mesh_inst._ptr
		, new_mesh
		, storage_clump
		));

	return ALL_OK;
}
