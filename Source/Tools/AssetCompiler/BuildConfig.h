#pragma once


#define WITH_SCRIPT_COMPILER			(1)
#define WITH_MESH_COMPILER				(1)
#define WITH_TEXTURE_COMPILER			(1)
#define WITH_SHADER_COMPILER			(1)
#define WITH_SHADER_EFFECT_COMPILER		(1)
#define WITH_MATERIAL_COMPILER			(1)
#define WITH_RENDERING_PIPELINE_COMPILER	(1)
#define WITH_INPUT_BINDINGS_COMPILER	(1)


// the asset compiler will act as a server and notify its clients about file changes, etc.
#define WITH_NETWORK			(0)
