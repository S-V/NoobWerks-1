//
#pragma once


#include <Base/Template/Containers/Blob.h>
#include <Graphics/Public/graphics_formats.h>	// TbRawMeshData
#include <Utility/Camera/NwFlyingCamera.h>	// NwFlyingCamera


class Rendering::NwMesh;
class Rendering::MeshInstance;









///
struct NwLocalTransform: CStruct
{
	V3f	translation;
	F32	uniform_scaling;
	Q4f	orientation;

public:
	mxDECLARE_CLASS(NwLocalTransform, CStruct);
	mxDECLARE_REFLECTION;
	NwLocalTransform();

	M44f toMat4() const;
};
ASSERT_SIZEOF(NwLocalTransform, 32);




class NwModelEditor: NonCopyable
{
public:
	TPtr< Rendering::NwMesh >		_mesh;
	TPtr< Rendering::MeshInstance >		_mesh_inst;
	TPtr< NwClump >				_storage_clump;

	NwFlyingCamera	_camera;
	//OrbitingCamera	_camera;

	NwLocalTransform	_mesh_transform;

	FilePathStringT	_loaded_mesh_filepath;
	NwBlob	_compiled_mesh_data_blob;

public:
	NwModelEditor();
	~NwModelEditor();

	ERet importMeshFromFile(
		const char* filepath
		, NwClump& storage_clump
		);

	ERet loadRawMeshFromFile(
		const char* filepath
		, NwClump& storage_clump
		);

	bool isLoaded() const { return _mesh_inst.IsValid(); }

	void clear();

private:
	ERet _createMeshInstance(
		NwClump& storage_clump
		);
};
