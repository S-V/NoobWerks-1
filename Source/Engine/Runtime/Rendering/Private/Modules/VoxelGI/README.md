# Global Illumination

Volumetric Grid for occlusion testing / GI effects on GPU.

# How it works

- static scene representation - SDF brick atlas, 8-bit (UNORM8).
In each brick/chunk, distances are normalized to [-1..1] range.


# TODO
- allocate bricks of different size, based on information density/curvature/etc.



# References:

### 'Brickmap' for fast ray tracing:

A Voxel Renderer for Learning C/C++
The voxels will be stored in a two-level grid, which consists of a coarse 128³ top-level grid, which stores 8³ voxel bricks for a total 1024³ resolution.
https://jacco.ompf2.com/2021/02/01/a-voxel-renderer-for-learning-c-c/

Sparse grid for ray marching sdfs. (Aug 1, 2021)
https://twitter.com/TheKristofLovas/status/1421638769296412672



NVScene 2014 Session: A Really Realtime Raytracer (Matt Swoboda)
https://www.youtube.com/watch?v=JSr6wvkvgM0

Tracing a 64x64x64 brickmap and 4x4x4 bricks for an effective resolution of 256x256x256.
https://directtovideo.wordpress.com/2013/05/08/real-time-ray-tracing-part-2/
https://directtovideo.wordpress.com/2013/05/07/real-time-ray-tracing/



### BIH on GPU - said to be slower than BVH

BIH Really that slow? (by Vilem Otte November 28, 2011)
https://www.gamedev.net/forums/topic/615878-bih-really-that-slow/

BIH with voxel grids in leaf nodes
GPU Volume Raycasting using Bounding Interval Hierarchies
https://www.cg.tuwien.ac.at/research/publications/2009/Kinkelin_2009/Kinkelin_2009-Paper.pdf



### The high-res "shadow"/opacity texture with 1 bit per voxel

From screen space to voxel space
http://blog.tuxedolabs.com/2018/10/17/from-screen-space-to-voxel-space.html



How is SDF stored in a octree?
https://www.reddit.com/r/VoxelGameDev/comments/ontjdf/how_is_sdf_stored_in_a_octree/h5vrbea/


https://blog.roblox.com/2013/02/dynamic-lighting-and-shadows-the-voxel-solution/



### Voxelization on GPU

GPU Pro 3
02_Practical Binary Surface and Solid Voxelization with Direct3D 11


### Signed Distance Field (SDF)

A lot of links:
https://github.com/CedricGuillemet/SDF

https://dercuano.github.io/notes/interval-raymarching.html

https://twitter.com/unbound_io



#### SDF soft shadows research:

softshadow comparison (Created by stduhpf in 2020-06-01)
https://www.shadertoy.com/view/tdSBDw

Better SDF soft shadows (Created by stduhpf in 2019-08-27)
https://www.shadertoy.com/view/3tBSWG

Soft Shadow Variation inward (Created by stduhpf in 2019-08-27)
https://www.shadertoy.com/view/3tBXDy


https://docs.unrealengine.com/4.27/en-US/BuildingWorlds/LightingAndShadows/RayTracedDistanceFieldShadowing/

#### SDF generation from triangle meshes:

explanations on how to find the distance from a point to a triangle, etc.
https://github.com/iamyoukou/sdf3d






## Real-Time Ray-Tracing on GPU


http://raytracey.blogspot.com/2016/01/gpu-path-tracing-tutorial-3-take-your.html

### BVH in Cycles, Blender's GPU path tracing renderer
https://wiki.blender.org/wiki/Source/Render/Cycles/BVH

### Quantitative Analysis of Voxel Raytracing Acceleration Structures [2014]
https://diglib.eg.org/bitstream/handle/10.2312/pgs.20141257.085-090/085-090.pdf?sequence=1&isAllowed=y




## Real-Time Global Illumination

### irradiance_probes.md
https://gist.github.com/pixelmager/c0506018d2a42b2d7a3403f66d17bdac


### Computer Graphics Group (Athens University of Economics and Business)

Papers:
http://graphics.cs.aueb.gr/graphics/research_illumination.html


### Dynamic Diffuse Global Illumination (DDGI)

Dynamic Diffuse Global Illumination with Ray-Traced Irradiance Fields [2019]
https://morgan3d.github.io/articles/2019-04-01-ddgi/overview.html

Precomputed Light Field Probes [2017]

An Engineer's Guide to Integrating Ray Traced Irradiance Fields
25 Apr 2019
https://www.youtube.com/watch?v=LRWWa4SwKuw


[Oct 23, 2020] Implementing DDGI using Vulkan Ray-Tracing extension. 
32x32x32 irradiance probes are updated each frame with 128 rays and result packed into cubemap with just 1 pixel per face for each probe.
Source code here: https://github.com/zakarumych/wilds
https://twitter.com/zakarum4/status/1319639941513662464?lang=en


## SDFGI

Signed Distance Fields Dynamic Diffuse Global Illumination
Uses SDF primitives to represent the scene - impractical?
https://www.arxiv-vanity.com/papers/2007.14394/
