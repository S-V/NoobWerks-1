#include "stdafx.h"
#pragma hdrstop

#include <AssetCompiler/AssetCompilers/Graphics/MeshCompiler.h>

#if WITH_MESH_COMPILER

#include <Core/Serialization/Serialization.h>
#include <Developer/Mesh/MeshImporter.h>
#include <Core/Serialization/Text/TxTSerializers.h>

#include <Graphics/Public/graphics_formats.h>
#include <Graphics/Public/graphics_utilities.h>
#include <Rendering/Public/Globals.h>
#include <Rendering/Public/Core/VertexFormats.h>
#include <Rendering/Public/Core/Mesh.h>

#include <AssetCompiler/AssetMetadata.h>


namespace AssetBaking
{

mxDEFINE_CLASS(MeshCompiler);

ERet MeshCompiler::Initialize()
{
	return ALL_OK;
}

void MeshCompiler::Shutdown()
{
}

const TbMetaClass* MeshCompiler::getOutputAssetClass() const
{
	return &Rendering::NwMesh::metaClass();
}

ERet MeshCompiler::CompileAsset(
	const AssetCompilerInputs& inputs,
	AssetCompilerOutputs &outputs
	)
{
	if( 0 == strcmp(
		Str::FindExtensionS( inputs.path.c_str() ),
		TbRawMeshData::metaClass().m_name )
		)
	{
		mxDO(NwBlob_::loadBlobFromStream( inputs.reader, outputs.object_data ));
		return ALL_OK;
	}

	//
	AllocatorI & scratchpad = MemoryHeaps::temporary();

	//
	Meshok::MeshImportSettings	mesh_import_settings;
	LoadAssetMetadataFromFile( mesh_import_settings, inputs, scratchpad );
	mesh_import_settings.SetDefaultVertexFormatIfNeeded();

	//
	TcModel	imported_model( scratchpad );

	//
	mxTRY(Meshok::ImportMesh(
		inputs.reader
		, imported_model
		, mesh_import_settings
		, inputs.path.c_str()
		));

	//
	TbRawMeshData	raw_mesh_data;
	mxDO(MeshLib::CompileMesh(
		imported_model
		, *mesh_import_settings.vertex_format
		, raw_mesh_data
		));

	//
#if MX_DEVELOPER
	mxDO(Rendering::NwMesh::CompileMesh(
		NwBlobWriter(outputs.object_data)
		, raw_mesh_data
		, scratchpad
		));
#else
	ptBREAK;
#endif

	//
	LogStream(LL_Info) << "Saved mesh " << inputs.path << ", AABB: " << raw_mesh_data.bounds;

	return ALL_OK;
}

ERet MeshCompiler::CompileMeshAsset(
									const AssetCompilerInputs& inputs
									, AssetCompilerOutputs &outputs
									, TcModel &imported_model_
									, const Meshok::MeshImportSettings&	mesh_import_settings
									)
{
	AllocatorI & scratchpad = MemoryHeaps::temporary();

	//
	mxTRY(Meshok::ImportMesh(
		inputs.reader
		, imported_model_
		, mesh_import_settings
		, inputs.path.c_str()
		));

	//
	if(mesh_import_settings.pretransform)
	{
		imported_model_.transformVertices(
			*mesh_import_settings.pretransform
			);
		imported_model_.RecomputeAABB();
	}

	//
	TbRawMeshData	raw_mesh_data;
	mxDO(MeshLib::CompileMesh(
		imported_model_
		, *mesh_import_settings.vertex_format
		, raw_mesh_data
		));

	//
	mxDO(Rendering::NwMesh::CompileMesh(
		NwBlobWriter(outputs.object_data)
		, raw_mesh_data
		, scratchpad
		));

	return ALL_OK;
}

}//namespace AssetBaking

#endif // WITH_MESH_COMPILER
