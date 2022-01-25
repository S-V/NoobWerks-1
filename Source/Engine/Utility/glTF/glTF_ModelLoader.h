// glTF model loader for displaying objects with PBR materials.
#pragma once

#include <Rendering/ForwardDecls.h>

class TcModel;
class NwClump;

namespace glTF
{
	ERet loadModelFromFile(
		TcModel *model_
		, const char* gltf_filepath
		, NwClump * storage
		, TArray< TRefPtr< TcMaterial > > &submesh_materials_ // for debug rendering
		);

	ERet createMeshFromFile(
		Rendering::NwMesh **new_mesh_
		, const char* gltf_filepath
		, NwClump * storage
		, TArray< TRefPtr< TcMaterial > > &submesh_materials_ // for debug rendering
		);

}//namespace glTF
