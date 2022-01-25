#pragma once

#include <Base/Base.h>
#include <Base/Math/Math.h>
#include <Base/Util/Color.h>
#include <Core/Text/Lexer.h>
#include <Core/VectorMath.h>
#include <Graphics/graphics_formats.h>
#include <Graphics/graphics_utilities.h>

//#include "Animation.h"

#define MD5_VERSION_STRING		"MD5Version"
// Doom 3, including BFG edition
#define MD5_VERSION_DOOM3		10
// Enemy Territory: Quake Wars
#define MD5_VERSION_ETQW		11

namespace md5
{

// bind-pose skeleton's joint, joints make up a skeleton
struct md5_joint
{
	idQuat	orientation;	// joint's orientation quaternion
	Vec3D	position;		// joint's position in object space
	int		parentNum;		// index of the parent joint; this is the root joint if (-1)
	String32	name;

//relative to the parent bone.
//Quat	relOrientation;
//Vec3D	relPosition;

Matrix4	bindMatrix;
};

struct md5_vertex
{
	Vec2D	uv;			// texture coordinates
	int	startWeight;	// index of the first weight affecting this vertex
	int	numWeights;		// number of weights affecting this vertex
	//TArray< int >	boneIndices;
	//TArray< float >	boneWeights;
};

/// used only if MD5_VERSION == 11 (ET:QW)
enum EVertexFlags
{
	// vertices contain color (4 bytes)
	EVF_VertexColor = BIT(0)
};
mxDECLARE_FLAGS( EVertexFlags, UINT32, FVertexFlagsT );

struct md5_triangle
{
	//MX_OPTIMIZE("16-bit indices for some models");
	int	indices[3];		// vertex indices
};

struct md5_weight
{
	Vec3D	position;	// weight's position in joint's local space
	int	jointIndex;		// joint this weight is for
	FLOAT	bias;		// bone weight, must be in range [0..1]
};

/// part of model that is associated with a single material
struct Mesh
{
	String					shader;		// material to render with
	TArray< md5_vertex >	vertices;	// vertex data
	TArray< md5_triangle >	triangles;	// triangle list
	TArray< md5_weight >	weights;	// weights of vertices, they reference joints

	// this data is really not needed, used only for testing & debugging the animation system
	TArray< Vec3D >		bindPosePositions;	// vertices in bind-pose positions
	TArray< Vec3D >		transformedPositions;	// animated vertex positions
};

/// MD5 model structure
struct Model
{
	TArray< Mesh >		meshes;
	TArray< md5_joint >		joints;

	// this data is really not needed, used only for testing & debugging the animation system
	TArray< Matrix4 >	bindPose;	// bind-pose matrix array
	TArray< Matrix4 >	inverseBindPose;	// inverse bind-pose matrix array

public:
	int NumMeshes() const { return meshes.Num(); }
	int NumJoints() const { return joints.Num(); }

	size_t Allocated() const
	{
		return joints.GetAllocatedMemory() + meshes.GetAllocatedMemory();
	}
	size_t SizeTotal() const
	{
		return sizeof( *this ) + Allocated();
	};
	bool IsEmpty() const
	{
		return meshes.IsEmpty() && joints.IsEmpty();
	}
	void Clear()
	{
		joints.Clear();
		meshes.Clear();
	}

	void Dbg_DrawSkeleton( AuxRenderer& dbgDrawer, const Matrix4& transform, const RGBAf& color );
	void Dbg_DrawMesh( AuxRenderer& dbgDrawer, const Matrix4& transform, const RGBAf& color );
};

void Dbg_DrawSkeleton( AuxRenderer& dbgDrawer, const md5_joint* joints, int numJoints, const Matrix4& transform, const RGBAf& color );


struct jointAnimInfo_t
{
	String32	name;
	int			parentNum;	// index of the parent joint; this is the root joint if (-1)
	int			animBits;	// used to compute the frame's skeleton
	int			firstComponent;	// index of the first component for computing the frame's skeleton
};

// base frame joint transform
struct md5_baseFrameJoint
{
	idQuat	q;	// joint's orientation relative to the parent joint
	Vec3D	t;	// joint's position relative to the parent joint
	int		pad;// so that it's binary compatible with md5_joint
};

typedef TArray< md5_joint >	md5_skeleton;

/// Animation data
struct Animation
{
	int		numFrames;	// number of frames of the animation
	int		numJoints;	// number of joints of the frame skeletons
	int		frameRate;	// number of frames per second to draw for the animation
	int		numAnimatedComponents;	// number of parameters per frame used to compute the frame skeletons

