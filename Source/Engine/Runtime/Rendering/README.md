# Renderer - a high-level rendering engine, uses the low-level Graphics module.

Table of contents:

### G-Buffer

/*
-----------------------------------------------------------------------------
	Deferred Shading
-----------------------------------------------------------------------------
*/
#G-Buffer

G-Buffer Layout:

// [-] - Hardware depth-stencil buffer (used for restoring view-space positions).
// [0] - RGB: view-space normal; A - roughness
// [1] - RGB: diffuse reflectance; A - metalness
// [2] - RGB: specular color; A - Specular Power

// RT0 =       Normal.x     | Normal.y         | Normal.z        | roughness
// RT1 =       Diffuse.r    | Diffuse.g        | Diffuse.b       | metalness
// RT2 =       Specular.x   | Specular.y       | Specular.b      | Specular Power
// RT3 =                                                         | 





## References:

### Collections of Links:
https://github.com/Gforcex/OpenGraphic


### Graphics Engines:

The G3D Innovation Engine
https://casual-effects.com/g3d/www/index.html



### General

RENDERING THE HELLSCAPE OF DOOM ETERNAL (SIGGRAPH 2020)
http://advances.realtimerendering.com/s2020/RenderingDoomEternal.pdf

DOOM Eternal - Graphics Study
30 Aug 2020 - Simon Coenen 
https://simoncoenen.com/blog/programming/graphics/DoomEternalStudy.html

Bringing Fortniteto Mobile with Vulkanand OpenGL ES (GDC 2019)
https://www.khronos.org/assets/uploads/developers/library/2019-gdc/Vulkan-Bringing-Fortnite-to-Mobile-Samsung-GDC-Mar19.pdf


### Modern Deferred shading

https://mynameismjp.wordpress.com/2016/03/25/bindless-texturing-for-deferred-rendering-and-decals/


/*
-----------------------------------------------------------------------------
	Physically Based Rendering (PBR)
-----------------------------------------------------------------------------
*/

- metallness / roughness workflow

Beginners' intro:
https://doc.babylonjs.com/how_to/physically_based_rendering

PBR references:

Filament, an in-depth explanation of real-time physically based rendering,
the graphics capabilities and implementation of Filament.
This document explains the math and reasoning behind most of our decisions.
This document is a good introduction to PBR for graphics programmers.
https://google.github.io/filament/Filament.html
https://github.com/google/filament

https://academy.substance3d.com/courses/the-pbr-guide-part-1
https://academy.substance3d.com/courses/the-pbr-guide-part-2
https://www.allegorithmic.com/pbr-guide
https://www.jeremyromanowski.com/blog/2015/11/19/lets-get-physical-preramble

Physically Based Shading at Disney
http://blog.selfshadow.com/publications/s2012-shading-course/burley/s2012_pbs_disney_brdf_notes_v3.pdf

http://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf

"An Inexpensive BRDF Model for Physically based Rendering" by Christophe Schlick
https://www.cs.virginia.edu/~jdl/bib/appearance/analytic%20models/schlick94b.pdf

Physically-Based Rendering in glTF 2.0 using WebGL:
https://github.com/KhronosGroup/glTF-Sample-Viewer



Unreal Engine 4:
Physically Based Materials
An overview of the primary Material inputs and how best to use them.
https://docs.unrealengine.com/en-US/Engine/Rendering/Materials/PhysicallyBased/index.html

Real Shading in Unreal Engine 4
http://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf


Unity:
Material charts
Use these charts as reference for realistic settings:
https://docs.unity3d.com/Manual/StandardShaderMaterialCharts.html

https://docs.unity3d.com/Manual/MaterialValidator.html

PBR texture compliance in War Thunder (tips for authoring and debugging PBR textures):
https://wiki.warthunder.com/PBR_texture_compliance

Reference implementations:
https://github.com/KhronosGroup/glTF-Sample-Viewer/blob/master/src/shaders/metallic-roughness.frag

https://github.com/topics/physically-based-rendering
https://github.com/topics/pbr-shading


