#include "Base/Base.h"
#pragma hdrstop

#include <Core/Serialization/Serialization.h>
#include <Core/Serialization/Text/TxTSerializers.h>
#include <Core/Serialization/Text/TxTWriter.h>

#include <ImGui/imgui.h>
#include <ImGuizmo/ImGuizmo.h>

#include <VoxelsSupport/MeshStamp.h>

#include <BlobTree/BlobTree.h>


namespace SDF {

mxDEFINE_CLASS(Transform);
mxBEGIN_REFLECTION(Transform)
	mxMEMBER_FIELD( scaling ),	
	mxMEMBER_FIELD( translation ),
	mxMEMBER_FIELD( rotation_angles_deg ),
mxEND_REFLECTION;

mxBEGIN_REFLECT_ENUM( ShapeType8 )
	mxREFLECT_ENUM_ITEM( PLANE			,	ShapeType::SHAPE_PLANE			),
	mxREFLECT_ENUM_ITEM( SPHERE			,	ShapeType::SHAPE_SPHERE			),
	mxREFLECT_ENUM_ITEM( BOX			,	ShapeType::SHAPE_BOX			),
	mxREFLECT_ENUM_ITEM( INF_CYLINDER	,	ShapeType::SHAPE_INF_CYLINDER	),
	mxREFLECT_ENUM_ITEM( CYLINDER		,	ShapeType::SHAPE_CYLINDER		),
	mxREFLECT_ENUM_ITEM( TORUS			,	ShapeType::SHAPE_TORUS			),
	mxREFLECT_ENUM_ITEM( SINE_WAVE		,	ShapeType::SHAPE_SINE_WAVE		),
	mxREFLECT_ENUM_ITEM( GYROID			,	ShapeType::SHAPE_GYROID			),

