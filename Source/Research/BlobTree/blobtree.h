// Signed Distance Fields primitives.
// SIMD-Accelerated SDF/BlobTree evaluation.
#pragma once

#include <Base/Math/MiniMath.h>
#include <Base/Memory/FreeList/FreeList.h>

#include <Voxels/public/vx5.h>
#include <VoxelsSupport/VoxelTerrainMaterials.h>


struct lua_State;

namespace SDF {

///
typedef VoxMatID	MaterialID;

///
struct Transform: CStruct
{
	V3f		scaling;
	V3f		translation;
	V3f		rotation_angles_deg;
public:
	mxDECLARE_CLASS(Transform,CStruct);
	mxDECLARE_REFLECTION;
};

///=== Primitives (Leaf nodes):
enum ShapeType: int
{
	SHAPE_PLANE,
	SHAPE_SPHERE,
	SHAPE_CUBE,
	SHAPE_BOX,
	SHAPE_INF_CYLINDER,	//!< infinite vertical cylinder (Z-up)
	SHAPE_CYLINDER,	//!< capped vertical cylinder
	SHAPE_TORUS,	//!< torus in the XY-plane
	SHAPE_SINE_WAVE,	//!< concentric waves with a 'mountain' at the center
	SHAPE_GYROID,

	/// triangle mesh
	SHAPE_MESH,

	SHAPE_LAST_PRIM = SHAPE_SPHERE,

	SHAPE_TRANSFORM,	//!< not used in optimized (linearized) blob tree
};
mxDECLARE_ENUM( ShapeType, U8, ShapeType8 );



struct MeshShape: CStruct
{
	Transform	transform;
public:
	mxDECLARE_CLASS(MeshShape,CStruct);
	mxDECLARE_REFLECTION;
};


/// Primitive
mxPREALIGN(16) struct Shape: CStruct
{
	union
	{
		// serialized as raw floats :(
		float	f[16];

		struct {
			V4f		normal_and_distance;
		} plane;

		struct {
			V3f		center;
			float	radius;
		} sphere;

		struct {
			V3f		center;
			F32		half_size;
		} cube;

		struct {
			V3f		center;
			V3f		extent;	// half size
		} box;

		struct {
			V3f		origin;
			float	radius;
		} inf_cylinder;

		// Torus in the XY-plane
		struct {
			V3f		center;
			float	big_radius;		//!< the radius of the big circle (around the center)
			float	small_radius;	//!< the radius of the small revolving circle
		} torus;

		struct {
			V3f		direction;
			float	radius;
			V3f		origin;
			float	height;	//!< half-height, starting from the origin
		} cylinder;

		struct {
			V3f		coord_scale;
		} sine_wave;

		struct {
			V3f		coord_scale;
		} gyroid;


		struct {
			Transform	world_transform;
			char		name[32];
		} mesh;
	};
public:
	mxDECLARE_CLASS(Shape,CStruct);
	mxDECLARE_REFLECTION;
};
//ASSERT_SIZEOF(Shape, 64);

///
enum OpType: int
{
	// CSG
	CSG_ADD,	//!< Set-theoretic union
	//CSG_SUB,	//!< Set-theoretic subtraction (difference): A \ B
	//SHAPE_COMPLEMENT,
	//SHAPE_INTERSECTION,	//!< Set-theoretic intersection
	//MT_PRODUCT,	//!< Set-theoretic Cartesian product
};
mxDECLARE_ENUM( OpType, U8, OpType8 );


/// Represents a single Editing operation.
struct EditOp
{
	OpType8		op_type;
	ShapeType8	shape_type;

	/// The material of the primitive or the editing brush (depends on type).
	MaterialID	material_id;

	// should be stored in MeshShape, cannot be placed into union
	AssetID		mesh_id;

	union
	{
		MeshShape	mesh;
	} shape;

public:
	EditOp()
	{
		op_type = CSG_ADD;
		shape_type = SHAPE_PLANE;

		material_id = VoxMat_Ground;
	}
};


/// A 'tape' with recorded editing operations.
class Tape: NwNonCopyable
{
	DynamicArray< EditOp >	_edits_list;

public:
	Tape(AllocatorI& allocator);

	ERet AddMesh(
		const AssetID& mesh_id
		, const Transform& transform
		, const MaterialID material_id
		, NwClump& assets_storage_clump
		, VX5::WorldI* voxel_world
		, AABBf *changed_region_aabb_ = nil
		);

	ERet UndoLastEdit(
		AABBf *changed_region_aabb_ = nil
		);

	//ERet AddEditOp(
	//	const OpType op_type
	//	, const Shape& shape
	//	, const ShapeType& shape_type
	//	, const MaterialID material_id
	//	);

	ERet ApplyEditsToWorld(
		VX5::WorldI* voxel_world
		, NwClump& assets_storage_clump
		) const;

public:
	ERet SaveToFile(const char* filename) const;
	ERet LoadFromFile(const char* filename);
};

mxSTOLEN("ImGuizmo");
// helper functions for manualy editing translation/rotation/scale with an input float
// translation, rotation and scale float points to 3 floats each
// Angles are in degrees (more suitable for human editing)
// example:
// float matrixTranslation[3], matrixRotation[3], matrixScale[3];
// ImGuizmo::DecomposeMatrixToComponents(gizmoMatrix.m16, matrixTranslation, matrixRotation, matrixScale);
// ImGui::InputFloat3("Tr", matrixTranslation, 3);
// ImGui::InputFloat3("Rt", matrixRotation, 3);
// ImGui::InputFloat3("Sc", matrixScale, 3);
// ImGuizmo::RecomposeMatrixFromComponents(matrixTranslation, matrixRotation, matrixScale, gizmoMatrix.m16);
//
// These functions have some numerical stability issues for now. Use with caution.
void DecomposeMatrix(
					 const M44f& matrix
					 , V3f &translation_
					 , V3f &rotation_angles_deg_
					 , V3f &scaling_
					 );
M44f RecomposeMatrix(
					 const V3f& translation
					 , const V3f& rotation_angles_deg
					 , const V3f& scaling
					 );

inline
M44f RecomposeMatrix(
					 const Transform& transform
					 )
{
	return RecomposeMatrix(
					 transform.translation
					 , transform.rotation_angles_deg
					 , transform.scaling
					 );
}

}//namespace SDF
