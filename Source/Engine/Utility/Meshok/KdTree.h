// KD-tree or kd-tree
#pragma once

#include <MeshLib/TriMesh.h>

namespace Meshok
{
	struct KdTree: CStruct
	{
		struct Node
		{
			union
			{
				U32	plane_or_type;	//!< upper two bits: 00 = leaf flag, 01 = x, 02 = y, 13 = z-split

				/// an internal node in the kd-tree
				struct {
					F32	plane_dist;
					U32	leaf_index;
				} inner;

				/// a leaf node in the kd-tree
				struct {
					F32	triangle_count;	//!< number of primitives stored in this leaf
					U32	leaf_index;
				} leaf;
			};
		};
		mxSTATIC_ASSERT(sizeof(Node) == 8);

		TArray< Node >	m_nodes;

	public:
		mxDECLARE_CLASS(KdTree, CStruct);
		mxDECLARE_REFLECTION;

		KdTree();
		~KdTree();

		struct BuildOptions
		{
			UINT	maxTrisPerLeaf;
			UINT	maxTreeDepth;
		public:
			BuildOptions()
			{
				maxTrisPerLeaf = 16;
				maxTreeDepth = 16;
			}
		};

		ERet Build(
			const TriMesh& _mesh,
			const BuildOptions& _options = BuildOptions()
		);

		PREVENT_COPY(KdTree);
	};

}//namespace Meshok

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
