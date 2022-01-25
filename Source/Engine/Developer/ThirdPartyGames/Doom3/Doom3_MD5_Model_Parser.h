#pragma once

#include <Developer/ThirdPartyGames/Doom3/Doom3_md5_Common.h>


//
class Lexer;
class ALineRenderer;



namespace Doom3
{

struct md5Animation;

///
struct md5Vertex
{
	V2f	uv;			// texture coordinates
	int	startWeight;	// index of the first weight affecting this vertex
	int	numWeights;		// number of weights affecting this vertex
};

/// used only if MD5_VERSION == 11 (ET:QW)
enum EVertexFlags
{
	// vertices contain color (4 bytes)
	EVF_VertexColor = BIT(0)
};
mxDECLARE_FLAGS( EVertexFlags, U32, FVertexFlagsT );

struct md5Triangle
{
	//MX_OPTIMIZE("16-bit indices for some models");
	int	indices[3];		// vertex indices
};

struct md5Weight
{
	V3f		position;	// weight's position in joint's local space
	int		jointIndex;	// joint this weight is for
	float	bias;		// bone weight, must be in range [0..1]
};

/// part of model that is associated with a single material
struct md5Mesh
{
	String64				shader;		// material to render with
	TArray< md5Vertex >		vertices;	// vertex data
	TArray< md5Triangle >	triangles;	// triangle list
	TArray< md5Weight >		weights;	// weights of vertices, they reference joints

	// this data is really not needed, used only for testing & debugging the animation system
	TArray< V3f >		bindPosePositions;	// vertices in bind-pose positions
	TArray< V3f >		bindPoseTangents;	// vertex tangents in bind-pose positions
	TArray< V3f >		bindPoseNormals;	// vertex normals in bind-pose positions

public:
	void computeBindPoseNormals();
	void computeBindPoseTangents();
};

/// MD5 model structure
struct md5Model
{
	TArray< md5Mesh >		meshes;

	/// the joints are already in object space in the bind pose;
	/// [0] - the root joint
	TArray< md5Joint >	joints;

	//// this data is really not needed, used only for testing & debugging the animation system
	TArray< M44f >	inverseBindPoseMatrices;	// inverse bind-pose matrix array

public:
	int NumMeshes() const { return meshes.num(); }
	int NumJoints() const { return joints.num(); }

	size_t Allocated() const
	{
		return joints.allocatedMemorySize() + meshes.allocatedMemorySize();
	}
	size_t SizeTotal() const
	{
		return sizeof( *this ) + Allocated();
	}
	bool IsEmpty() const
	{
		return meshes.IsEmpty() && joints.IsEmpty();
	}
	void clear()
	{
		joints.clear();
		meshes.clear();
	}

	void Dbg_DrawSkeleton( ALineRenderer& dbgDrawer, const M44f& transform, const RGBAf& color );

	void drawMeshInBindPose( ALineRenderer& dbgDrawer, const M44f& transform, const RGBAf& color );

	AABBf computeBoundingBox() const;

	//void ApplyUniformScale( float scaling_factor );
};

void Dbg_DrawSkeleton( ALineRenderer& dbgDrawer, const md5Joint* joints, int numJoints, const M44f& transform, const RGBAf& color );





ERet MD5_LoadModel(
			   const char* fileName
			   , md5Model &model
			   , const MD5LoadOptions& options = MD5LoadOptions()
			   );

ERet MD5_LoadModel(
			   Lexer& parser
			   , md5Model &model
			   , const MD5LoadOptions& options = MD5LoadOptions()
			   );

}//namespace Doom3
