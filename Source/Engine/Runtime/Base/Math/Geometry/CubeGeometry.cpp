// Cube-related constants.
#include <Base/Base.h>
#pragma hdrstop

#include <Base/Text/TextUtils.h>	// Dev_PrintArrayAsHex
#include <Base/Math/Geometry/CubeGeometry.h>
#include <Base/Math/Axes.h>


namespace CubeFace
{
String64 DbgMaskToString( int mask )
{
	String64	result;

	if( mask & BIT(CubeFace::PosX) ) {
		Str::Append( result, "+X, " );
	}
	if( mask & BIT(CubeFace::NegX) ) {
		Str::Append( result, "-X, " );
	}

	if( mask & BIT(CubeFace::PosY) ) {
		Str::Append( result, "+Y, " );
	}
	if( mask & BIT(CubeFace::NegY) ) {
		Str::Append( result, "-Y, " );
	}

	if( mask & BIT(CubeFace::PosZ) ) {
		Str::Append( result, "+Z, " );
	}
	if( mask & BIT(CubeFace::NegZ) ) {
		Str::Append( result, "-Z, " );
	}

	// removing trailing comma
	result.capLength( result.length() - 2 );

	return result;
}
}//namespace CubeFace

mxBEGIN_REFLECT_ENUM( CubeFaceEnum )
	mxREFLECT_ENUM_ITEM( NegX, CubeFace::NegX ),
	mxREFLECT_ENUM_ITEM( PosX, CubeFace::PosX ),
	mxREFLECT_ENUM_ITEM( NegY, CubeFace::NegY ),
	mxREFLECT_ENUM_ITEM( PosY, CubeFace::PosY ),
	mxREFLECT_ENUM_ITEM( NegZ, CubeFace::NegZ ),
	mxREFLECT_ENUM_ITEM( PosZ, CubeFace::PosZ ),
mxEND_REFLECT_ENUM

mxBEGIN_FLAGS( CubeFaceFlags )
	mxREFLECT_BIT( NegX, BIT(CubeFace::NegX) ),
	mxREFLECT_BIT( PosX, BIT(CubeFace::PosX) ),
	mxREFLECT_BIT( NegY, BIT(CubeFace::NegY) ),
	mxREFLECT_BIT( PosY, BIT(CubeFace::PosY) ),
	mxREFLECT_BIT( NegZ, BIT(CubeFace::NegZ) ),
	mxREFLECT_BIT( PosZ, BIT(CubeFace::PosZ) ),
mxEND_FLAGS

