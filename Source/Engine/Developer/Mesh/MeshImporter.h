#pragma once

#include <Rendering/Public/Core/VertexFormats.h>

#include <Developer/Mesh/MeshCompiler.h>
#include <Developer/Mesh/MeshImporter.h>


namespace Meshok
{

enum EMeshOptimizationLevel
{
	OL_None,
	OL_Fast,	// fast optimizations for real-time quality
	OL_High,	// high level of quality
	OL_Max		// maximum level of quality (slow)
};

struct MeshImportSettings: CStruct
{
	const TbVertexFormat*	vertex_format;

	/// normalize all vertex components into the [-1,1] range?
	bool	normalize_to_unit_cube;

	const M44f*	pretransform;

	bool	flip_winding_order;

	//
	EMeshOptimizationLevel	mesh_optimization_level;


	/// useful if some models fail to import, because they don't have UV data:
	/// "Failed to compute tangents; need UV data in channel0"
	bool	force_ignore_uvs;

	///
	bool	force_ignore_tangents;

	/// NOTE: auto-generated UVs are nearly always wrong!
	bool	force_regenerate_UVs;

public:
	mxDECLARE_CLASS( MeshImportSettings, CStruct );
	mxDECLARE_REFLECTION;
	MeshImportSettings();

	void SetDefaultVertexFormatIfNeeded();
};

ERet ImportMesh(
				AReader& stream_reader
				, TcModel &imported_model_
				, const MeshImportSettings& import_settings
				, const char* debug_name = ""
				);
//ERet ImportMesh( AReader& _source, TcModel &_output, EMeshOptimizationLevel _level, const ImportSettings& _settings );
ERet ImportMeshFromFile(
						const char* _path
						, TcModel &_mesh
						, const MeshImportSettings& import_settings = MeshImportSettings()
						);

}//namespace Meshok