	mxREFLECT_ENUM_ITEM( MESH			,	ShapeType::SHAPE_MESH			),
mxEND_REFLECT_ENUM

mxDEFINE_CLASS(MeshShape);
mxBEGIN_REFLECTION(MeshShape)
	mxMEMBER_FIELD( transform ),
mxEND_REFLECTION;

//tbPRINT_SIZE_OF(Shape);
mxDEFINE_CLASS(Shape);
mxBEGIN_REFLECTION(Shape)
	mxMEMBER_FIELD( f ),
mxEND_REFLECTION;

mxBEGIN_REFLECT_ENUM( OpType8 )
	mxREFLECT_ENUM_ITEM( CSG_ADD, OpType::CSG_ADD ),
	//mxREFLECT_ENUM_ITEM( CSG_SUB, OpType::CSG_SUB ),
mxEND_REFLECT_ENUM

Tape::Tape(AllocatorI& allocator)
	: _edits_list(allocator)
{
}

//ERet Tape::AddEditOp(
//	const OpType op_type
//	, const SDF::Shape& shape
//	, const ShapeType& shape_type
//	, const MaterialID material_id
//	)
//{
//	DBGOUT("Adding op '%s'...",
//		OpType8_Type().GetString(op_type)
//		);
//
//	//
//	EditOp	new_edit_op;
//	new_edit_op.op_type = op_type;
//	new_edit_op.shape_type = shape_type;
//	new_edit_op.material_id = material_id;
//	new_edit_op.shape_index = shapes.num();
//
//	mxDO(shapes.add(shape));
//	mxDO(edit_ops.add(new_edit_op));
//
//	return ALL_OK;
//}

static ERet ApplyEdit_StampMesh(
								const AssetID& mesh_id
								, const Transform& transform
								, const MaterialID material_id
								, NwClump& assets_storage_clump
								, VX5::WorldI* voxel_world
								, AABBf *changed_region_aabb_ = nil
								)
{
	DBGOUT("[EDIT] Stamping mesh '%s'...",
		mesh_id.c_str()
		);

	TResPtr<NwMesh>	mesh_ptr(mesh_id);

	mxTRY(VXExt::LoadMeshForStamping(
		mesh_ptr
		, assets_storage_clump
		));

	mxENSURE(
		mesh_ptr->vertex_format == &MeshStampVertex::metaClass()
		, ERR_INVALID_PARAMETER
		, "Only meshes with MeshStampVertex vertex format can be voxelized!"
		);

	//
	const M44f	local_to_world_matrix = RecomposeMatrix(transform);

	//
	VXTriangleMeshWrapper	vx_trimesh_wrapper(mesh_ptr);

	voxel_world->StampMesh(
		&vx_trimesh_wrapper
		, local_to_world_matrix
		, material_id
		);

	const AABBf transformed_mesh_aabb_in_world_space
		= mesh_ptr->local_aabb
		.transformed(local_to_world_matrix)
		;

	if(changed_region_aabb_) {
		*changed_region_aabb_ = transformed_mesh_aabb_in_world_space;
	}

	return ALL_OK;
}

ERet Tape::AddMesh(
			 const AssetID& mesh_id
			 , const Transform& transform
			 , const MaterialID material_id
			 , NwClump& assets_storage_clump
			 , VX5::WorldI* voxel_world
			 , AABBf *changed_region_aabb_ /*= nil*/
			 )
{
	DBGOUT("[EDIT] Recording mesh '%s'...",
		mesh_id.c_str()
		);

	//
	EditOp	new_edit_op;
	new_edit_op.op_type = CSG_ADD;
	new_edit_op.shape_type = SHAPE_MESH;
	new_edit_op.material_id = material_id;

	new_edit_op.mesh_id = mesh_id;
	new_edit_op.shape.mesh.transform = transform;

	//
	mxDO(_edits_list.add(new_edit_op));

	//
	mxDO(ApplyEdit_StampMesh(
		mesh_id
		, transform
		, material_id
		, assets_storage_clump
		, voxel_world
		, changed_region_aabb_
		));

	return ALL_OK;
}

ERet Tape::UndoLastEdit(
						AABBf *changed_region_aabb_ /*= nil*/
						)
{
	if(_edits_list.isEmpty())
	{
		DBGOUT("Cannot undo - empty tree!");
		return ERR_OBJECT_NOT_FOUND;
	}

	DBGOUT("Undoing last edit...");

	EditOp undone_edit_op = _edits_list.PopLastValue();
	UNDONE;

	return ALL_OK;
}

ERet Tape::ApplyEditsToWorld(
	VX5::WorldI* voxel_world
	, NwClump& assets_storage_clump
	) const
{
	nwFOR_EACH(const EditOp& edit_op, _edits_list)
	{
		switch(edit_op.op_type)
		{
		case CSG_ADD:
			switch(edit_op.shape_type)
			{
			case SHAPE_MESH:
				mxDO(ApplyEdit_StampMesh(
					edit_op.mesh_id
					, edit_op.shape.mesh.transform
					, edit_op.material_id
					, assets_storage_clump
					, voxel_world
					));
				break;

			default:
				mxENSURE(false, ERR_UNSUPPORTED_FEATURE, "");
			}
			break;

		default:
			mxENSURE(false, ERR_UNSUPPORTED_FEATURE, "");
		}
		
	}//for each edit op

	return ALL_OK;
}

void DecomposeMatrixToComponents(
								 const M44f& matrix
								 , V3f &translation_
								 , V3f &rotation_angles_deg_
								 , V3f &scaling_
								 )
{
	ImGuizmo::DecomposeMatrixToComponents(
		matrix.a,
		translation_.a,
		rotation_angles_deg_.a,
		scaling_.a
		);
}

M44f RecomposeMatrix(
					 const V3f& translation
					 , const V3f& rotation_angles_deg
					 , const V3f& scaling
					 )
{
	M44f	matrix;
	ImGuizmo::RecomposeMatrixFromComponents(
		translation.a,
		rotation_angles_deg.a,
		scaling.a,
		matrix.a
		);
	return matrix;
}

}//namespace SDF