/// For each sign configuration: 12-bit bitmask of intersected cube edges.
mxPREALIGN(16) const U16	g_LUT_cellcase_to_edge_mask[256] =
{
	0x0000, 0x3111, 0x3221, 0x4330, 0x3412, 0x4503, 0x6633, 0x5722, 
	0x3822, 0x6933, 0x4a03, 0x5b12, 0x4c30, 0x5d21, 0x5e11, 0x4f00, 
	0x3144, 0x4055, 0x6365, 0x5274, 0x6556, 0x5447, 0x9777, 0x6666, 
	0x6966, 0x7877, 0x7b47, 0x6a56, 0x7d74, 0x6c65, 0x8f55, 0x5e44, 
	0x3284, 0x6395, 0x40a5, 0x51b4, 0x6696, 0x7787, 0x74b7, 0x65a6, 
	0x6aa6, 0x9bb7, 0x5887, 0x6996, 0x7eb4, 0x8fa5, 0x6c95, 0x5d84, 
	0x43c0, 0x52d1, 0x51e1, 0x40f0, 0x77d2, 0x66c3, 0x85f3, 0x54e2, 
	0x7be2, 0x8af3, 0x69c3, 0x58d2, 0x8ff0, 0x7ee1, 0x7dd1, 0x4cc0, 
	0x3448, 0x6559, 0x6669, 0x7778, 0x405a, 0x514b, 0x727b, 0x636a, 
	0x6c6a, 0x9d7b, 0x7e4b, 0x8f5a, 0x5878, 0x6969, 0x6a59, 0x5b48, 
	0x450c, 0x541d, 0x772d, 0x663c, 0x511e, 0x400f, 0x833f, 0x522e, 
	0x7d2e, 0x8c3f, 0x8f0f, 0x7e1e, 0x693c, 0x582d, 0x7b1d, 0x4a0c, 
	0x66cc, 0x97dd, 0x74ed, 0x85fc, 0x72de, 0x83cf, 0x80ff, 0x71ee, 
	0x9eee, 0xcfff, 0x8ccf, 0x9dde, 0x8afc, 0x9bed, 0x78dd, 0x69cc, 
	0x5788, 0x6699, 0x65a9, 0x54b8, 0x639a, 0x528b, 0x71bb, 0x40aa, 
	0x8faa, 0x9ebb, 0x7d8b, 0x6c9a, 0x7bb8, 0x6aa9, 0x6999, 0x3888, 
	0x3888, 0x6999, 0x6aa9, 0x7bb8, 0x6c9a, 0x7d8b, 0x9ebb, 0x8faa, 
	0x40aa, 0x71bb, 0x528b, 0x639a, 0x54b8, 0x65a9, 0x6699, 0x5788, 
	0x69cc, 0x78dd, 0x9bed, 0x8afc, 0x9dde, 0x8ccf, 0xcfff, 0x9eee, 
	0x71ee, 0x80ff, 0x83cf, 0x72de, 0x85fc, 0x74ed, 0x97dd, 0x66cc, 
	0x4a0c, 0x7b1d, 0x582d, 0x693c, 0x7e1e, 0x8f0f, 0x8c3f, 0x7d2e, 
	0x522e, 0x833f, 0x400f, 0x511e, 0x663c, 0x772d, 0x541d, 0x450c, 
	0x5b48, 0x6a59, 0x6969, 0x5878, 0x8f5a, 0x7e4b, 0x9d7b, 0x6c6a, 
	0x636a, 0x727b, 0x514b, 0x405a, 0x7778, 0x6669, 0x6559, 0x3448, 
	0x4cc0, 0x7dd1, 0x7ee1, 0x8ff0, 0x58d2, 0x69c3, 0x8af3, 0x7be2, 
	0x54e2, 0x85f3, 0x66c3, 0x77d2, 0x40f0, 0x51e1, 0x52d1, 0x43c0, 
	0x5d84, 0x6c95, 0x8fa5, 0x7eb4, 0x6996, 0x5887, 0x9bb7, 0x6aa6, 
	0x65a6, 0x74b7, 0x7787, 0x6696, 0x51b4, 0x40a5, 0x6395, 0x3284, 
	0x5e44, 0x8f55, 0x6c65, 0x7d74, 0x6a56, 0x7b47, 0x7877, 0x6966, 
	0x6666, 0x9777, 0x5447, 0x6556, 0x5274, 0x6365, 0x4055, 0x3144, 
	0x4f00, 0x5e11, 0x5d21, 0x4c30, 0x5b12, 0x4a03, 0x6933, 0x3822, 
	0x5722, 0x6633, 0x4503, 0x3412, 0x4330, 0x3221, 0x3111, 0x0000, 
};



#if vxCFG_CUBE_LIB_EXPOSE_LUT_GENERATORS

void Offline_Build_LUT_cellcase_to_edge_mask()
{
	U16	edge_masks[256];

	// for each cube configuration (out of 256)
	for( UINT cell_case = 0; cell_case < 256; cell_case++ )
	{
		U32	edge_mask = 0;
		U32	num_intersecting_edges = 0;

		// for each edge of the cube
		for( UINT iEdge = 0; iEdge < 12; iEdge++ )
		{
			// record intersecting edges
			ECubeCorner iVertexA, iVertexB;
			Cube_Endpoints_from_Edge( (ECubeEdge)iEdge, iVertexA, iVertexB );

			// compare signs at edge endpoints (1 if the corner is inside, 0 if outside)
			const UINT signA = (cell_case >> iVertexA) & 1;
			const UINT signB = (cell_case >> iVertexB) & 1;

			if( signA != signB )
			{
				edge_mask |= (1 << iEdge);
				++num_intersecting_edges;
			}
		}

		edge_masks[ cell_case ] = edge_mask | (num_intersecting_edges << 12);

	}//For each cell/sign configuration.

	//	DBGOUT("Max %d intersecting edges", maxIntersectingEdges);

	Dev_PrintArrayAsHex(
		edge_masks, 256
		, 8, true
		);
}

