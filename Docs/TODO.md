
- normals in world space!
use octa 10:10 - 20 bits enough!
https://knarkowicz.wordpress.com/2014/04/16/octahedron-normal-vector-encoding/

I was using this for encoding normals into two components of a 10:10:10:2 GBuffer format and I get great results but some shaders did see a noticeable performance degradation due to the increased decode cost over vanilla XYZ format.
https://johnwhite3d.blogspot.com/2017/10/signed-octahedron-normal-encoding.html



- allocate G_PerObject from CB pool ?



chunk resolution is configurable at run-time




voxel engine examples:

1) Hello, world!

- proc gen

- heightmap importing

- changing resolution at runtime

- CSG editing

- mesh stamping

- undo/redo

- rigid body physics





Engine features

- sparse SDF (brickmap) with max-norm directional distance field for skipping empty space
- SDF-based ray-casted occlusion culling (on CPU)



use strpool and other libs from
https://github.com/mattiasgustavsson/libs/