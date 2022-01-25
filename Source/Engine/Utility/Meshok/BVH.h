// Bounding Volume Hierarchy
#pragma once

#include <Graphics/Public/graphics_utilities.h>
#include <Utility/MeshLib/TriMesh.h>
#include <Utility/Meshok/Volumes.h>	// VX::ADebugView

namespace Meshok
{
	mxSTOLEN("Based on https://github.com/brandonpelfrey/Fast-BVH");
	/// A Bounding Volume Hierarchy system for fast Ray-Object intersection tests.
	/// This is actually an AABB-Tree.
	struct BVH: CStruct
	{
		/// NOTE: objects are stored only in leaf nodes!
		mxPREALIGN(16)
		struct Node
		{
			V3f		mins;	//12
			union {
				/// Node data:
				U32	unused;	// must be unused! it's non-zero only for leaves!

				/// Leaf data:
				U32	count;	//!< number of primitives stored in this leaf
			};

			V3f		maxs;	//12
			union {
				/// Node data:
				/// index of the second (right) child
				/// (the first (left) child goes right after the node)
				U32	iRightChild;	// absolute, but could use relative offsets

				/// Leaf data:
				U32	start;	//!< index of the first primitive
			};

		public:
			bool IsLeaf() const
			{
				return count;	// if count != 0 then this is a leaf
			}
		};
		mxSTATIC_ASSERT(sizeof(Node) == 32);

		DynamicArray< Node >	m_nodes;
		DynamicArray< U32 >		m_indices;	//!< indices of original mesh triangles
		AABBf					m_bounds;

	public:
		mxDECLARE_CLASS(BVH, CStruct);
		mxDECLARE_REFLECTION;

		BVH(AllocatorI& allocator);

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

		/// Builds the tree based on the input object data set.
		ERet Build(
			const TriMeshI& _mesh
			, const BuildOptions& build_options
			, AllocatorI & scratchpad = MemoryHeaps::temporary()
		);


		typedef void TriangleCallback(
			int triangle_index
			, void* user_data
			);

		/// returns the number of all intersections
		U32 FindAllIntersections(
			const V3f& _O,
			const V3f& _D,
			const float _tmax,	// Maximum distance ray can travel (makes it a segment, otherwise it would be infinite)
			TriangleCallback* triangle_callback,
			void* triangle_callback_data
			) const;

		U32 FindAllPrimitivesInBox(
			const AABBf& _bounds,
			U32 *_primitives,
			U32 _bufferSize
			) const;

		struct Intersection
		{
			U32	iTriangle;	//!< Object that was hit
			F32	minT;	//!< Intersection distance along the ray
		};
		bool FindClosestIntersection(
			const V3f& _O,
			const V3f& _D,
			const float _tmax,
			const TriMesh& _mesh,
			Intersection &_result
			) const;

		void DebugDraw(
			ADebugDraw & renderer
			);
		void dbg_show(
			VX::ADebugView & renderer
			, const M44f& transform
			);

		PREVENT_COPY(BVH);
	};

}//namespace Meshok

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
