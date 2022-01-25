// utilities for performing CSG operations
#include "csg.h"
//#include "render.h"	// PackNormal()

static ERet Build_BSP_Tree(
						   const BSP::Vertex* vertices, int numVertices,
						   const UINT16* indices, int numIndices,
						   BSP::Tree &tree
						   )
{
	BSP::TProcessTriangles< BSP::Vertex, UINT16 > enumerateMeshVertices(
		vertices, numVertices, indices, numIndices
	);
	mxDO(tree.Build( &enumerateMeshVertices ));
	return ALL_OK;
}
static ERet Build_BSP_Tree(
						   const TArray< BSP::Vertex >& vertices,
						   const TArray< UINT16 >& indices,
						   BSP::Tree &tree
						   )
{
	return Build_BSP_Tree(
		vertices.ToPtr(), vertices.Num(),
		indices.ToPtr(), indices.Num(),
		tree
	);
}

ERet CSG::Initialize()
{
	// Build a BSP tree for the destructible environment.
	{
		BSP::Vertex planeVertices[4] =
		{
			BSP::Vertex( V3_Set( -100.0f, -100.0f, 0.0f ), PackNormal( 0.0f, 0.0f, 1.0f ), PackNormal( 1.0f, 0.0f, 0.0f ),  V2F_Set(    0, 1.0f ) ),
			BSP::Vertex( V3_Set( -100.0f,  100.0f, 0.0f ), PackNormal( 0.0f, 0.0f, 1.0f ), PackNormal( 1.0f, 0.0f, 0.0f ),  V2F_Set(    0,    0 ) ),
			BSP::Vertex( V3_Set(  100.0f,  100.0f, 0.0f ), PackNormal( 0.0f, 0.0f, 1.0f ), PackNormal( 1.0f, 0.0f, 0.0f ),  V2F_Set( 1.0f,    0 ) ),
			BSP::Vertex( V3_Set(  100.0f, -100.0f, 0.0f ), PackNormal( 0.0f, 0.0f, 1.0f ), PackNormal( 1.0f, 0.0f, 0.0f ),  V2F_Set( 1.0f, 1.0f ) ),
		};

		const UINT16 planeIndices[6] = {
			0, 1, 2, 0, 2, 3,
		};

		Build_BSP_Tree(
			planeVertices, mxCOUNT_OF(planeVertices),
			planeIndices, mxCOUNT_OF(planeIndices),
			worldTree
		);

		DBGOUT("\nWorld BSP tree:\n");
		BSP::Debug::PrintTree(worldTree);
	}


	// Build a BSP tree for the subtractive mesh.


	// Create test models.
	{
		MakeBoxMesh( 10.0f, 50.0f, 10.0f, vertices, indices );
		FlipWinding( vertices, indices );
		Build_BSP_Tree( vertices, indices, operand );
		DBGOUT("\nModel BSP tree:\n");
		BSP::Debug::PrintTree(operand);

		MakeBoxMesh( 10.0f, 10.0f, 10.0f, vertices, indices );
		Build_BSP_Tree( vertices, indices, smallBox );

		MakeBoxMesh( 70.0f, 200.0f, 70.0f, vertices, indices );
		Build_BSP_Tree( vertices, indices, largeBox );
	}

UNDONE;
#if 0
	// Create render mesh.
	dynamicVB = bgfx::createDynamicVertexBuffer( 1024, BSP::Vertex::ms_decl, BGFX_BUFFER_ALLOW_RESIZE );
	dynamicIB = bgfx::createDynamicIndexBuffer( 1024, BGFX_BUFFER_ALLOW_RESIZE );
#endif

	return ALL_OK;
}
void CSG::Shutdown()
{
	UNDONE;
#if 0
	bgfx::destroyDynamicIndexBuffer( dynamicIB );
	bgfx::destroyDynamicVertexBuffer( dynamicVB );
#endif
}