#endif // vxCFG_CUBE_LIB_EXPOSE_LUT_GENERATORS


mxBEGIN_REFLECT_ENUM( CellVertexTypeE )
	mxREFLECT_ENUM_ITEM( Internal, ECellType::Cell_Internal ),

	mxREFLECT_ENUM_ITEM( FaceNegX, ECellType::Cell_FaceNegX ),
	mxREFLECT_ENUM_ITEM( FacePosX, ECellType::Cell_FacePosX ),
	mxREFLECT_ENUM_ITEM( FaceNegY, ECellType::Cell_FaceNegY ),
	mxREFLECT_ENUM_ITEM( FacePosY, ECellType::Cell_FacePosY ),
	mxREFLECT_ENUM_ITEM( FaceNegZ, ECellType::Cell_FaceNegZ ),
	mxREFLECT_ENUM_ITEM( FacePosZ, ECellType::Cell_FacePosZ ),

	mxREFLECT_ENUM_ITEM( Edge0, ECellType::Cell_Edge0 ),
	mxREFLECT_ENUM_ITEM( Edge1, ECellType::Cell_Edge1 ),
	mxREFLECT_ENUM_ITEM( Edge2, ECellType::Cell_Edge2 ),
	mxREFLECT_ENUM_ITEM( Edge3, ECellType::Cell_Edge3 ),
	mxREFLECT_ENUM_ITEM( Edge4, ECellType::Cell_Edge4 ),
	mxREFLECT_ENUM_ITEM( Edge5, ECellType::Cell_Edge5 ),
	mxREFLECT_ENUM_ITEM( Edge6, ECellType::Cell_Edge6 ),
	mxREFLECT_ENUM_ITEM( Edge7, ECellType::Cell_Edge7 ),
	mxREFLECT_ENUM_ITEM( Edge8, ECellType::Cell_Edge8 ),
	mxREFLECT_ENUM_ITEM( Edge9, ECellType::Cell_Edge9 ),
	mxREFLECT_ENUM_ITEM( EdgeA, ECellType::Cell_EdgeA ),
	mxREFLECT_ENUM_ITEM( EdgeB, ECellType::Cell_EdgeB ),

	mxREFLECT_ENUM_ITEM( Corner0, ECellType::Cell_Corner0 ),
	mxREFLECT_ENUM_ITEM( Corner1, ECellType::Cell_Corner1 ),
	mxREFLECT_ENUM_ITEM( Corner2, ECellType::Cell_Corner2 ),
	mxREFLECT_ENUM_ITEM( Corner3, ECellType::Cell_Corner3 ),
	mxREFLECT_ENUM_ITEM( Corner4, ECellType::Cell_Corner4 ),
	mxREFLECT_ENUM_ITEM( Corner5, ECellType::Cell_Corner5 ),
	mxREFLECT_ENUM_ITEM( Corner6, ECellType::Cell_Corner6 ),
	mxREFLECT_ENUM_ITEM( Corner7, ECellType::Cell_Corner7 ),
mxEND_REFLECT_ENUM