Packing G-Buffer with PBR / Optimizations:
https://nosferalatu.com/CQ2Rendering.html
http://filmicworlds.com/blog/optimizing-ggx-update/
https://www.imgtec.com/blog/physically-based-rendering-on-a-powervr-gpu/
https://github.com/bartwronski/CSharpRenderer/blob/master/shaders/optimized-ggx.hlsl

Free PBR textures:
https://cc0textures.com/
https://freepbr.com/
https://www.poliigon.com/


BRDF Explorer:
http://www.disneyanimation.com/technology/brdf.html










/*
-----------------------------------------------------------------------------
	General Volume Rendering
-----------------------------------------------------------------------------
*/

Volume Rendering with WebGL (very good tutorial!):
https://www.willusher.io/webgl/2019/01/13/volume-rendering-with-webgl

/*
-----------------------------------------------------------------------------
	Volumetric Light Scattering/Fog/Particles
-----------------------------------------------------------------------------
*/

References:

Transparency (or Translucency) Rendering
By Alex Dunn, posted Oct 20 2014 at 06:03PM
https://developer.nvidia.com/content/transparency-or-translucency-rendering

"Volume Rendering For Games" by Simon Green
https://download.nvidia.com/developer/presentations/2005/GDC/Sponsored_Day/GDC_2005_VolumeRenderingForGames.pdf


Implementations:

Bachelor Thesis, Unity, 2019:
https://github.com/SiiMeR/unity-volumetric-fog/blob/master/Assets/Shaders/CalculateFogDensity.shader


/*
-----------------------------------------------------------------------------
	Shader/Effect/Material System Design
-----------------------------------------------------------------------------
*/

Godot:
https://docs.godotengine.org/en/3.0/tutorials/3d/spatial_material.html

Material system overview in Torque 3D game engine:
http://wiki.torque3d.org/coder:material-system-overview

Accessing and Modifying Material parameters via script
https://docs.unity3d.com/Manual/MaterialsAccessingViaScript.html






### Large Terrain Rendering





### Reverse Depth Buffer


https://www.gamedev.net/forums/topic/693404-reverse-depth-buffer/5362188/

Change the DXGI_FORMATs of the resources/SRVs/DSVs of your depth buffer, shadow maps, etc. optional
Swap the near and far plane while constructing the view-to-projection and projection-to-view transformation matrices of your camera, lights, etc.
Change the D3D11_COMPARISON_LESS_EQUAL (D3D11_COMPARISON_LESS) to D3D11_COMPARISON_GREATER_EQUAL (D3D11_COMPARISON_GREATER) of your depth-stencil and PCF sampling states.
Clear the depths of the DSVs of your depth buffer, shadow maps, etc. to 0.0f instead of 1.0f.
Negate DepthBias, SlopeScaledDepthBias and DepthBiasClamp in the rasterizer states for your shadow maps.
Set all far plane primitives to the near plane and vice versa. (Ideally using a macro for Z_FAR and Z_NEAR in your shaders.)



### Rendering first-person character weapons

Weapon FOV
http://imaginaryblend.com/2018/10/16/weapon-fov/

First person weapon with different FOV in deferred engine
https://www.gamedev.net/forums/topic/605666-first-person-weapon-with-different-fov-in-deferred-engine/4832710/

Procedural Weapon Animations
https://docs.cryengine.com/display/SDKDOC2/Procedural+Weapon+Animations


### Decals


### Level of Detail

Level of Detail (LOD) in DigitalRune engine
see LOD blending
https://digitalrune.github.io/DigitalRune-Documentation/html/b320aebd-46a0-45d8-8edb-0c717152a56b.htm
Screen-Door Transparency:
https://digitalrune.github.io/DigitalRune-Documentation/html/fa431d48-b457-4c70-a590-d44b0840ab1e.htm




### Renderer Design (Architecture) & Implementation:

(137 slides)
https://media.contentapi.ea.com/content/dam/ea/seed/presentations/wihlidal-halcyonarchitecture-notes.pdf



http://www.gamedev.ru/code/forum/?id=94243&page=2

Horde3D: The Flexible Rendering Pipeline
http://www.horde3d.org/wiki/index.php5?title=The_Flexible_Rendering_Pipeline

https://github.com/nem0/LumixEngine/wiki/Renderer

