//NOTE: 'Tc' stands for 'Toolchain'
#pragma once

#include <optional-lite/optional.hpp>
#include <Base/Template/Containers/Array/DynamicArray.h>
#include <Core/Assets/AssetManagement.h>
#include <Graphics/Public/graphics_formats.h>
#include <Rendering/Public/Core/VertexFormats.h>

class BitArray;

// most models have less than 80 bones, so even a 8-bit integer will suffice
typedef UINT16 BoneIndex;

enum EAnimType
{
	ANIMATION_NODE,
	ANIMATION_MORPHTARGET,
	ANIMATION_SKELETAL,
	ANIMATION_FACIAL,
};

enum EAnimMethod
{
	ANIM_ONESHOT,
	ANIM_ONELOOP,
	ANIM_LOOP,
	ANIM_PINGPONG,
	ANIM_PINGPONG_LOOP,
};

enum EAnimMode
{
	ANIM_FORWARDS   = 1,
	ANIM_STOP       = 0,
	ANIM_BACKWARDS  = -1
};

enum EAnimBehavior
{
	/** The value from the default node transformation is taken*/
	AnimBehavior_DEFAULT  = 0x0,  

	/** The nearest key value is used without interpolation */
	AnimBehavior_CONSTANT = 0x1,

	/** The value of the nearest two keys is linearly
	*  extrapolated for the current time value.*/
	AnimBehavior_LINEAR   = 0x2,

	/** The animation is repeated.
	*
	*  If the animation key go from n to m and the current
	*  time is t, use the value at (t-n) % (|m-n|).*/
	AnimBehavior_REPEAT   = 0x3	
};


/*
=======================================================================
	GENERIC MESH FORMAT (FAT & SLOW - FOR TOOLS ONLY)
=======================================================================
*/

/*
-----------------------------------------------------------------------------
	BoneWeight
-----------------------------------------------------------------------------
*/
struct TcWeight: CStruct
{
	// Index of bone influencing the vertex.
	int		boneIndex;
	// The strength of the influence in the range (0...1).
	// The influence from all bones at one vertex amounts to 1.
	float	boneWeight;

public:
	mxDECLARE_CLASS( TcWeight, CStruct );
	mxDECLARE_REFLECTION;
	TcWeight();
};

typedef TFixedArray< TcWeight, MAX_INFLUENCES >	TcWeights;

/*
-----------------------------------------------------------------------------
	MeshBone (aka Joint) - The bone structure as seen by the toolchain.
	Represents one bone in the skeleton of a mesh.
	All bones (with the exception of the root) are oriented with respect to their parent.
		You multiply each bone's matrix by it's parent's matrix
	(which has been multiplied by it's parent, etc., all the way back to the root)
	to calculate the bone's absolute transform (in mesh-object space).
		Skin offset matrices are applied to the bone's combined transform
	to (eventually) transform the mesh vertices from mesh space to bone space.
-----------------------------------------------------------------------------
*/
struct TcBone: CStruct
{
	String		name;			// The name of the bone from the toolchain
	int			parent;			// array index of parent bone; -1 means there is no parent
	Vector4		rotation;		// The joint's rotation relative to the parent joint
	V3f			translation;	// The joint's position relative to the parent joint
public:
	mxDECLARE_CLASS( TcBone, CStruct );
	mxDECLARE_REFLECTION;
	TcBone();
};

// Skeleton/Armature/JointTree
// which can be deformed by an animation clip
struct TcSkeleton: CStruct
{
	TArray< TcBone >	bones;	// the first bone is always the root
public:
	mxDECLARE_CLASS( TcSkeleton, CStruct );
	mxDECLARE_REFLECTION;
	int FindBoneIndexByName( const char* boneName ) const;
};


///
struct Tb_TextureView
{
	AssetID			id;

	////NOTE: the data is owned by cgltf!
	//char *		data;
	//U32			size;

	TArray< BYTE >	data;

public:
	//bool IsValid() const { return data && size; }

