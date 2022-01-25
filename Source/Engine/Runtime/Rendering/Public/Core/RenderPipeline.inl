/// Enumeration of possible scene [passes|layers|stages].
/// It's mainly to prevent typing errors when using 'StringHash("ScenePassName")'.
/// Ideally, they shouldn't be hardcoded into the engine,
/// but they are declared in code for performance reasons.
/// NOTE: the order is not important and doesn't influence the final rendering order!
///
#ifdef DECLARE_SCENE_PASS

/// A scene pass that isn't one of the other 
/// predefined scene pass types.
DECLARE_SCENE_PASS(Unknown, Bad)

/// e.g. update global resources
DECLARE_SCENE_PASS(FrameStart, Bad)


DECLARE_SCENE_PASS(DepthPrepass, Bad)

/// The scene passes for rendering shadow casters into a shadow map.
DECLARE_SCENE_PASS(RenderDepthOnly, Bad)


/// Voxelization pass for Global Illumination via Voxel Cone Tracing
DECLARE_SCENE_PASS(VXGI_Voxelize, Bad)

/// Copy the packed voxel scene data from the buffer to a 3D texture.
DECLARE_SCENE_PASS(VXGI_BuildVolumeTexture, Bad)

/// 
DECLARE_SCENE_PASS(VXGI_InjectSkyLighting, Bad)

// must be done before rendering the scene's geometry for mirrors, CSG, etc.
DECLARE_SCENE_PASS(MaskGeometryViaStencil, Bad)


/// The regular forward diffuse scene pass.
DECLARE_SCENE_PASS(Opaque_Forward, Bad)



/// Geometry stage: draw opaque surfaces into the G-Buffer.
DECLARE_SCENE_PASS(FillGBuffer, Bad)

/// Alpha-blended screen-space decals.
DECLARE_SCENE_PASS(DeferredDecals, Bad)


/// The scene pass made for reflection rendering.
//DECLARE_SCENE_PASS(Reflection)

/// Generate an occlusion buffer for Screen space ambient [occlusion|obscurance].
DECLARE_SCENE_PASS(SSAO, Bad)


/// Alpha-Blended (Order-Dependent) Transparency. E.g.: glass windows, weapon muzzle flashes.
DECLARE_SCENE_PASS(Translucent, Bad)


/// Blended Order-Independent Transparency (OIT)
DECLARE_SCENE_PASS(BlendedOIT, Bad)

/// e.g. fallback materials, missing textures
DECLARE_SCENE_PASS(Unlit, Bad)

/// filter the sky/environment (described analytically or by a cubemap) into a set of SH or ambient cube coefficients
DECLARE_SCENE_PASS(GenerateSkyIrradianceMap, Bad)

/// re-light light probes using radiance stored in voxels
DECLARE_SCENE_PASS(GenerateLightProbes, Bad)



/// Diffuse Global Illumination based on Light Probes
/// See:
/// Precomputed Light Field Probes [2017]
/// Dynamic Diffuse Global Illumination with Ray-Traced Irradiance Fields [2019]

/// Generate random rays
DECLARE_SCENE_PASS(DDGI_GenerateRays, Bad)

DECLARE_SCENE_PASS(DDGI_TraceProbeRays, Bad)

/// Re-light light probes using radiance stored in voxels
DECLARE_SCENE_PASS(DDGI_UpdateIrradianceProbes, Bad)
DECLARE_SCENE_PASS(DDGI_UpdateDistanceProbes, Bad)

DECLARE_SCENE_PASS(DDGI_VisualizeProbes, Bad)



// HACK: clear the LightingTarget
DECLARE_SCENE_PASS(ClearLightingTarget, Bad)



// Lighting stage

/// Render global (full-screen) lights (e.g. deferred directional lights)
DECLARE_SCENE_PASS(DeferredGlobalLights, PostFx)

// e.g. deferred shadow-casting spot/point lights
DECLARE_SCENE_PASS(DeferredLocalLights, PostFx)

/// Glowing objects
DECLARE_SCENE_PASS(EmissiveAdditive, PostFx)

/// deferred lighting
DECLARE_SCENE_PASS(DeferredShadows, PostFx)

// Cull lights and do lighting on the GPU, using a single Compute Shader.
DECLARE_SCENE_PASS(TiledDeferred_CS, PostFx)



// Alpha stage

// e.g. glass windows, other transparent surfaces
DECLARE_SCENE_PASS(Deferred_Translucent, Bad)

// e.g. explosions, smoke
DECLARE_SCENE_PASS(VolumetricParticles, Bad)



// Sky rendering
DECLARE_SCENE_PASS(Sky, Bad)




// Post-Processing

DECLARE_SCENE_PASS(PostFX, PostFx)

DECLARE_SCENE_PASS(HDR_CreateLuminance, PostFx)	// Creates the luminance map for the scene.
DECLARE_SCENE_PASS(HDR_AdaptLuminance, PostFx)	// Adjusts the scene luminance based on the previous scene luminance.
DECLARE_SCENE_PASS(Bloom_Threshold, PostFx)		// 'Bright Pass'
DECLARE_SCENE_PASS(Bloom_Downscale, PostFx)
DECLARE_SCENE_PASS(HDR_Tonemap, PostFx)	// Blit from HDR to LDR backbuffer



/// Tonemaps and writes HDR scene target to back-buffer or to input to FXAA.
DECLARE_SCENE_PASS(DeferredCompositeLit, PostFx)



// Fast Approximate Anti-Aliasing (FXAA)
DECLARE_SCENE_PASS(FXAA, PostFx)

/// e.g. show the contents of a ray-traced buffer - want to render this before debug lines
DECLARE_SCENE_PASS(DebugTextures, Debug)
/// draw all meshes in 'solid', 'thick' wireframe mode
DECLARE_SCENE_PASS(DebugWireframe, DebugWireframe)
/// auxiliary rendering
DECLARE_SCENE_PASS(DebugLines, Debug)
/// e.g. visualize the contents of a shadow map for debugging
DECLARE_SCENE_PASS(DebugScreenQuads, Debug)


/// Reduce screen-space depth into 1x1 RT with min & max values.
DECLARE_SCENE_PASS(ReduceViewDepth_CS, PostFx)



// user interface
DECLARE_SCENE_PASS(HUD, UI)

/// e.g. ImGui
DECLARE_SCENE_PASS(GUI, UI)

// performance HUD, debug texts

#undef DECLARE_SCENE_PASS
#endif
