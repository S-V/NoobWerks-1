//NOTE: 'Tc' stands for 'Toolchain'
#pragma once

#include <Base/Template/Containers/Array/DynamicArray.h>
#include <Core/Assets/AssetManagement.h>
#include <Graphics/Public/graphics_formats.h>
#include <Rendering/Public/Core/VertexFormats.h>

namespace Meshok
{

U32 EulerNumber( U32 V, U32 E, U32 F, U32 H = 0 );
bool EulerTest( U32 V, U32 E, U32 F, U32 H, U32 C, U32 G );

/// _voxels - bit array (_gridSize^3 bits)
ERet VoxelizeSphere( const float _radius, const U32 _gridSize, void *_voxels );
//
//ERet VoxelizeMesh( const TcModel& _source,
//				  const U32 _sizeX, const U32 _sizeY, const U32 _sizeZ,
//				  void *_volume
//				  );

}//namespace Meshok

struct ATriangleIndexCallback
{
	struct Vertex {
		V3f xyz;
		V2f st;
	};
	virtual void ProcessTriangle( const Vertex& a, const Vertex& b, const Vertex& c ) = 0;
	virtual ~ATriangleIndexCallback() {}
};

struct ATriangleMeshInterface : DbgNamedObject<>
{
	virtual void ProcessAllTriangles( ATriangleIndexCallback* callback ) = 0;
	virtual ~ATriangleMeshInterface() {}
};

template< typename INDEX_TYPE >
void ProcessAllTriangles(
						   const INDEX_TYPE* indices, const UINT numTriangles,
						   const V3f* positions, const UINT numVertices,
						   ATriangleIndexCallback* callback
						   )
{
	mxSTATIC_ASSERT( sizeof(indices[0]) == 2 || sizeof(indices[0]) == 4 );

	for( UINT iTriangle = 0; iTriangle < numTriangles; iTriangle++ )
	{
		const UINT	idx0 = indices[ iTriangle*3 + 0 ];
		const UINT	idx1 = indices[ iTriangle*3 + 1 ];
		const UINT	idx2 = indices[ iTriangle*3 + 2 ];

		callback->ProcessTriangle(
			positions[idx0],
			positions[idx1],
			positions[idx2]
		);
	}
}

struct AMeshBuilder
{
	virtual ERet Begin() = 0;
	virtual int addVertex( const DrawVertex& vertex ) = 0;
	virtual int addTriangle( int v1, int v2, int v3 ) = 0;
	virtual ERet End( int &verts, int &tris ) = 0;

protected:
	virtual ~AMeshBuilder() {}
};

struct MeshBuilder : AMeshBuilder
{
	TArray< DrawVertex >	vertices;
	TArray< UINT16 >		indices;

	virtual ERet Begin() override;
	virtual int addVertex( const DrawVertex& vertex ) override;
	virtual int addTriangle( int v1, int v2, int v3 ) override;
	virtual ERet End( int &verts, int &tris ) override;
};

struct FatVertex
{
	V3f	position;
	V2f	texCoord;
	V3f	normal;
	V3f	tangent;
};
struct FatVertex2
{
	V3f	position;
	V2f	texCoord;
	V3f	normal;
	V3f	tangent;
	V3f	bitangent;	// 'bitangent' - orthogonal to both the normal and the tangent, B = T x N
};



ERet CreateBox(
			   float width, float height, float depth,
			   TArray<FatVertex> &_vertices, TArray<int> &_indices
			   );

ERet CreateSphere(
				  float radius, int sliceCount, int stackCount,
				  TArray<FatVertex> &_vertices, TArray<int> &_indices
				  );

ERet CreateGeodesicSphere(
						  float radius, int numSubdivisions, //[0..8]
						  TArray<FatVertex> &_vertices, TArray<int> &_indices
						  );


mxSTOLEN("OpenVDB");
// Bit compression method that efficiently represents a unit vector using
// 2 bytes i.e. 16 bits of data by only storing two quantized components.
// Based on "Higher Accuracy Quantized Normals" article from GameDev.Net LLC, 2000

