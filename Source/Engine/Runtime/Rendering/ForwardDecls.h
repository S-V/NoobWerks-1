#pragma once


class NwClump;
class ClumpList;


struct NwTime;


namespace NGpu
{
	class NwRenderContext;
}

//
class NwShaderEffect;
class ViewFrustum;
class DrawVertex;
class ADebugDraw;

class NwTexture;


//
namespace Rendering
{

struct NwFloatingOrigin;
struct NwCameraView;

//
class NwMesh;
struct RrTransform;
class MeshInstance;
class Material;
class RrDirectionalLight;


//
class SpatialDatabaseI;
struct RenderEntityBase;
struct RenderEntityList;
struct RenderCallbacks;
struct RenderCallbackParameters;
struct VoxelizationCallbackParameters;


class RenderPath;
struct RrGlobalSettings;



namespace VXGI {
	class VoxelGrids;
	class VoxelBVH;
	class IrradianceField;
}//namespace VXGI

}//namespace Rendering
