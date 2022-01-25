#include "Base/Base.h"
#pragma hdrstop

#include <Core/Serialization/Serialization.h>
#include <Core/Serialization/Text/TxTSerializers.h>
#include <Core/Serialization/Text/TxTWriter.h>

#include <BlobTree/BlobTree.h>


namespace SDF
{

namespace
{
	enum { SON_NODE_POOL_SIZE = 256 };

	static const char* EDIT_TYPE_KEY = "type";

	static const char* MATERIAL_KEY = "material";

	static const char* SHAPE_KEY = "shape";
	static const char* SHAPE_TYPE_KEY = "type";
	static const char* MESH_ID_KEY = "id";


	ERet SerializeEdit(
		const EditOp& edit_op
		, SON::Node& son_edit_op_node
		, SON::NodeAllocator& son_node_pool
		)
	{
		SON::MakeObject( &son_edit_op_node );

		{
			SON::Node* shape_node = nil;

			switch(edit_op.shape_type)
			{
			case SHAPE_MESH:
				//const MeshShape& mesh = edit_op.shape.mesh;
				shape_node = SON::Encode(edit_op.shape.mesh, son_node_pool);
				SON::AddChild(
					shape_node,
					MESH_ID_KEY,
					SON::NewString(edit_op.mesh_id.c_str(), son_node_pool)
					);
				break;

				mxNO_SWITCH_DEFAULT;
			}//switch shape

			mxENSURE(shape_node, ERR_NOT_IMPLEMENTED, "");

			SON::AddChild( &son_edit_op_node, SHAPE_KEY, shape_node );

			{
				SON::Node* shape_type_node = son_node_pool.AllocateNode();
				SON::MakeString(shape_type_node, ShapeType8_Type().GetString(edit_op.shape_type));

				SON::AddChild( shape_node, SHAPE_TYPE_KEY, shape_type_node );
			}
		}//save shape

		{
			SON::Node* material_node = son_node_pool.AllocateNode();
			SON::MakeString(material_node, VoxMatID_Type().GetString(edit_op.material_id));

			SON::AddChild( &son_edit_op_node, MATERIAL_KEY, material_node );
		}

		//
		{
			SON::Node* op_type_node = son_node_pool.AllocateNode();
			SON::MakeString(op_type_node, OpType8_Type().GetString(edit_op.op_type));

			SON::AddChild( &son_edit_op_node, EDIT_TYPE_KEY, op_type_node );
		}

		return ALL_OK;
	}

	ERet DeserializeEdit(
		EditOp &edit_op_
		, const SON::Node& edit_op_node
		)
	{
		mxENSURE(edit_op_node.IsObject(),
			ERR_FAILED_TO_PARSE_DATA,
			"An 'Edit op' node must be an object!"
			);

		const SON::Node* shape_node = edit_op_node.FindChildNamed(SHAPE_KEY);
		mxENSURE(shape_node, ERR_OBJECT_NOT_FOUND, "");


		{
			{
				const SON::Node* shape_type_node = shape_node->FindChildNamed(SHAPE_TYPE_KEY);
				mxENSURE(shape_type_node, ERR_OBJECT_NOT_FOUND, "");
				mxENSURE(shape_type_node->IsString(), ERR_OBJECT_OF_WRONG_TYPE, "");

				mxDO(GetEnumFromString(
					&edit_op_.shape_type,
					shape_type_node->GetStringValue()
					));
			}

			switch(edit_op_.shape_type)
			{
			case SHAPE_MESH:
				mxDO(SON::Decode(shape_node, edit_op_.shape.mesh));
				{
					const SON::Node* mesh_id_node = shape_node->FindChildNamed(MESH_ID_KEY);
					mxENSURE(mesh_id_node, ERR_OBJECT_NOT_FOUND, "");
					mxENSURE(mesh_id_node->IsString(), ERR_OBJECT_OF_WRONG_TYPE, "");

					edit_op_.mesh_id = MakeAssetID(mesh_id_node->GetStringValue());
				}
				break;

			default:
				mxENSURE(false, ERR_UNSUPPORTED_FEATURE, "");
			}
		}

		{
			const SON::Node* material_node = edit_op_node.FindChildNamed(MATERIAL_KEY);
			mxENSURE(material_node, ERR_OBJECT_NOT_FOUND, "");
			mxENSURE(material_node->IsString(), ERR_OBJECT_OF_WRONG_TYPE, "");

			mxDO(GetEnumFromString(
				&edit_op_.material_id,
				material_node->GetStringValue()
				));
		}

		{
			const SON::Node* op_type_node = edit_op_node.FindChildNamed(EDIT_TYPE_KEY);
			mxENSURE(op_type_node, ERR_OBJECT_NOT_FOUND, "");
			mxENSURE(op_type_node->IsString(), ERR_OBJECT_OF_WRONG_TYPE, "");

			mxDO(GetEnumFromString(
				&edit_op_.op_type,
				op_type_node->GetStringValue()
				));
		}

		return ALL_OK;
	}
}

ERet Tape::SaveToFile(const char* filename) const
{
	SON::NodeAllocator	son_node_pool(
		MemoryHeaps::temporary(), SON_NODE_POOL_SIZE
		);

	SON::Node* son_root = SON::NewArray(_edits_list.num(), son_node_pool);

	nwFOR_EACH_INDEXED(const EditOp& edit_op, _edits_list, i)
	{
		SON::Node& son_edit_op_node = son_root->value.a.kids[i];

		mxDO(SerializeEdit(
			edit_op
			, son_edit_op_node
			, son_node_pool
			));
	}//for each edit op

	mxDO(SON::WriteToStream(
		son_root,
		FileWriter(filename)
		));

	return ALL_OK;
}

ERet Tape::LoadFromFile(const char* filename)
{
	NwBlob	file_contents(MemoryHeaps::temporary());
	mxDO(NwBlob_::loadBlobFromFile(
		file_contents
		, filename
		));

	SON::NodeAllocator	son_node_pool(
		MemoryHeaps::temporary(), SON_NODE_POOL_SIZE
		);

	SON::Node* root = SON::ParseTextBuffer(
		file_contents.raw(), file_contents.rawSize()
		, son_node_pool
		, filename
		);
	mxENSURE(root, ERR_FAILED_TO_PARSE_DATA, "");

	//
	mxENSURE(root->IsArray(),
		ERR_FAILED_TO_PARSE_DATA,
		"root node must be array!"
		);

	//
	const U32 num_edit_ops = SON::GetArraySize( root );
	mxDO(_edits_list.reserveExactly(num_edit_ops));

	nwFOR_LOOP(UINT, i, num_edit_ops)
	{
		if(const SON::Node* son_edit_op_node = SON::GetArrayItem(root, i))
		{
			EditOp	edit_op;
			//
			mxDO(DeserializeEdit(
				edit_op
				, *son_edit_op_node
				));

			mxDO(_edits_list.add(edit_op));
		}
	}//for each edit op

	DBGOUT("Loaded tape: %d edits.",
		_edits_list.num()
		);

	return ALL_OK;
}

}//namespace SDF