void CSG::RunTestCode()
{
//	DBGOUT("\nPolygons before CSG:\n");
//	BSP::Debug::PrintFaceList(worldTree, worldTree.m_nodes[0].faces);
#if 1
	{
		V3F4 pos = V3_Set(40,0,20);
#if 1
		//Subtract(
		//	pos,
		//	worldTree,
		//	operand,
		//	temporary1
		//	);

		//MakeBoxMesh( 50.0f, 10.0f, 50.0f, vertices, indices );
		//Build_BSP_Tree( vertices, indices, worldTree );

		BSP::Tree subtractiveModel;
		MakeBoxMesh( 70.0f, 200.0f, 70.0f, vertices, indices );
		//MakeBoxMesh( 20.0f, 50.0f, 10.0f, vertices, indices );
		FlipWinding( vertices, indices );
		Build_BSP_Tree( vertices, indices, subtractiveModel );

		temporary1.CopyFrom( subtractiveModel );
		temporary2.CopyFrom( subtractiveModel );
		worldTree.Subtract2( temporary1, temporary2 );


		//subtractiveModel.Translate(V3_Set(10,0,10));
		//temporary1.CopyFrom( subtractiveModel );
		//temporary2.CopyFrom( subtractiveModel );
		//worldTree.Subtract2( temporary1, temporary2 );


#else
		temporary1.CopyFrom( operand );
		temporary1.Translate( pos );
		//worldTree.CopyFrom( temporary1 );
		worldTree.m_nodes[0].back = BSP::CopySubTree(worldTree, temporary1, 0);
		BSP::Debug::PrintTree(worldTree);
#endif
	}
#endif



#if 0
#if 1
	this->UpdateRenderMesh( worldTree );
#else
	{
		//temporary1.CopyFrom( largeBox );
		worldTree.CopyFrom( largeBox );

		Vector4 plane = { 1, 0, 0, 0 };
		BSP::NodeID front, back;
		worldTree.PartitionNodeWithPlane( plane, 0, &front, &back );

		DBGOUT("\nFront side:\n");
		BSP::Debug::PrintTree(worldTree, front);
		DBGOUT("\nBack side:\n");
		BSP::Debug::PrintTree(worldTree, back);

		Generate_CPU_Mesh(
			worldTree, front,
			vertices, indices
		);
	}
#endif
#endif



#if 0
	{
		BSP::FaceID clippedFaces = ClipFacesOutsideBrush(
			worldTree, worldTree.m_nodes[0].faces,
			operand, 0
			);

		BSP::Debug::PrintFaceList(worldTree, clippedFaces);
		{
			vertices.Empty();
			indices.Empty();
			TriangulateFaces(
				worldTree, clippedFaces,
				vertices, indices
				);
			UpdateRenderMesh( vertices.ToPtr(), vertices.Num(), indices.ToPtr(), indices.Num() );
		}
	}

#if 0
	DBGOUT("\nPolygons after CSG:\n");
	BSP::Debug::PrintFaceList(worldTree, worldTree.m_nodes[0].faces);
	// 12 -> 11 -> 10 -> 9 -> 8

	//DbgRemoveFirstPoly( worldTree, 0 );
	//DbgRemoveFirstPoly( worldTree, 0 );
	//DbgRemoveFirstPoly( worldTree, 0 );
	//BSP::Debug::PrintFaceList(worldTree, worldTree.m_nodes[0].faces);

	{
		vertices.Empty();
		indices.Empty();
		TriangulateFaces(
			worldTree, worldTree.m_nodes[0].faces,
			vertices, indices
			);
		UpdateRenderMesh( vertices.ToPtr(), vertices.Num(), indices.ToPtr(), indices.Num() );
	}
#endif
#endif
}

void CSG::Subtract(
				   const V3F4& _position,
				   BSP::Tree & _worldTree,
				   const BSP::Tree& _mesh
				   )
{
	temporary1.CopyFrom( _mesh );
	temporary1.Translate( _position );

	temporary2.CopyFrom( _mesh );
	temporary2.Translate( _position );

	_worldTree.Subtract2( temporary1, temporary2 );
	//	DBGOUT("\nBSP tree after CSG:\n");
	//	BSP::Debug::PrintTree(worldTree);
}