mxBEGIN_FLAGS( CellVertexTypeF )
	mxREFLECT_BIT( Internal, BIT(ECellType::Cell_Internal) ),

	mxREFLECT_BIT( FaceNegX, BIT(ECellType::Cell_FaceNegX) ),
	mxREFLECT_BIT( FacePosX, BIT(ECellType::Cell_FacePosX) ),
	mxREFLECT_BIT( FaceNegY, BIT(ECellType::Cell_FaceNegY) ),
	mxREFLECT_BIT( FacePosY, BIT(ECellType::Cell_FacePosY) ),
	mxREFLECT_BIT( FaceNegZ, BIT(ECellType::Cell_FaceNegZ) ),
	mxREFLECT_BIT( FacePosZ, BIT(ECellType::Cell_FacePosZ) ),

	mxREFLECT_BIT( Edge0, BIT(ECellType::Cell_Edge0) ),
	mxREFLECT_BIT( Edge1, BIT(ECellType::Cell_Edge1) ),
	mxREFLECT_BIT( Edge2, BIT(ECellType::Cell_Edge2) ),
	mxREFLECT_BIT( Edge3, BIT(ECellType::Cell_Edge3) ),
	mxREFLECT_BIT( Edge4, BIT(ECellType::Cell_Edge4) ),
	mxREFLECT_BIT( Edge5, BIT(ECellType::Cell_Edge5) ),
	mxREFLECT_BIT( Edge6, BIT(ECellType::Cell_Edge6) ),
	mxREFLECT_BIT( Edge7, BIT(ECellType::Cell_Edge7) ),
	mxREFLECT_BIT( Edge8, BIT(ECellType::Cell_Edge8) ),
	mxREFLECT_BIT( Edge9, BIT(ECellType::Cell_Edge9) ),
	mxREFLECT_BIT( EdgeA, BIT(ECellType::Cell_EdgeA) ),
	mxREFLECT_BIT( EdgeB, BIT(ECellType::Cell_EdgeB) ),

	mxREFLECT_BIT( Corner0, BIT(ECellType::Cell_Corner0) ),
	mxREFLECT_BIT( Corner1, BIT(ECellType::Cell_Corner1) ),
	mxREFLECT_BIT( Corner2, BIT(ECellType::Cell_Corner2) ),
	mxREFLECT_BIT( Corner3, BIT(ECellType::Cell_Corner3) ),
	mxREFLECT_BIT( Corner4, BIT(ECellType::Cell_Corner4) ),
	mxREFLECT_BIT( Corner5, BIT(ECellType::Cell_Corner5) ),
	mxREFLECT_BIT( Corner6, BIT(ECellType::Cell_Corner6) ),
	mxREFLECT_BIT( Corner7, BIT(ECellType::Cell_Corner7) ),
mxEND_FLAGS

namespace CubeEdges
{
	const Int3 GetEdgeNeighborOffset(const ECubeEdge cube_edge)
	{
		// could be optimized
		const EAxis edge_axis = EAxis_from_CubeEdge(cube_edge);

		const UINT mask = InsertZeroBitAtAxisPosition_Branchless(
			cube_edge & 0x3,	// == cube_edge % 4
			edge_axis
			);

		Int3	result;
		result.a[ 0 ] = (mask & BIT(0)) ? +1 : -1;
		result.a[ 1 ] = (mask & BIT(1)) ? +1 : -1;
		result.a[ 2 ] = (mask & BIT(2)) ? +1 : -1;

		result.a[ edge_axis ] = 0;

		return result;
	}
}



mxBEGIN_REFLECT_ENUM( CubeNeighborE )
	mxREFLECT_ENUM_ITEM( FaceNegX, CubeNeighbor::FaceNegX ),
	mxREFLECT_ENUM_ITEM( FacePosX, CubeNeighbor::FacePosX ),
	mxREFLECT_ENUM_ITEM( FaceNegY, CubeNeighbor::FaceNegY ),
	mxREFLECT_ENUM_ITEM( FacePosY, CubeNeighbor::FacePosY ),
	mxREFLECT_ENUM_ITEM( FaceNegZ, CubeNeighbor::FaceNegZ ),
	mxREFLECT_ENUM_ITEM( FacePosZ, CubeNeighbor::FacePosZ ),

	mxREFLECT_ENUM_ITEM( Edge0, CubeNeighbor::Edge0 ),
	mxREFLECT_ENUM_ITEM( Edge1, CubeNeighbor::Edge1 ),
	mxREFLECT_ENUM_ITEM( Edge2, CubeNeighbor::Edge2 ),
	mxREFLECT_ENUM_ITEM( Edge3, CubeNeighbor::Edge3 ),
	mxREFLECT_ENUM_ITEM( Edge4, CubeNeighbor::Edge4 ),
	mxREFLECT_ENUM_ITEM( Edge5, CubeNeighbor::Edge5 ),
	mxREFLECT_ENUM_ITEM( Edge6, CubeNeighbor::Edge6 ),
	mxREFLECT_ENUM_ITEM( Edge7, CubeNeighbor::Edge7 ),
	mxREFLECT_ENUM_ITEM( Edge8, CubeNeighbor::Edge8 ),
	mxREFLECT_ENUM_ITEM( Edge9, CubeNeighbor::Edge9 ),
	mxREFLECT_ENUM_ITEM( EdgeA, CubeNeighbor::EdgeA ),
	mxREFLECT_ENUM_ITEM( EdgeB, CubeNeighbor::EdgeB ),

	mxREFLECT_ENUM_ITEM( Corner0, CubeNeighbor::Corner0 ),
	mxREFLECT_ENUM_ITEM( Corner1, CubeNeighbor::Corner1 ),
	mxREFLECT_ENUM_ITEM( Corner2, CubeNeighbor::Corner2 ),
	mxREFLECT_ENUM_ITEM( Corner3, CubeNeighbor::Corner3 ),
	mxREFLECT_ENUM_ITEM( Corner4, CubeNeighbor::Corner4 ),
	mxREFLECT_ENUM_ITEM( Corner5, CubeNeighbor::Corner5 ),
	mxREFLECT_ENUM_ITEM( Corner6, CubeNeighbor::Corner6 ),
	mxREFLECT_ENUM_ITEM( Corner7, CubeNeighbor::Corner7 ),