	TArray< jointAnimInfo_t >	jointInfo;	// hierarchy information
	TArray< AABB24 >			bounds;		// bounding boxes for each frame

	TArray< md5_baseFrameJoint >	baseFrame;	// position and orientation of each joint from which the frame skeletons will be built
	TArray< float >					componentFrames;	// parameters used to compute the frame's skeleton

	// built from the above data
	TArray< md5_skeleton >		frameSkeletons;

public:
	int NumJoints() const { return jointInfo.Num(); }

	void Clear()
	{
		numFrames = 0;
		numJoints = 0;
		frameRate = 0;
		numAnimatedComponents = 0;

		jointInfo.Empty();
		bounds.Empty();
		baseFrame.Empty();
		componentFrames.Empty();
	}
};

bool CheckModelAnim( const Model& model, const Animation& anim );

struct frameBlend_t
{
	int		cycleCount;	// how many times the anim has wrapped to the beginning (0 for clamped anims)
	int		frame1;
	int		frame2;
	float	frontlerp;
	float	backlerp;
};

// bit flags which tell you how to compute the skeleton of a frame for a joint
//
#define	ANIM_TX				BIT( 0 )
#define	ANIM_TY				BIT( 1 )
#define	ANIM_TZ				BIT( 2 )
#define	ANIM_QX				BIT( 3 )
#define	ANIM_QY				BIT( 4 )
#define	ANIM_QZ				BIT( 5 )

enum ESkinningTechnique
{
	Hardware_Skinning,
	Software_Skinning,
};

/*
-----------------------------------------------------------------------------
	SkeletalMesh
-----------------------------------------------------------------------------
*/
class md5SkeletalMesh
{
public:
	md5SkeletalMesh();

	void SetModel( Model* md5Model );
	void SetAnim( Animation* md5Anim );

	void Animate( FLOAT deltaSeconds );

	void Clear();

	void Dbg_Draw( AuxRenderer& dbgDrawer, const Matrix4& transform, const RGBAf& color );
	void Dbg_DrawSkeleton( AuxRenderer& dbgDrawer, const Matrix4& transform, const RGBAf& color );

private:
	void UpdateMesh();

	void UpdateGPUBuffers();

public:
	TPtr< Model >	model;
	TPtr< Animation >	anim;

	float	m_iFrameRate;		// number of frames per second
	float	m_fFrameDuration;	// seconds per frame
	float	m_fAnimDuration;	// total time of current animation, in seconds
	float	m_fAnimTime;		// current animation time, in seconds
	int		m_iNumFrames;

	float	m_currentTime;

	md5_skeleton	animatedSkeleton;

	AABB24	localBounds;	// object-space bounds
};

//void SetupSkinnedModel( const SkeletalMesh& skeletalMesh, mxSkinnedModel &skinnedModel );




struct BoneIndices
{
	UINT8	_[4];	// MAX_BONES_PER_VERTEX
};
struct BoneWeights
{
	FLOAT	_[4];	// MAX_BONES_PER_VERTEX
};


//void TempHack_DrawSkinnedModel( mxSkinnedModel& skinnedModel, const RGBAf& color );

struct SoftwareMesh
{
	//
};

ERet LoadMd5Mesh( const char* _file, SoftwareMesh &_mesh );

ERet LoadModel( const char* fileName, Model &model );
ERet LoadModel( Lexer& parser, Model &model );

ERet LoadAnim( const char* fileName, Animation &anim );
ERet LoadAnim( const void* data, size_t size, Animation &anim );

//ERet MySkeleton_FromMd5Mesh( const md5_model& _md5model, rxSkeleton &_skel );
//ERet MyAnimClip_FromMd5Anim( const md5_anim& _md5anim, AnimClip &_animClip );

}//namespace md5