void CSG::Shoot(
	const V3F4& start,
	const V3F4& direction,
	BSP::RayCastResult &result
)
{
	worldTree.CastRay( start, direction, result );
	if( result.hitAnything )
	{
		LogStream(LL_Info) << "Hit pos: " << result.position;
		Subtract(
			result.position,
			worldTree,
			operand
		);
		this->UpdateRenderMesh( worldTree );
	}
}

void CSG::Draw( Renderer & renderer )
{
	UNDONE;
#if 0
	//float invView[16];
	//bx::mtxInverse(invView, view);
	//bgfx::setUniform(renderer.u_inverseViewMat, invView);

	bgfx::setVertexBuffer(dynamicVB);
	bgfx::setIndexBuffer(dynamicIB);

	bgfx::setTexture(0, renderer.s_texColor,  renderer.colorMap);
	bgfx::setTexture(1, renderer.s_texNormal, renderer.normalMap);

	bgfx::setState(0
		| BGFX_STATE_RGB_WRITE
		| BGFX_STATE_ALPHA_WRITE
		| BGFX_STATE_CULL_CCW
		| BGFX_STATE_DEPTH_WRITE
		| BGFX_STATE_DEPTH_TEST_LESS
		| BGFX_STATE_MSAA
		);

	bgfx::submit(RENDER_PASS_GEOMETRY_ID, renderer.geomProgram);
#endif
}

void CSG::UpdateRenderMesh(
					  const BSP::Tree& tree,
					  const BSP::NodeID root
					  )
{
	tree.GenerateMesh( vertices, indices, root );
	UpdateRenderMesh( vertices, indices );
}

void CSG::UpdateRenderMesh(
	const TArray< BSP::Vertex >& vertices,
	const TArray< UINT16 >& indices
)
{
	this->UpdateRenderMesh( vertices.ToPtr(), vertices.Num(), indices.ToPtr(), indices.Num() );
}

void CSG::UpdateRenderMesh(
	const BSP::Vertex* vertices, int numVertices,
	const UINT16* indices, int numIndices
)
{
	UNDONE;
#if 0
	const bgfx::Memory* vertexMemory = bgfx::copy( vertices, sizeof(vertices[0])*numVertices );
	const bgfx::Memory* indexMemory = bgfx::copy( indices, sizeof(indices[0])*numIndices );
	bgfx::updateDynamicVertexBuffer( dynamicVB, 0, vertexMemory );
	bgfx::updateDynamicIndexBuffer( dynamicIB, 0, indexMemory );
#endif
}

