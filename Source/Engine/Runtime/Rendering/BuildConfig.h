/*
=============================================================================
	Build configuration settings.
	These are compile time settings for the libraries.
=============================================================================
*/
#pragma once


#define nwRENDERER_CFG_ENABLE_DEBUGGING	(MX_DEBUG && MX_DEVELOPER)
#define nwRENDERER_DEBUG_DRAW_CALLS	(MX_DEBUG && MX_DEVELOPER)



/// must be synced with USE_REVERSED_DEPTH in shader code!
#define nwRENDERER_USE_REVERSED_DEPTH	(1)





/// Doom 3
#define nwRENDERER_CFG_ENABLE_idTech4 (0)


#define nwRENDERER_CFG_WITH_SKELETAL_ANIMATION	(0)



#define nwRENDERER_ENABLE_SHADOWS	(0)

/// Enable Voxel Grid for Global Illumination?
#define nwRENDERER_CFG_WITH_VOXEL_GI	(0)

#define nwRENDERER_ENABLE_LIGHT_PROBES	(0)






enum ERendererConstants
{
	// must be synced with MAX_BONES_LIMIT in shaders!
	nwRENDERER_MAX_BONES = 128,	// 128 joints in "maledict" MD5 mesh

	// to avoid redundant dynamic memory allocations;
	// the size should be tuned for the most common scenario
	nwRENDERER_NUM_RESERVED_SUBMESHES = 4,

	/// the maximum size of a pooled dynamic vertex or index buffer
	nwRENDERER_MAX_GEOMETRY_BUFFER_SIZE = mxMiB(8),

	//
	rrMAX_MATERIAL_PASSES = 8,
};


#ifndef MAX_SHADOW_CASCADES
enum { MAX_SHADOW_CASCADES = 4 };
#endif

/// The number of (nested) boxes for Global Illumination based on Cascaded Voxel Cone Tracing
enum { MAX_GI_CASCADES = 4 };
