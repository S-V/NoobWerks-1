// utilities for performing CSG operations
#pragma once

#include "Utility/Meshok/bsp.h"

class Renderer;

struct CSG
{
	// The tree representing a destructible environment.
	BSP::Tree	worldTree;

	// Subtractive CSG models for testing (they are 'inside-out' meshes).
	BSP::Tree	operand;
	BSP::Tree	smallBox;
	BSP::Tree	largeBox;
	BSP::Tree	rotatedBox;

	// Temporary storage for storing intermediate results of CSG calculations to reduce memory allocations.
	BSP::Tree	temporary1;
	BSP::Tree	temporary2;

	// Buffers for storing generated geometry.
	TArray< BSP::Vertex >	vertices;
	TArray< UINT16 >		indices;

	// Dynamic GPU buffers for rendering.
//	bgfx::DynamicVertexBufferHandle dynamicVB;
//	bgfx::DynamicIndexBufferHandle dynamicIB;

public:
	ERet Initialize();
	void Shutdown();

	void RunTestCode();

	//void Subtract(
	//	const V3F4& _position,
	//	BSP::Tree & _worldTree,
	//	const BSP::Tree& _mesh,	// subtractive brush (template)
	//	BSP::Tree & _temporary	// subtractive brush (instance)
	//);

	void Subtract(
		const V3F4& _position,
		BSP::Tree & _worldTree,
		const BSP::Tree& _mesh	// subtractive brush
	);

	void Shoot(
		const V3F4& start,
		const V3F4& direction,
		BSP::RayCastResult &result
	);

	void Draw( Renderer & renderer );

	void UpdateRenderMesh(
		const BSP::Tree& tree,
		const BSP::NodeID root = 0
	);
	void UpdateRenderMesh(
		const BSP::Vertex* vertices, int numVertices,
		const UINT16* indices, int numIndices
	);
	void UpdateRenderMesh(
		const TArray< BSP::Vertex >& vertices,
		const TArray< UINT16 >& indices
	);

	static void MakeBoxMesh(
		float length, float height, float depth,
		TArray< BSP::Vertex > &vertices, TArray< UINT16 > &indices
	);
	static void FlipWinding(
		TArray< BSP::Vertex > & vertices, TArray< UINT16 > & indices
	);
};