void CSG::MakeBoxMesh(
							float length, float height, float depth,
							TArray< BSP::Vertex > &vertices, TArray< UINT16 > &indices
							)
{
	enum { NUM_VERTICES = 24 };
	enum { NUM_INDICES = 36 };

	// Create vertex buffer.

	vertices.SetNum( NUM_VERTICES );

	const float HL = 0.5f * length;
	const float HH = 0.5f * height;
	const float HD = 0.5f * depth;

	// Fill in the front face vertex data.
	vertices[0]  = BSP::Vertex( V3_Set( -HL, -HH, -HD ),	PackNormal( 0.0f, 0.0f, -1.0f ), 	PackNormal( 1.0f, 0.0f, 0.0f ), 	V2F_Set( 0.0f, 1.0f )	);
	vertices[1]  = BSP::Vertex( V3_Set( -HL,  HH, -HD ),	PackNormal( 0.0f, 0.0f, -1.0f ), 	PackNormal( 1.0f, 0.0f, 0.0f ), 	V2F_Set( 0.0f, 0.0f )	);
	vertices[2]  = BSP::Vertex( V3_Set(  HL,  HH, -HD ),	PackNormal( 0.0f, 0.0f, -1.0f ), 	PackNormal( 1.0f, 0.0f, 0.0f ), 	V2F_Set( 1.0f, 0.0f )	);
	vertices[3]  = BSP::Vertex( V3_Set(  HL, -HH, -HD ),	PackNormal( 0.0f, 0.0f, -1.0f ), 	PackNormal( 1.0f, 0.0f, 0.0f ), 	V2F_Set( 1.0f, 1.0f )	);

	// Fill in the back face vertex data.
	vertices[4]  = BSP::Vertex( V3_Set( -HL, -HH, HD ), 	PackNormal( 0.0f, 0.0f, 1.0f ), 	PackNormal( -1.0f, 0.0f, 0.0f ),	V2F_Set( 1.0f, 1.0f )	);
	vertices[5]  = BSP::Vertex( V3_Set(  HL, -HH, HD ), 	PackNormal( 0.0f, 0.0f, 1.0f ), 	PackNormal( -1.0f, 0.0f, 0.0f ),	V2F_Set( 0.0f, 1.0f )	);
	vertices[6]  = BSP::Vertex( V3_Set(  HL,  HH, HD ), 	PackNormal( 0.0f, 0.0f, 1.0f ), 	PackNormal( -1.0f, 0.0f, 0.0f ),	V2F_Set( 0.0f, 0.0f )	);
	vertices[7]  = BSP::Vertex( V3_Set( -HL,  HH, HD ), 	PackNormal( 0.0f, 0.0f, 1.0f ), 	PackNormal( -1.0f, 0.0f, 0.0f ),	V2F_Set( 1.0f, 0.0f )	);

	// Fill in the top face vertex data.
	vertices[8]  = BSP::Vertex( V3_Set( -HL, HH, -HD ),		PackNormal( 0.0f, 1.0f, 0.0f ), 	PackNormal( 1.0f, 0.0f, 0.0f ), 	V2F_Set( 0.0f, 1.0f )	);
	vertices[9]  = BSP::Vertex( V3_Set( -HL, HH,  HD ),		PackNormal( 0.0f, 1.0f, 0.0f ), 	PackNormal( 1.0f, 0.0f, 0.0f ), 	V2F_Set( 0.0f, 0.0f )	);
	vertices[10] = BSP::Vertex( V3_Set(  HL, HH,  HD ),		PackNormal( 0.0f, 1.0f, 0.0f ), 	PackNormal( 1.0f, 0.0f, 0.0f ), 	V2F_Set( 1.0f, 0.0f )	);
	vertices[11] = BSP::Vertex( V3_Set(  HL, HH, -HD ),		PackNormal( 0.0f, 1.0f, 0.0f ), 	PackNormal( 1.0f, 0.0f, 0.0f ), 	V2F_Set( 1.0f, 1.0f )	);

	// Fill in the bottom face vertex data.
	vertices[12] = BSP::Vertex( V3_Set( -HL, -HH, -HD ),	PackNormal( 0.0f, -1.0f, 0.0f ), 	PackNormal( -1.0f, 0.0f, 0.0f ),	V2F_Set( 1.0f, 1.0f )	);
	vertices[13] = BSP::Vertex( V3_Set(  HL, -HH, -HD ),	PackNormal( 0.0f, -1.0f, 0.0f ), 	PackNormal( -1.0f, 0.0f, 0.0f ),	V2F_Set( 0.0f, 1.0f )	);
	vertices[14] = BSP::Vertex( V3_Set(  HL, -HH,  HD ),	PackNormal( 0.0f, -1.0f, 0.0f ), 	PackNormal( -1.0f, 0.0f, 0.0f ),	V2F_Set( 0.0f, 0.0f )	);
	vertices[15] = BSP::Vertex( V3_Set( -HL, -HH,  HD ),	PackNormal( 0.0f, -1.0f, 0.0f ), 	PackNormal( -1.0f, 0.0f, 0.0f ),	V2F_Set( 1.0f, 0.0f )	);

	// Fill in the left face vertex data.
	vertices[16] = BSP::Vertex( V3_Set( -HL, -HH,  HD ),	PackNormal( -1.0f, 0.0f, 0.0f ), 	PackNormal( 0.0f, 0.0f, -1.0f ),	V2F_Set( 0.0f, 1.0f )	);
	vertices[17] = BSP::Vertex( V3_Set( -HL,  HH,  HD ),	PackNormal( -1.0f, 0.0f, 0.0f ), 	PackNormal( 0.0f, 0.0f, -1.0f ),	V2F_Set( 0.0f, 0.0f )	);
	vertices[18] = BSP::Vertex( V3_Set( -HL,  HH, -HD ),	PackNormal( -1.0f, 0.0f, 0.0f ), 	PackNormal( 0.0f, 0.0f, -1.0f ),	V2F_Set( 1.0f, 0.0f )	);
	vertices[19] = BSP::Vertex( V3_Set( -HL, -HH, -HD ),	PackNormal( -1.0f, 0.0f, 0.0f ), 	PackNormal( 0.0f, 0.0f, -1.0f ),	V2F_Set( 1.0f, 1.0f )	);

	// Fill in the right face vertex data.
	vertices[20] = BSP::Vertex( V3_Set(  HL, -HH, -HD ),	PackNormal( 1.0f, 0.0f, 0.0f ), 	PackNormal( 0.0f, 0.0f, 1.0f ), 	V2F_Set( 0.0f, 1.0f )	);
	vertices[21] = BSP::Vertex( V3_Set(  HL,  HH, -HD ),	PackNormal( 1.0f, 0.0f, 0.0f ), 	PackNormal( 0.0f, 0.0f, 1.0f ), 	V2F_Set( 0.0f, 0.0f )	);
	vertices[22] = BSP::Vertex( V3_Set(  HL,  HH,  HD ),	PackNormal( 1.0f, 0.0f, 0.0f ), 	PackNormal( 0.0f, 0.0f, 1.0f ), 	V2F_Set( 1.0f, 0.0f )	);
	vertices[23] = BSP::Vertex( V3_Set(  HL, -HH,  HD ),	PackNormal( 1.0f, 0.0f, 0.0f ), 	PackNormal( 0.0f, 0.0f, 1.0f ), 	V2F_Set( 1.0f, 1.0f )	);

	//// Scale the box.
	//{
	//	M44F4 scaleMatrix = Matrix_Scaling( length, height, depth );
	//	M44F4 scaleMatrixIT = Matrix_Transpose( Matrix_Inverse( scaleMatrix ) );

	//	for( UINT i = 0; i < NUM_VERTICES; ++i )
	//	{
	//	vertices[i].xyz = Matrix_TransformPoint( scaleMatrix, vertices[i].xyz );
	//		scaleMatrixIT.TransformNormal( vertices[i].Normal );
	//		scaleMatrixIT.TransformNormal( vertices[i].Tangent );
	//	}
	//}

	// Create the index buffer.

	indices.SetNum( NUM_INDICES );

	// Fill in the front face index data
	indices[0] = 0; indices[1] = 1; indices[2] = 2;
	indices[3] = 0; indices[4] = 2; indices[5] = 3;

	// Fill in the back face index data
	indices[6] = 4; indices[7]  = 5; indices[8]  = 6;
	indices[9] = 4; indices[10] = 6; indices[11] = 7;

	// Fill in the top face index data
	indices[12] = 8; indices[13] =  9; indices[14] = 10;
	indices[15] = 8; indices[16] = 10; indices[17] = 11;

	// Fill in the bottom face index data
	indices[18] = 12; indices[19] = 13; indices[20] = 14;
	indices[21] = 12; indices[22] = 14; indices[23] = 15;

	// Fill in the left face index data
	indices[24] = 16; indices[25] = 17; indices[26] = 18;
	indices[27] = 16; indices[28] = 18; indices[29] = 19;

	// Fill in the right face index data
	indices[30] = 20; indices[31] = 21; indices[32] = 22;
	indices[33] = 20; indices[34] = 22; indices[35] = 23;
}

void CSG::FlipWinding(
					  TArray< BSP::Vertex > & vertices, TArray< UINT16 > & indices
							)
{
	// flip winding to turn the model inside out
	const int numTriangles = indices.Num() / 3;
	for( int i = 0; i < numTriangles; i++ )
	{
		UINT16 * tri = indices.ToPtr() + i*3;
		TSwap( tri[0], tri[2] );
	}

	// flip vertex normals
	for( int iVertex = 0; iVertex < vertices.Num(); iVertex++ )
	{
		UByte4 packed;
		packed = vertices[ iVertex ].N;
		V3F4 N = UnpackNormal( packed );
		N = V3_Normalized( V3_Negate( N ) );
		vertices[ iVertex ].N = PackNormal( N.x, N.y, N.z );
	}
}