mxEND_REFLECT_ENUM


namespace CubeNeighbor
{
String128 DbgMaskToString(const Cube26NeighborsMask& cube_neighbors_mask)
{
	String128	result;

	//
	for(UINT i = 0; i < CubeNeighbor::Count; i++)
	{
		if(cube_neighbors_mask.u & BIT(i))
		{
			const char* name = CubeNeighborE_Type().GetString(i);
			Str::SAppendF( result, "%s, ", name );
		}
	}

	// removing trailing comma
	result.capLength( result.length() - 2 );

	return result;
}

//const U8	g_OppositeCellType[CellTypeCount27] =
//{
//	ECellType::Internal,
//	//
//	FacePosX,
//	FaceNegX,
//	FacePosY,
//	FaceNegY,
//	FacePosZ,
//	FaceNegZ,
//	//
//	EdgeNum2, // EdgeNum0
//	EdgeNum3, // EdgeNum1
//	EdgeNum0, // EdgeNum2
//	EdgeNum1, // EdgeNum3
//	EdgeNum6, // EdgeNum4
//	EdgeNum7, // EdgeNum5
//	EdgeNum4, // EdgeNum6
//	EdgeNum5, // EdgeNum7
//	EdgeNumA, // EdgeNum8
//	EdgeNumB, // EdgeNum9
//	EdgeNum8, // EdgeNumA
//	EdgeNum9, // EdgeNumB
//	//
//	Corner7, // Corner0,
//	Corner6, // Corner1,
//	Corner5, // Corner2,
//	Corner4, // Corner3,
//	Corner3, // Corner4,
//	Corner2, // Corner5,
//	Corner1, // Corner6,
//	Corner0, // Corner7,
//};

}//namespace CubeNeighbor


void get_Coords_of_26_Neighbors(
	Int3 neighbors_coords_[CubeNeighbor::Count]
	, const Int3& my_coords
)
{
	// 6 face-adjacent neighbors
	for( UINT iFace = 0; iFace < 6; iFace++ )
	{
		neighbors_coords_[ iFace ] = my_coords
			+ CubeFaceNeighborOffset( (CubeFace::Enum) iFace );
	}//face neighbors

	// 12 edge-adjacent neighbors
	for( UINT iEdge = 0; iEdge < 12; iEdge++ )
	{
		neighbors_coords_[ CubeFace::Count + iEdge ] = my_coords
			+ CubeEdges::GetEdgeNeighborOffset( (ECubeEdge)iEdge );
	}//edge neighbors

	// 8 corner-adjacent neighbors
	for( UINT iCorner = 0; iCorner < 8; iCorner++ )
	{
		neighbors_coords_[ CubeFace::Count + CubeEdgeCount + iCorner ] = my_coords
			+ Cube_GetCornerNeighborOffset( (ECubeCorner) iCorner );
	}//corner neighbors
}
