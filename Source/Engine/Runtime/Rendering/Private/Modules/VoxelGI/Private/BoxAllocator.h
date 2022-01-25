// A helper for allocating regions inside a 3D texture atlas.
#pragma once


namespace Graphics
{

mxSTOLEN("https://github.com/EpicGames/UnrealEngine/blob/c3caf7b6bf12ae4c8e09b606f10a09776b4d1f38/Engine/Source/Runtime/Engine/Public/TextureLayout3d.h");

/**
 * An incremental texture space allocator.
 * For best results, add the elements ordered descending in size.
 */
class BoxAllocator: NonCopyable
{
	struct FTextureLayoutNode3d
	{
		I32		ChildA,
				ChildB;

		U16		MinX,
				MinY,
				MinZ,
				SizeX,
				SizeY,
				SizeZ;

		bool	bUsed;

	public:
		FTextureLayoutNode3d() {}

		FTextureLayoutNode3d(
			U16 InMinX, U16 InMinY, U16 InMinZ
			, U16 InSizeX, U16 InSizeY, U16 InSizeZ
			):
			ChildA(INDEX_NONE),
			ChildB(INDEX_NONE),
			MinX(InMinX),
			MinY(InMinY),
			MinZ(InMinZ),
			SizeX(InSizeX),
			SizeY(InSizeY),
			SizeZ(InSizeZ),
			bUsed(false)
		{}
	};

	// growing:
	U32 SizeX;
	U32 SizeY;
	U32 SizeZ;

	const bool bPowerOfTwoSize;
	const bool bAlignByFour;

	DynamicArray<FTextureLayoutNode3d > _nodes;
	FTextureLayoutNode3d	_inline_node_storage[16];

public:

	/**
	 * Minimal initialization constructor.
	 * @param	MinSizeX - The minimum width of the texture.
	 * @param	MinSizeY - The minimum height of the texture.
	 * @param	MaxSizeX - The maximum width of the texture.
	 * @param	MaxSizeY - The maximum height of the texture.
	 * @param	InPowerOfTwoSize - True if the texture size must be a power of two.
	 */
	BoxAllocator(
		U32 MinSizeX, U32 MinSizeY, U32 MinSizeZ
		, U32 MaxSizeX, U32 MaxSizeY, U32 MaxSizeZ
		, AllocatorI& allocator
		, bool bInPowerOfTwoSize = false
		, bool bInAlignByFour = true
		):
		SizeX(MinSizeX),
		SizeY(MinSizeY),
		SizeZ(MinSizeZ),
		bPowerOfTwoSize(bInPowerOfTwoSize),
		bAlignByFour(bInAlignByFour)
		, _nodes(allocator)
	{
		mxASSERT(MaxSizeX < USHRT_MAX && MaxSizeY < USHRT_MAX && MaxSizeZ < USHRT_MAX);

		Arrays::initializeWithExternalStorage( _nodes, _inline_node_storage );

		_nodes.add(FTextureLayoutNode3d(0, 0, 0, MaxSizeX, MaxSizeY, MaxSizeZ));
	}