	bool IsValid() const { return !data.IsEmpty(); }
};

/// A set of parameter values that are used to define the metallic-roughness material model
/// from Physically-Based Rendering (PBR) methodology.
/// https://github.com/KhronosGroup/glTF/blob/master/specification/2.0/README.md#metallic-roughness-material
/// https://microsoft.github.io/MixedRealityToolkit-Unity/api/Microsoft.MixedReality.Toolkit.Utilities.Gltf.Schema.GltfPbrMetallicRoughness.html
///
struct Tb_pbr_metallic_roughness
{
	/// The base color has two different interpretations depending on the value of metalness.
	/// When the material is a metal, the base color is the specific measured reflectance value at normal incidence (F0).
	/// For a non-metal the base color represents the reflected diffuse color of the material.
	Tb_TextureView base_color_texture;

	/// metallicRoughnessTexture: Texture containing both the metallic value in the B channel
	/// and the roughness value in the G channel to keep better precision.
	/// Ambient occlusion can also be saved in R channel.
	Tb_TextureView metallic_roughness_texture;

	float base_color_factor[4];
	float metallic_factor;
	float roughness_factor;
};

struct Tb_pbr_specular_glossiness
{
	Tb_TextureView diffuse_texture;
	Tb_TextureView specular_glossiness_texture;

	float diffuse_factor[4];
	float specular_factor[3];
	float glossiness_factor;
};

//struct Tb_clearcoat
//{
//	Tb_TextureView clearcoat_texture;
//	Tb_TextureView clearcoat_roughness_texture;
//	Tb_TextureView clearcoat_normal_texture;
//
//	float clearcoat_factor;
//	float clearcoat_roughness_factor;
//};

/// based on glTF PBR materials
struct TcMaterial: ReferenceCounted
{
	typedef TRefPtr< TcMaterial >	Ref;

public:

	AssetID		name;

	Tb_TextureView 		normal_texture;
	Tb_TextureView 		occlusion_texture;
	Tb_TextureView 		emissive_texture;

	nonstd::optional<Tb_pbr_metallic_roughness>		pbr_metallic_roughness;
	nonstd::optional<Tb_pbr_specular_glossiness>	pbr_specular_glossiness;
	//nonstd::optional<Tb_clearcoat>					clearcoat;

	//float emissive_factor[3];
	//Tb_alpha_mode alpha_mode;
	//float alpha_cutoff;
	//Tb_bool double_sided;
	//Tb_bool unlit;
	//Tb_extras extras;
};



/*
-----------------------------------------------------------------------------
	Submesh (aka Mesh Subset, Mesh Part, Vertex-Index Range)
	represents a part of the mesh
	which is typically associated with a single material
-----------------------------------------------------------------------------
*/
struct TcTriMesh
	: ReferenceCounted
	, NonCopyable
{
	typedef TRefPtr< TcTriMesh >	Ref;

public:

	String32					name;	// optional

	// vertex data in SoA layout
	DynamicArray< V3f >			positions;	// always present
	DynamicArray< V2f >			texCoords;
	DynamicArray< V3f >			tangents;
	DynamicArray< V3f >			binormals;	// 'bitangents'
	DynamicArray< V3f >			normals;
	DynamicArray< V4f >			colors;		// vertex colors (RGBA)
	DynamicArray< TcWeights >	weights;

	// index data - triangle list
	DynamicArray< U32 >			indices;	// always 32-bit indices

	AABBf						aabb;	// local-space bounding box
	//Sphere					sphere;	// local-space bounding sphere

	TRefPtr< TcMaterial >		material;	// default material

public:
	//mxDECLARE_CLASS( TcTriMesh, CStruct );
	//mxDECLARE_REFLECTION;

	TcTriMesh( AllocatorI & _allocator );

	U32 NumVertices() const { return positions.num(); }
	U32 NumIndices() const { return indices.num(); }

	void transformVerticesBy( const M44f& transform_matrix );

	ERet CopyFrom(const TcTriMesh& other);

	//// Export surfaces in .OBJ file format.
	ERet Debug_SaveToObj( const char* filename ) const;
};

