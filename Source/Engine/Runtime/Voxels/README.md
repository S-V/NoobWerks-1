# Voxel Engine Source Code structure

The source code of the Voxel Engine is structured in the following folders:

 * Common - database.
 * DataChunking - chunk format.
 * LevelOfDetail - LoD generation and stitching.
 * Meshing - based on Dual Contouring.
 * Physics - support for physicalizing terrain chunks.
 * Scene - scene management.
 * Utility - _ .
 
## Compiling the Voxel Engine


## BUGS
- if only root is visible, regenerateworld() -> BUG!
- if remesh job is failed, remeshing flag is not reset!


## TODO

- OPTIMIZE LOD REBUILD: SoA2_SortedMap:

Flags - U32
CoarseChunk*

- the set of chunk resolutions is small (8, 16, 32, 64) - can template-specialize perf-crit loops!

- we restrict the number of cores used per machine to 8 for our runs. 

- We bound memory.
 When we execute a process, we fix the amount of memory it can allocate, including not just the heap but even the executable image itself. We guarantee such memory has been allocated so features such as the opportunistic memory allocation strategy used in Linux cannot cause strange out of memory errors. Even the dynamic allocation of things like low-level network buffers comes from the memory allocated to a user process ensuring that our process execution mechanism never needs to perform the (possibly failing) dynamic allocation itself.

- store remeshing flags separately, clear before each frame

- store colors

- aperiodic texturing

- all sorting methods, e.g. Sorting::InsertionSort_Keys_and_Values2,
should assert count > 1, else crashes!!!

- rename vx_baked_mesh_data to ChunkResultData
- ChunkInfo should have different flags for LoD 0 and coarse LODs.

- get rid of all global variables (e.g. "static NullWorldStorage	null_world_storage")

- ChunkKey can be built from ChunkID_Ext by reversing bits?!

- no way to return error codes from jobs


## TODO: Tools
- Cubify brush - Minecraft look
- Erode brush - ridges simulation (?)