	/**
	 * Finds a free area in the texture large enough to contain a surface with the given size.
	 * If a large enough area is found, it is marked as in use, the output parameters OutBaseX and OutBaseY are
	 * set to the coordinates of the upper left corner of the free area and the function return true.
	 * Otherwise, the function returns false and OutBaseX and OutBaseY remain uninitialized.
	 * @param	OutBaseX - If the function succeeds, contains the X coordinate of the upper left corner of the free area on return.
	 * @param	OutBaseY - If the function succeeds, contains the Y coordinate of the upper left corner of the free area on return.
	 * @param	ElementSizeX - The size of the surface to allocate in horizontal pixels.
	 * @param	ElementSizeY - The size of the surface to allocate in vertical pixels.
	 * @return	True if succeeded, false otherwise.
	 */
	ERet AddElement(
		U32& OutBaseX, U32& OutBaseY, U32& OutBaseZ
		, U32 ElementSizeX, U32 ElementSizeY, U32 ElementSizeZ
		)
	{
		if (ElementSizeX == 0 || ElementSizeY == 0 || ElementSizeZ == 0)
		{
			OutBaseX = 0;
			OutBaseY = 0;
			OutBaseZ = 0;
			return ERR_INVALID_PARAMETER;
		}

		if (bAlignByFour)
		{
			// Pad to 4 to ensure alignment
			ElementSizeX = (ElementSizeX + 3) & ~3;
			ElementSizeY = (ElementSizeY + 3) & ~3;
			ElementSizeZ = (ElementSizeZ + 3) & ~3;
		}
		
		// Try allocating space without enlarging the texture.
		I32	NodeIndex = AddSurfaceInner(0, ElementSizeX, ElementSizeY, ElementSizeZ, false);
		if (NodeIndex == INDEX_NONE)
		{
			// Try allocating space which might enlarge the texture.
			NodeIndex = AddSurfaceInner(0, ElementSizeX, ElementSizeY, ElementSizeZ, true);
		}

		if (NodeIndex != INDEX_NONE)
		{
			FTextureLayoutNode3d&	Node = _nodes[NodeIndex];
			Node.bUsed = true;
			OutBaseX = Node.MinX;
			OutBaseY = Node.MinY;
			OutBaseZ = Node.MinZ;

			if (bPowerOfTwoSize)
			{
				SizeX = Max<U32>(SizeX, CeilPowerOfTwo(Node.MinX + ElementSizeX));
				SizeY = Max<U32>(SizeY, CeilPowerOfTwo(Node.MinY + ElementSizeY));
				SizeZ = Max<U32>(SizeZ, CeilPowerOfTwo(Node.MinZ + ElementSizeZ));
			}
			else
			{
				SizeX = Max<U32>(SizeX, Node.MinX + ElementSizeX);
				SizeY = Max<U32>(SizeY, Node.MinY + ElementSizeY);
				SizeZ = Max<U32>(SizeZ, Node.MinZ + ElementSizeZ);
			}
			return ALL_OK;
		}
		else
		{
			return ERR_BUFFER_TOO_SMALL;
		}
	}

	/** 
	 * Removes a previously allocated element from the layout and collapses the tree as much as possible,
	 * In order to create the largest free block possible and return the tree to its state before the element was added.
	 * @return	True if the element specified by the input parameters existed in the layout.
	 */
	bool RemoveElement(U32 ElementBaseX, U32 ElementBaseY, U32 ElementBaseZ, U32 ElementSizeX, U32 ElementSizeY, U32 ElementSizeZ)
	{
		I32 FoundNodeIndex = INDEX_NONE;
		// Search through nodes to find the element to remove
		//@todo - traverse the tree instead of iterating through all nodes
		for (I32 NodeIndex = 0; NodeIndex < _nodes.num(); NodeIndex++)
		{
			FTextureLayoutNode3d&	Node = _nodes[NodeIndex];

			if (Node.MinX == ElementBaseX
				&& Node.MinY == ElementBaseY
				&& Node.MinZ == ElementBaseZ
				&& Node.SizeX == ElementSizeX
				&& Node.SizeY == ElementSizeY
				&& Node.SizeZ == ElementSizeZ)
			{
				FoundNodeIndex = NodeIndex;
				break;
			}
		}

		if (FoundNodeIndex != INDEX_NONE)
		{
			// Mark the found node as not being used anymore
			_nodes[FoundNodeIndex].bUsed = false;

			// Walk up the tree to find the node closest to the root that doesn't have any used children
			I32 ParentNodeIndex = FindParentNode(FoundNodeIndex);
			ParentNodeIndex = IsNodeUsed(ParentNodeIndex) ? INDEX_NONE : ParentNodeIndex;
			I32 LastParentNodeIndex = ParentNodeIndex;
			while (ParentNodeIndex != INDEX_NONE 
				&& !IsNodeUsed(_nodes[ParentNodeIndex].ChildA) 
				&& !IsNodeUsed(_nodes[ParentNodeIndex].ChildB))
			{
				LastParentNodeIndex = ParentNodeIndex;
				ParentNodeIndex = FindParentNode(ParentNodeIndex);
			} 

			// Remove the children of the node closest to the root with only unused children,
			// Which restores the tree to its state before this element was allocated,
			// And allows allocations as large as LastParentNode in the future.
			if (LastParentNodeIndex != INDEX_NONE)
			{
				RemoveChildren(LastParentNodeIndex);
			}
			return true;
		}

		return false;
	}

