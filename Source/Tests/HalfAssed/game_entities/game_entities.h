#pragma once

#include <Utility/Camera/NwFlyingCamera.h>
#include <Developer/ModelEditor/ModelEditor.h>	// NwLocalTransform
#include "game_forward_declarations.h"


///
struct NwMeshSocket: CStruct
{
	NwLocalTransform	local_transform;
	NameHash32			name_hash;

public:
	mxDECLARE_CLASS(NwMeshSocket, CStruct);
	mxDECLARE_REFLECTION;
	NwMeshSocket();
};



class GameSpaceship: CStruct
{
public:
	V3d		position_in_world_space;
	V3f		forward_dir;
	V3f		right_dir;


	TResPtr< NwMesh >	mesh;
	TPtr< RrMeshInstance >	mesh_instance;

	//NwLocalTransform	local_mesh_transform;

	TFixedArray< NwMeshSocket, 8 >	sockets;

public:
	mxDECLARE_CLASS(GameSpaceship, CStruct);
	mxDECLARE_REFLECTION;
	GameSpaceship();

	ERet load(
		NwClump* storage_clump
		, ARenderWorld* render_world
		);

	void unload();

	void reset();

	void update(
		const NwTime& args
		);

public:
	float calc_debug_camera_speed(
		) const;
};
