// Bounding Interval Hierarchy
// "Instant Ray Tracing: The Bounding Interval Hierarchy" [2006]
// "GPU Volume Raycasting using Bounding Interval Hierarchies" [2009]
#pragma once

#include <Graphics/Public/graphics_utilities.h>
#include <Utility/MeshLib/TriMesh.h>

namespace Meshok
{
	struct BIH: CStruct
	{
		/// NOTE: objects are stored only in leaf nodes!
		/// If an object straddles the splitting plane, it's added to either the left or right child
		/// (unlike a Kd-tree where the object has to be referenced in both children).
		struct Node
		{
			//U32	plane_or_type;	//!< upper two bits: 0 = leaf flag, 1 = x, 2 = y, 3 = z-split;
			U32	flags_and_count;	//!< 0 if internal node, otherwise, the number of primitives stored in this leaf

			union
			{
				/// an internal node in the BIH:
				struct {
					U32	children;	//!< index of the first (left) child
					F32	clip[2];
				} inner;

				/// a leaf node in the BIH
				struct {
					U32	start_tri;	//!< index of the first primitive
					U32	unused[2];
				} leaf;
			};
		public:
			bool IsLeaf() const
			{
				return !(flags_and_count >> 30);	// leaf if upper two bits are zero
			}
			UINT Axis() const
			{
				mxASSERT(!this->IsLeaf());
				return (flags_and_count >> 30) - 1;
			}
			U32 PrimitiveCount() const
			{
				return flags_and_count & 0x3FFFFFFF;	// zero out the upper two bits
			}
		};
		mxSTATIC_ASSERT(sizeof(Node) == 16);

		TArray< Node >	m_nodes;
		TArray< U32 >	m_indices;
		AABBf			m_bounds;

	public:
		mxDECLARE_CLASS(BIH, CStruct);
		mxDECLARE_REFLECTION;

		BIH();

		struct BuildOptions
		{
			UINT	maxTrisPerLeaf;
			UINT	maxTreeDepth;
		public:
			BuildOptions()
			{
				maxTrisPerLeaf = 8;
				maxTreeDepth = 16;
			}
		};

		ERet Build(
			const TriMesh& _mesh,
			const BuildOptions& _options = BuildOptions()
		);

		/// returns the number of all intersections
		U32 FindAllIntersections(
			const V3f& _O,
			const V3f& _D,
			const float _tmax,
			U32 *_primitives,
			U32 _bufferSize
			) const;

		U32 FindAllPrimitivesInBox(
			const AABBf& _bounds,
			U32 *_primitives,
			U32 _bufferSize
			) const;

		void DebugDraw(
			ADebugDraw & renderer
			);

		PREVENT_COPY(BIH);
	};

}//namespace Meshok

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