	/**
	 * Returns the minimum texture width which will contain the allocated surfaces.
	 */
	U32 GetSizeX() const { return SizeX; }

	/**
	 * Returns the minimum texture height which will contain the allocated surfaces.
	 */
	U32 GetSizeY() const { return SizeY; }

	U32 GetSizeZ() const { return SizeZ; }

	Int3 GetSize() const { return Int3(SizeX, SizeY, SizeZ); }

	U32 GetMaxSizeX() const 
	{
		return _nodes[0].SizeX;
	}

	U32 GetMaxSizeY() const 
	{
		return _nodes[0].SizeY;
	}

	U32 GetMaxSizeZ() const 
	{
		return _nodes[0].SizeZ;
	}

private:

	/** Recursively traverses the tree depth first and searches for a large enough leaf node to contain the requested allocation. */
	I32 AddSurfaceInner(I32 NodeIndex, U32 ElementSizeX, U32 ElementSizeY, U32 ElementSizeZ, bool bAllowTextureEnlargement)
	{
		mxASSERT(NodeIndex != INDEX_NONE);
		// But do access this node via a pointer until the first recursive call. Prevents a ton of LHS.
		const FTextureLayoutNode3d* CurrentNodePtr = &_nodes[NodeIndex];
		if (CurrentNodePtr->ChildA != INDEX_NONE)
		{
			// Children are always allocated together
			mxASSERT(CurrentNodePtr->ChildB != INDEX_NONE);

			// Traverse the children
			const I32 Result = AddSurfaceInner(CurrentNodePtr->ChildA, ElementSizeX, ElementSizeY, ElementSizeZ, bAllowTextureEnlargement);
			
			// The pointer is now invalid, be explicit!
			CurrentNodePtr = 0;

			if (Result != INDEX_NONE)
			{
				return Result;
			}

			return AddSurfaceInner(_nodes[NodeIndex].ChildB, ElementSizeX, ElementSizeY, ElementSizeZ, bAllowTextureEnlargement);
		}
		// Node has no children, it is a leaf
		else
		{
			// Reject this node if it is already used
			if (CurrentNodePtr->bUsed)
			{
				return INDEX_NONE;
			}

			// Reject this node if it is too small for the element being placed
			if (CurrentNodePtr->SizeX < ElementSizeX || CurrentNodePtr->SizeY < ElementSizeY || CurrentNodePtr->SizeZ < ElementSizeZ)
			{
				return INDEX_NONE;
			}

			if (!bAllowTextureEnlargement)
			{
				// Reject this node if this is an attempt to allocate space without enlarging the texture, 
				// And this node cannot hold the element without enlarging the texture.
				if (CurrentNodePtr->MinX + ElementSizeX > SizeX || CurrentNodePtr->MinY + ElementSizeY > SizeY || CurrentNodePtr->MinZ + ElementSizeY > SizeZ)
				{
					return INDEX_NONE;
				}
			}

			// Use this node if the size matches the requested element size
			if (CurrentNodePtr->SizeX == ElementSizeX && CurrentNodePtr->SizeY == ElementSizeY && CurrentNodePtr->SizeZ == ElementSizeZ)
			{
				return NodeIndex;
			}

			const U32 ExcessWidth = CurrentNodePtr->SizeX - ElementSizeX;
			const U32 ExcessHeight = CurrentNodePtr->SizeY - ElementSizeY;
			const U32 ExcessDepth = CurrentNodePtr->SizeZ - ElementSizeZ;

			// The pointer to the current node may be invalidated below, be explicit!
			CurrentNodePtr = 0;

			// Add new nodes, and link them as children of the current node.
			if (ExcessWidth > ExcessHeight)
			{
				// Store a copy of the current node on the stack for easier debugging.
				// Can't store a pointer to the current node since _nodes may be reallocated in this function.
				const FTextureLayoutNode3d CurrentNode = _nodes[NodeIndex];

				if (ExcessWidth > ExcessDepth)
				{
					// Update the child indices
					_nodes[NodeIndex].ChildA = _nodes.num();

					// Create a child with the same width as the element being placed.
					// The height may not be the same as the element height yet, in that case another subdivision will occur when traversing this child node.
					_nodes.add(FTextureLayoutNode3d(
						CurrentNode.MinX,
						CurrentNode.MinY,
						CurrentNode.MinZ,
						ElementSizeX,
						CurrentNode.SizeY,
						CurrentNode.SizeZ
						));

					// Create a second child to contain the leftover area in the X direction
					_nodes[NodeIndex].ChildB = _nodes.num();
					_nodes.add(FTextureLayoutNode3d(
						CurrentNode.MinX + ElementSizeX,
						CurrentNode.MinY,
						CurrentNode.MinZ,
						CurrentNode.SizeX - ElementSizeX,
						CurrentNode.SizeY,
						CurrentNode.SizeZ
						));
				}
				else
				{
					_nodes[NodeIndex].ChildA = _nodes.num();

					_nodes.add(FTextureLayoutNode3d(
						CurrentNode.MinX,
						CurrentNode.MinY,
						CurrentNode.MinZ,
						CurrentNode.SizeX,
						CurrentNode.SizeY,
						ElementSizeZ
						));

					_nodes[NodeIndex].ChildB = _nodes.num();
					_nodes.add(FTextureLayoutNode3d(
						CurrentNode.MinX,
						CurrentNode.MinY,
						CurrentNode.MinZ + ElementSizeZ,
						CurrentNode.SizeX,
						CurrentNode.SizeY,
						CurrentNode.SizeZ - ElementSizeZ
						));
				}
			}
			else
			{
				// Store a copy of the current node on the stack for easier debugging.
				// Can't store a pointer to the current node since _nodes may be reallocated in this function.
				const FTextureLayoutNode3d CurrentNode = _nodes[NodeIndex];

				if (ExcessHeight > ExcessDepth)
				{
					_nodes[NodeIndex].ChildA = _nodes.num();
					_nodes.add(FTextureLayoutNode3d(
						CurrentNode.MinX,
						CurrentNode.MinY,
						CurrentNode.MinZ,
						CurrentNode.SizeX,
						ElementSizeY,
						CurrentNode.SizeZ
						));

					_nodes[NodeIndex].ChildB = _nodes.num();
					_nodes.add(FTextureLayoutNode3d(
						CurrentNode.MinX,
						CurrentNode.MinY + ElementSizeY,
						CurrentNode.MinZ,
						CurrentNode.SizeX,
						CurrentNode.SizeY - ElementSizeY,
						CurrentNode.SizeZ
						));
				}
				else
				{
					_nodes[NodeIndex].ChildA = _nodes.num();

					_nodes.add(FTextureLayoutNode3d(
						CurrentNode.MinX,
						CurrentNode.MinY,
						CurrentNode.MinZ,
						CurrentNode.SizeX,
						CurrentNode.SizeY,
						ElementSizeZ
						));

					_nodes[NodeIndex].ChildB = _nodes.num();
					_nodes.add(FTextureLayoutNode3d(
						CurrentNode.MinX,
						CurrentNode.MinY,
						CurrentNode.MinZ + ElementSizeZ,
						CurrentNode.SizeX,
						CurrentNode.SizeY,
						CurrentNode.SizeZ - ElementSizeZ
						));
				}
			}

			// Only traversing ChildA, since ChildA is always the newly created node that matches the element size
			return AddSurfaceInner(_nodes[NodeIndex].ChildA, ElementSizeX, ElementSizeY, ElementSizeZ, bAllowTextureEnlargement);
		}
	}