class QuantizedUnitVec
{
public:
    static U16 pack(const V3f& vec);
    static V3f unpack(const U16 data);

    static void flipSignBits(U16&);

    // initialization function for the normalization weights. NOT threadsafe!
    static void StaticInitializeLUT();

private:
    QuantizedUnitVec() {}

    // bit masks
    static const U16 MASK_SLOTS = 0x1FFF; // 0001111111111111
    static const U16 MASK_XSLOT = 0x1F80; // 0001111110000000
    static const U16 MASK_YSLOT = 0x007F; // 0000000001111111
    static const U16 MASK_XSIGN = 0x8000; // 1000000000000000
    static const U16 MASK_YSIGN = 0x4000; // 0100000000000000
    static const U16 MASK_ZSIGN = 0x2000; // 0010000000000000

    // normalization weights, 32 kilobytes.
    static float sNormalizationWeights[MASK_SLOTS + 1];
}; // class QuantizedUnitVec


////////////////////////////////////////

inline U16
QuantizedUnitVec::pack(const V3f& vec)
{
    mxASSERT(V3_LengthSquared(vec) > 0.0f);

    U16 data = 0;
    F32 x(vec[0]), y(vec[1]), z(vec[2]);

    // The sign of the three components are first stored using
    // 3-bits and can then safely be discarded.
    if (x < F32(0.0)) { data |= MASK_XSIGN; x = -x; }
    if (y < F32(0.0)) { data |= MASK_YSIGN; y = -y; }
    if (z < F32(0.0)) { data |= MASK_ZSIGN; z = -z; }

    // The z component is discarded and x & y are quantized in
    // the 0 to 126 range.
    F32 w = F32(126.0) / (x + y + z);
    U16 xbits = static_cast<U16>((x * w));
    U16 ybits = static_cast<U16>((y * w));

    // The remaining 13 bits in our 16 bit word are divided into a
    // 6-bit x-slot and a 7-bit y-slot. Both the xbits and the ybits
    // can still be represented using (2^7 - 1) quantization levels.

    // If the xbits require more than 6-bits, store the complement.
    // (xbits + ybits < 127, thus if xbits > 63 => ybits <= 63)
    if(xbits > 63) {
        xbits = static_cast<U16>(127 - xbits);
        ybits = static_cast<U16>(127 - ybits);
    }

    // Pack components into their respective slots.
    data = static_cast<U16>(data | (xbits << 7));
    data = static_cast<U16>(data | ybits);
    return data;
}


inline V3f
QuantizedUnitVec::unpack(const U16 data)
{
    const float w = sNormalizationWeights[data & MASK_SLOTS];

    U16 xbits = static_cast<U16>((data & MASK_XSLOT) >> 7);
    U16 ybits = static_cast<U16>(data & MASK_YSLOT);

    // Check if the complement components where stored and revert.
    if ((xbits + ybits) > 126) {
        xbits = static_cast<U16>(127 - xbits);
        ybits = static_cast<U16>(127 - ybits);
    }

	CV3f vec(
		float(xbits) * w,
		float(ybits) * w,
		float(126 - xbits - ybits) * w
	);

    if (data & MASK_XSIGN) vec[0] = -vec[0];
    if (data & MASK_YSIGN) vec[1] = -vec[1];
    if (data & MASK_ZSIGN) vec[2] = -vec[2];
    return vec;
}


////////////////////////////////////////


inline void
QuantizedUnitVec::flipSignBits(U16& v)
{
    v = static_cast<U16>((v & MASK_SLOTS) | (~v & ~MASK_SLOTS));
}

//
//	TraversalType
//
enum TraversalType
{
	Traverse_PreOrder,	// Visit the root. Traverse the left subtree. Traverse the right subtree.
	Traverse_PostOrder,	// Traverse the left subtree. Traverse the right subtree. Visit the root.
	Traverse_InOrder,	// Traverse the left subtree. Visit the root. Traverse the right subtree.
};

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