// Key - A set of data points defining the position and rotation of an individual bone at a specific time.

// A time-value pair specifying a certain 3D vector for the given time.
struct TcVecKey: CStruct {
	V3f	data;	// holds the data for this key frame
	float	time;	// time stamp
public:
	mxDECLARE_CLASS( TcVecKey, CStruct );
	mxDECLARE_REFLECTION;
};
// A time-value pair specifying a rotation for the given time. 
struct TcQuatKey: CStruct {
	V4f	data;	// holds the data for this key frame
	float	time;
public:
	mxDECLARE_CLASS( TcQuatKey, CStruct );
	mxDECLARE_REFLECTION;
};

/*
-----------------------------------------------------------------------------
	AnimationChannel/AnimTrack/BoneTrack/AnimCurve
	It is a set of keys describing the motion an individual bone over time.
	Stores frame time stamp and value for each component of the transformation.
-----------------------------------------------------------------------------
*/
struct TcAnimChannel: CStruct
{
	String					target;			// e.g. name of the mesh bone
	TArray< TcVecKey >		positionKeys;	// translation key frames
	TArray< TcQuatKey >		rotationKeys;	// rotation key frames
	TArray< TcVecKey >		scalingKeys;	// scaling key frames
public:
	mxDECLARE_CLASS( TcAnimChannel, CStruct );
	mxDECLARE_REFLECTION;
};

// Animation Clip/Track/Stack(in FBX terminology)
// AnimSequence in Unreal Engine parlance:
// A set of Tracks defining the motion of all the bones making up an entire skeleton
// (e.g. a run cycle, attack)
struct TcAnimation: CStruct
{
	String	name;
	TArray< TcAnimChannel >	channels;
	float	duration;	// Duration of the animation in seconds.
	int		numFrames;	// Number of frames.
public:
	mxDECLARE_CLASS( TcAnimation, CStruct );
	mxDECLARE_REFLECTION;
	TcAnimation();
	const TcAnimChannel* FindChannelByName( const char* name ) const;
};

/*
-----------------------------------------------------------------------------
	TcModel
	TcModel is a useful interchange format,
	which is usually created by an asset importer.

	It's mainly used for storing asset data in intermediate format,
	it cannot be directly loaded by the engine.

	The intermediate format is either a standard format like COLLADA
	or a custom defined format that is easy for our tools to read.
	This file is exported from the content creation tool
	and stores all of the information we could possibly desire
	about the asset, both now and in the future.

	Since the intermediate format is designed to be general and easy to read,
	this intermediate format isn't optimized for space or fast loading in-game.
	To create the game-ready format, we compile the intermediate format
	into a game format.
-----------------------------------------------------------------------------
*/
class TcModel: NonCopyable
{
public:
	// parts of sets typically associated with a single material
	DynamicArray< TcTriMesh::Ref >	meshes;

	// local-space bounding box of the mesh (in bind pose)
	AABBf				bounds;

	// skinning data for skeletal animation
	TcSkeleton			skeleton;

	// A set of related animations which all play on the this skeleton.
	DynamicArray< TcAnimation >	animations;

	AllocatorI &	allocator;

public:
	TcModel( AllocatorI & _allocator );

	//// Export surfaces in .OBJ file format.
	ERet Debug_SaveToObj( const char* filename ) const;

	void transformVertices( const M44f& affine_matrix );

	void RecomputeAABB();
};

void CalculateTotalVertexIndexCount( const TcModel& data, UINT &vertexCount, UINT &index_count );

namespace MeshLib
{

ERet CompileMesh(
				 const TcModel& src
				 , const TbVertexFormat& vertex_format
				 , TbRawMeshData &dst
				 );

}//namespace MeshLib