	/** Returns the index into _nodes of the parent node of SearchNode. */
	I32 FindParentNode(I32 SearchNodeIndex)
	{
		//@todo - could be a constant time search if the nodes stored a parent index
		for (I32 NodeIndex = 0; NodeIndex < _nodes.num(); NodeIndex++)
		{
			FTextureLayoutNode3d&	Node = _nodes[NodeIndex];
			if (Node.ChildA == SearchNodeIndex || Node.ChildB == SearchNodeIndex)
			{
				return NodeIndex;
			}
		}
		return INDEX_NONE;
	}

	/** Returns true if the node or any of its children are marked used. */
	bool IsNodeUsed(I32 NodeIndex)
	{
		bool bChildrenUsed = false;
		if (_nodes[NodeIndex].ChildA != INDEX_NONE)
		{
			mxASSERT(_nodes[NodeIndex].ChildB != INDEX_NONE);
			bChildrenUsed = IsNodeUsed(_nodes[NodeIndex].ChildA) || IsNodeUsed(_nodes[NodeIndex].ChildB);
		}
		return _nodes[NodeIndex].bUsed || bChildrenUsed;
	}

	/** Recursively removes the children of a given node from the _nodes array and adjusts existing indices to compensate. */
	void RemoveChildren(I32 NodeIndex)
	{
		// Traverse the children depth first
		if (_nodes[NodeIndex].ChildA != INDEX_NONE)
		{
			RemoveChildren(_nodes[NodeIndex].ChildA);
		}
		if (_nodes[NodeIndex].ChildB != INDEX_NONE)
		{
			RemoveChildren(_nodes[NodeIndex].ChildB);
		}

		if (_nodes[NodeIndex].ChildA != INDEX_NONE)
		{
			// Store off the index of the child since it may be changed in the code below
			const I32 OldChildA = _nodes[NodeIndex].ChildA;

			// Remove the child from the _nodes array
			_nodes.RemoveAt(OldChildA);

			// Iterate over all the _nodes and fix up their indices now that an element has been removed
			for (I32 OtherNodeIndex = 0; OtherNodeIndex < _nodes.num(); OtherNodeIndex++)
			{
				if (_nodes[OtherNodeIndex].ChildA >= OldChildA)
				{
					_nodes[OtherNodeIndex].ChildA--;
				}
				if (_nodes[OtherNodeIndex].ChildB >= OldChildA)
				{
					_nodes[OtherNodeIndex].ChildB--;
				}
			}
			// Mark the node as not having a ChildA
			_nodes[NodeIndex].ChildA = INDEX_NONE;
		}

		if (_nodes[NodeIndex].ChildB != INDEX_NONE)
		{
			const I32 OldChildB = _nodes[NodeIndex].ChildB;
			_nodes.RemoveAt(OldChildB);
			for (I32 OtherNodeIndex = 0; OtherNodeIndex < _nodes.num(); OtherNodeIndex++)
			{
				if (_nodes[OtherNodeIndex].ChildA >= OldChildB)
				{
					_nodes[OtherNodeIndex].ChildA--;
				}
				if (_nodes[OtherNodeIndex].ChildB >= OldChildB)
				{
					_nodes[OtherNodeIndex].ChildB--;
				}
			}
			_nodes[NodeIndex].ChildB = INDEX_NONE;
		}
	}
};

}//namespace Graphics
