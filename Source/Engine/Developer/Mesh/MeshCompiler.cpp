#include <Base/Base.h>
#pragma hdrstop
#include <DirectXMath/DirectXMath.h>
#include <Base/Math/MiniMath.h>
#include <Base/Math/Quaternion.h>
#include <Base/Template/Containers/BitSet/BitArray.h>
#include <Core/Memory.h>
#include <Core/Text/TextWriter.h>	// Debug_SaveToObj
#include <Developer/Mesh/MeshCompiler.h>


/*
-----------------------------------------------------------------------------
	TcWeight
-----------------------------------------------------------------------------
*/
mxDEFINE_CLASS( TcWeight );
mxBEGIN_REFLECTION( TcWeight )
	mxMEMBER_FIELD( boneIndex ),
	mxMEMBER_FIELD( boneWeight ),
mxEND_REFLECTION
TcWeight::TcWeight()
{
	boneIndex = 0;
	boneWeight = 0.0f;
}

/*
-----------------------------------------------------------------------------
	MeshBone
-----------------------------------------------------------------------------
*/
mxDEFINE_CLASS( TcBone );
mxBEGIN_REFLECTION( TcBone )
	mxMEMBER_FIELD( name ),
	mxMEMBER_FIELD( parent ),
	mxMEMBER_FIELD( rotation ),
	mxMEMBER_FIELD( translation ),
mxEND_REFLECTION
TcBone::TcBone()
{
	parent = -1;
	rotation = Quaternion_Identity();
	translation = V3_Zero();
}

mxDEFINE_CLASS( TcSkeleton );
mxBEGIN_REFLECTION( TcSkeleton )
	mxMEMBER_FIELD( bones ),
mxEND_REFLECTION
int TcSkeleton::FindBoneIndexByName( const char* boneName ) const
{
	for( UINT i = 0; i < bones.num(); i++ ) {
		if( Str::EqualS(bones[i].name, boneName) ) {
			return i;
		}
	}
	return -1;
}

/*
-----------------------------------------------------------------------------
	MeshPart
-----------------------------------------------------------------------------
*/
//mxDEFINE_CLASS_NO_DEFAULT_CTOR( TcTriMesh );
//mxBEGIN_REFLECTION( TcTriMesh )
//	mxMEMBER_FIELD( name ),
//	mxMEMBER_FIELD( positions ),
//	mxMEMBER_FIELD( texCoords ),
//	mxMEMBER_FIELD( tangents ),
//	mxMEMBER_FIELD( binormals ),
//	mxMEMBER_FIELD( normals ),
//	mxMEMBER_FIELD( colors ),
//	mxMEMBER_FIELD( weights ),
//	mxMEMBER_FIELD( indices ),
//	mxMEMBER_FIELD( aabb ),
//	//mxMEMBER_FIELD( sphere ),
//	//mxMEMBER_FIELD( material ),
//mxEND_REFLECTION
TcTriMesh::TcTriMesh( AllocatorI & _allocator )
	: positions( _allocator )
	, texCoords( _allocator )
	, tangents( _allocator )
	, binormals( _allocator )
	, normals( _allocator )
	, colors( _allocator )
	, weights( _allocator )
	, indices( _allocator )
{
	aabb.clear();
}

static void transformNormalsIfNotEmpty(
							 DynamicArray< V3f > & normals,
							 const M44f& transform_matrix
							 )
{
	if( normals.num() )
	{
		XMVector3TransformNormalStream(
			(DirectX::XMFLOAT3*) normals.raw(),
			sizeof(normals[0]),
			(DirectX::XMFLOAT3*) normals.raw(),
			sizeof(normals[0]),
			normals.num(),
			(DirectX::CXMMATRIX) transform_matrix
			);
	}
}

void TcTriMesh::transformVerticesBy( const M44f& transform_matrix )
{
	if( positions.num() )
	{
		XMVector3TransformCoordStream(
			(DirectX::XMFLOAT3*) positions.raw(),
			sizeof(positions[0]),
			(DirectX::XMFLOAT3*) positions.raw(),
			sizeof(positions[0]),
			positions.num(),
			(DirectX::CXMMATRIX) transform_matrix
			);
	}

	transformNormalsIfNotEmpty( tangents, transform_matrix );
	transformNormalsIfNotEmpty( binormals, transform_matrix );
	transformNormalsIfNotEmpty( normals, transform_matrix );
}

ERet TcTriMesh::CopyFrom(const TcTriMesh& other)
{
	this->name = other.name;

	this->positions = other.positions;
	this->texCoords = other.texCoords;
	this->tangents = other.tangents;
	this->binormals = other.binormals;
	this->normals = other.normals;
	this->colors = other.colors;
	this->weights = other.weights;

	this->indices = other.indices;

	this->aabb = other.aabb;

	this->material = other.material;

	return ALL_OK;
}

ERet TcTriMesh::Debug_SaveToObj( const char* filename ) const
{
	FileWriter	file;
	mxDO(file.Open(filename));

	TextWriter	writer(file);

	writer.Emitf("# %d vertices\n", this->positions._count);
	for( UINT iVertex = 0; iVertex < this->positions._count; iVertex++ )
	{
		const V3f& pos = this->positions._data[ iVertex ];
		writer.Emitf("v %lf %lf %lf\n", pos.x, pos.y, pos.z);
	}

	//
	const U32 num_tris = this->indices._count / 3;

	writer.Emitf("# %d triangles\n", num_tris);
	for( UINT iFace = 0; iFace < num_tris; iFace++ )
	{
		writer.Emitf("f %d// %d// %d//\n",
			this->indices._data[ iFace*3 + 0 ]+1,
			this->indices._data[ iFace*3 + 1 ]+1,
			this->indices._data[ iFace*3 + 2 ]+1
			);
	}
	return ALL_OK;
}


mxDEFINE_CLASS( TcVecKey );
mxBEGIN_REFLECTION( TcVecKey )
	mxMEMBER_FIELD( data ),
	mxMEMBER_FIELD( time ),
mxEND_REFLECTION

mxDEFINE_CLASS( TcQuatKey );
mxBEGIN_REFLECTION( TcQuatKey )
	mxMEMBER_FIELD( data ),
	mxMEMBER_FIELD( time ),
mxEND_REFLECTION

mxDEFINE_CLASS( TcAnimChannel );
mxBEGIN_REFLECTION( TcAnimChannel )
	mxMEMBER_FIELD( target ),
	mxMEMBER_FIELD( positionKeys ),
	mxMEMBER_FIELD( rotationKeys ),
	mxMEMBER_FIELD( scalingKeys ),
mxEND_REFLECTION

mxDEFINE_CLASS( TcAnimation );
mxBEGIN_REFLECTION( TcAnimation )
	mxMEMBER_FIELD( name ),
	mxMEMBER_FIELD( channels ),
	mxMEMBER_FIELD( duration ),
	mxMEMBER_FIELD( numFrames ),
mxEND_REFLECTION
TcAnimation::TcAnimation()
{
	duration = 0.0f;
	numFrames = 0;
}
const TcAnimChannel* TcAnimation::FindChannelByName( const char* name ) const
{
	for( UINT i = 0; i < channels.num(); i++ ) {
		if(Str::EqualS(channels[i].target, name)) {
			return &channels[i];
		}
	}
	return nil;
}

/*
-----------------------------------------------------------------------------
	MeshData
-----------------------------------------------------------------------------
*/
TcModel::TcModel( AllocatorI & allocator )
	: meshes( allocator )
	, animations( allocator )
	, allocator( allocator )
{
	AABBf_Clear( &bounds );
}

void TcModel::transformVertices( const M44f& affine_matrix )
{
	const M44f& matrixIT = M44_Transpose( M44_OrthoInverse(affine_matrix) );

	for( UINT iMesh = 0; iMesh < meshes.num(); iMesh++ )
	{
		TcTriMesh& mesh = *meshes[ iMesh ];

		for( UINT iPos = 0; iPos < mesh.positions.num(); iPos++ )
		{
			mesh.positions[iPos] = affine_matrix.transformPoint( mesh.positions[iPos] );

			if(!mesh.normals.IsEmpty()) {
				mesh.normals[iPos] = M44_TransformNormal(matrixIT, mesh.normals[iPos]);
				mesh.normals[iPos].normalize();
			}

			if(!mesh.tangents.IsEmpty()) {
				mesh.tangents[iPos] = M44_TransformNormal(matrixIT, mesh.tangents[iPos]);
				mesh.tangents[iPos].normalize();
			}

			if(!mesh.binormals.IsEmpty()) {
				mesh.binormals[iPos] = M44_TransformNormal(matrixIT, mesh.binormals[iPos]);
				mesh.binormals[iPos].normalize();
			}
		}

	}//For each mesh.
}

void TcModel::RecomputeAABB()
{
	AABBf	aabb;
	aabb.clear();

	for( UINT iMesh = 0; iMesh < meshes.num(); iMesh++ )
	{
		TcTriMesh& mesh = *meshes[ iMesh ];

		for( UINT iPos = 0; iPos < mesh.positions.num(); iPos++ )
		{
			aabb.addPoint( mesh.positions[iPos] );
		}

	}//For each mesh.

	this->bounds = aabb;
}

#if 0
ERet TcModel::Debug_SaveToObj( const char* filename ) const
{
	FileWriter	file;
	mxDO(file.Open(filename));

	//
	TextWriter	writer(file);

	UINT	total_num_vertices = 0;
	UINT	total_num_indices = 0;

	for( UINT iMesh = 0; iMesh < meshes.num(); iMesh++ )
	{
		const TcTriMesh& mesh = *meshes[ iMesh ];

		total_num_vertices += mesh.positions.num();
		total_num_indices += mesh.indices.num();
		
	}//For each mesh.

	for( UINT iMesh = 0; iMesh < meshes.num(); iMesh++ )
	{
		const TcTriMesh& mesh = *meshes[ iMesh ];

		writer.Emitf("# %d vertices\n", this->vertices.num());
		for( UINT iVertex = 0; iVertex < this->vertices.num(); iVertex++ )
		{
			const V3f& pos = this->vertices[ iVertex ].xyz;
			writer.Emitf("v %lf %lf %lf\n", pos.x, pos.y, pos.z);
		}

		writer.Emitf("# %d triangles\n", this->triangles.num());
		for( UINT iFace = 0; iFace < this->triangles.num(); iFace++ )
		{
			const Triangle& face = this->triangles[ iFace ];
			writer.Emitf("f %d// %d// %d//\n", face.a[0]+1, face.a[1]+1, face.a[2]+1);
		}
	}//For each mesh.


	return ALL_OK;
}
#endif
void CalculateTotalVertexIndexCount( const TcModel& data, UINT &vertexCount, UINT &index_count )
{
	vertexCount = 0;
	index_count = 0;
	for( UINT iSubMesh = 0; iSubMesh < data.meshes.num(); iSubMesh++ )
	{
		const TcTriMesh* submesh = data.meshes[ iSubMesh ];
		vertexCount += submesh->NumVertices();
		index_count += submesh->NumIndices();
	}
}

namespace MeshLib
{

// keeps attribute indices into VertexDescription::attribsArray
struct VertexStream
{
	TFixedArray< U8, LLGL_MAX_VERTEX_ATTRIBS >	components;
};

static UINT GatherStreams( const NwVertexDescription& vertex, TArray< VertexStream > &streams )
{
	UINT numVertexStreams = 0;

	for( UINT attribIndex = 0; attribIndex < vertex.attribCount; attribIndex++ )
	{
		const NwVertexElement& attrib = vertex.attribsArray[ attribIndex ];

		numVertexStreams = largest( numVertexStreams, attrib.inputSlot + 1 );

		streams.setNum(numVertexStreams);
		VertexStream &stream = streams[ attrib.inputSlot ];

		stream.components.add( attribIndex );
	}

	return numVertexStreams;
}

// Converts the given floating-point data into the specified dst_type format.
// src_dimension - dimension of the source data (size of vector: 1,2,3,4).
// _dstDimension is assumed to be less than or equal to src_dimension.
// if input_is_normalized is true then the source data is assumed to be normalized
// to the range of [-1..+1] if input_is_positive is false,
// and to the range of [0..1] if input_is_positive is true.
// if input_is_normalized is false then input_is_positive is ignored.
//
static void PackVertexAttribF( const float* src_ptr, UINT src_dimension,
					   void *dst_ptr, NwAttributeType8 dst_type,
					   bool input_is_normalized, bool input_is_positive )
{
	const UINT _dstDimension = NwAttributeType::GetDimension(dst_type);
	const float* src_ptr_f = src_ptr;
	const UINT minCommonDim = smallest(src_dimension, _dstDimension);
	mxASSERT(minCommonDim <= 4);
	//const UINT leftOver = _dstDimension - minCommonDim;
	switch( dst_type )
	{
	case NwAttributeType::Byte4 :
	case NwAttributeType::UByte4 :
		{
			U8* dstPtr8 = (U8*)dst_ptr;
			if( input_is_normalized )
			{
				if( input_is_positive )
				{
					// [0..1] float => [0..255] integer, scale: f * 255.0f
					switch(minCommonDim)
					{
					default:	*dstPtr8++ = (U8)( *src_ptr_f++ * 255.0f );
					case 3:		*dstPtr8++ = (U8)( *src_ptr_f++ * 255.0f );
					case 2:		*dstPtr8++ = (U8)( *src_ptr_f++ * 255.0f );
					case 1:		*dstPtr8++ = (U8)( *src_ptr_f++ * 255.0f );
					}
				}
				else
				{
					// [-1..+1] float => [0..255] integer, scale and bias: ((f + 1.0f) * 0.5f) * 255.0f
					switch(minCommonDim)
					{
					default:	*dstPtr8++ = (U8)( *src_ptr_f++ * 127.5f + 127.5f );
					case 3:		*dstPtr8++ = (U8)( *src_ptr_f++ * 127.5f + 127.5f );
					case 2:		*dstPtr8++ = (U8)( *src_ptr_f++ * 127.5f + 127.5f );
					case 1:		*dstPtr8++ = (U8)( *src_ptr_f++ * 127.5f + 127.5f );
					}
				}
			}
			else
			{
				switch(minCommonDim)
				{
				default:	*dstPtr8++ = (U8)( *src_ptr_f++ );
				case 3:		*dstPtr8++ = (U8)( *src_ptr_f++ );
				case 2:		*dstPtr8++ = (U8)( *src_ptr_f++ );
				case 1:		*dstPtr8++ = (U8)( *src_ptr_f++ );
				}
			}
		}
		break;

	case NwAttributeType::Short2 :
		UNDONE;
	case NwAttributeType::UShort2 :
		{
			mxASSERT(minCommonDim <= 2);

			UINT16* dstPtr16 = (UINT16*)dst_ptr;
			if( input_is_normalized )
			{
				// [-1..+1] float => [0..65535] int
				// scale and bias: ((f + 1.0f) * 0.5f) * 65535.0f
				switch(minCommonDim)
				{
				default:	*dstPtr16++ = (UINT16)( *src_ptr_f++ * 32767.5f + 32767.5f );
				case 3:		*dstPtr16++ = (UINT16)( *src_ptr_f++ * 32767.5f + 32767.5f );
				case 2:		*dstPtr16++ = (UINT16)( *src_ptr_f++ * 32767.5f + 32767.5f );
				case 1:		*dstPtr16++ = (UINT16)( *src_ptr_f++ * 32767.5f + 32767.5f );
				}
			}
			else
			{
				switch(minCommonDim)
				{
				default:	*dstPtr16++ = (UINT16)( *src_ptr_f++ );
				case 3:		*dstPtr16++ = (UINT16)( *src_ptr_f++ );
				case 2:		*dstPtr16++ = (UINT16)( *src_ptr_f++ );
				case 1:		*dstPtr16++ = (UINT16)( *src_ptr_f++ );
				}
			}
		}
		break;

	case NwAttributeType::Half2 :
	case NwAttributeType::Half4 :
		{
			mxASSERT(!input_is_normalized);
			Half* dstPtr16 = (Half*)dst_ptr;
			switch(minCommonDim)
			{
			default:	*dstPtr16++ = Float_To_Half( *src_ptr_f++ );
			case 3:		*dstPtr16++ = Float_To_Half( *src_ptr_f++ );
			case 2:		*dstPtr16++ = Float_To_Half( *src_ptr_f++ );
			case 1:		*dstPtr16++ = Float_To_Half( *src_ptr_f++ );
			}
		}
		break;
	case NwAttributeType::Float1 :
	case NwAttributeType::Float2 :
	case NwAttributeType::Float3 :
	case NwAttributeType::Float4 :
		{
			mxASSERT(!input_is_normalized);
			float* dst_ptr_f = (float*)dst_ptr;
			switch(minCommonDim)
			{
			default:	*dst_ptr_f++ = *src_ptr_f++;
			case 3:		*dst_ptr_f++ = *src_ptr_f++;
			case 2:		*dst_ptr_f++ = *src_ptr_f++;
			case 1:		*dst_ptr_f++ = *src_ptr_f++;
			}
		}
		break;

	case NwAttributeType::R10G10B10A2_UNORM:
		{
			mxASSERT(input_is_normalized);
			mxASSERT(minCommonDim == 3);

			const float x = *src_ptr_f++;
			const float y = *src_ptr_f++;
			const float z = *src_ptr_f++;
			mxASSERT(x >= 0 && x <= 1);
			mxASSERT(y >= 0 && y <= 1);
			mxASSERT(z >= 0 && z <= 1);

			//
			R10G10B1A2& dst_vec = *(R10G10B1A2*) dst_ptr;
			dst_vec.v = 0;

			dst_vec.x = x * 1023.0f;
			dst_vec.y = y * 1023.0f;
			dst_vec.z = z * 1023.0f;
			//dst_vec.w = w * 3.0f;
		}
		break;

	mxNO_SWITCH_DEFAULT;
	}
}

static void PackVertexAttribI( const int* src_ptr, UINT src_dimension,
					   void *dst_ptr, NwAttributeType8 dst_type )
{
	const UINT _dstDimension = NwAttributeType::GetDimension(dst_type);
	const UINT minCommonDim = smallest(src_dimension, _dstDimension);
	//const UINT leftOver = _dstDimension - minCommonDim;
	switch( dst_type )
	{
	case NwAttributeType::Byte4 :
	case NwAttributeType::UByte4 :
		{
			U8* dstPtr8 = (U8*)dst_ptr;
			switch(minCommonDim)
			{
			default:	*dstPtr8++ = (U8)( *src_ptr++ );
			case 3:		*dstPtr8++ = (U8)( *src_ptr++ );
			case 2:		*dstPtr8++ = (U8)( *src_ptr++ );
			case 1:		*dstPtr8++ = (U8)( *src_ptr++ );
			}
		}
		break;

	case NwAttributeType::Short2 :
		UNDONE;
	case NwAttributeType::UShort2 :
		UNDONE;
	case NwAttributeType::Half2 :
		UNDONE;
	case NwAttributeType::Float1 :
	case NwAttributeType::Float2 :
	case NwAttributeType::Float3 :
	case NwAttributeType::Float4 :
		{
			float* dst_ptr_f = (float*)dst_ptr;
			switch(minCommonDim)
			{
			default:	*dst_ptr_f++ = (float)( *src_ptr++ );
			case 3:		*dst_ptr_f++ = (float)( *src_ptr++ );
			case 2:		*dst_ptr_f++ = (float)( *src_ptr++ );
			case 1:		*dst_ptr_f++ = (float)( *src_ptr++ );
			}
		}
		break;
	mxNO_SWITCH_DEFAULT;
	}
}

// Merges data into a single vertex and index buffer referenced by primitive groups (submeshes).
//
ERet CompileMesh(
				 const TcModel& src
				 , const TbVertexFormat& vertex_format
				 , TbRawMeshData &dst
				 )
{
	NwVertexDescription	vertex_description;
	(*vertex_format.build_vertex_description_fun)( &vertex_description );

	//
	UINT totalVertexCount = 0;
	UINT totalIndexCount = 0;
	CalculateTotalVertexIndexCount( src, totalVertexCount, totalIndexCount );

	const bool use32indices = (totalVertexCount >= MAX_UINT16);
	const UINT indexStride = use32indices ? 4 : 2;

	VertexStream	streamStorage[8];

	TArray< VertexStream >	streams;
	streams.initializeWithExternalStorageAndCount( streamStorage, mxCOUNT_OF(streamStorage) );

	GatherStreams(vertex_description, streams);

	const UINT numStreams = streams.num();

	// reserve space in vertex data
	RawVertexData& dstVD = dst.vertexData;
	dstVD.streams.setNum(numStreams);
	dstVD.count = totalVertexCount;
	dstVD.vertex_format_id = vertex_format.GetTypeGUID();

	for( UINT streamIndex = 0; streamIndex < numStreams; streamIndex++ )
	{
		const U32 stride = vertex_description.streamStrides[ streamIndex ];	// stride of the vertex buffer
		RawVertexStream &dstVB = dstVD.streams[ streamIndex ];
		mxDO(dstVB.data.setNum( stride * totalVertexCount ));
	}

	// reserve space in index data
	RawIndexData& dstID = dst.indexData;
	mxDO(dstID.data.setNum(indexStride * totalIndexCount));
	dstID.stride = indexStride;

	dst.topology = NwTopology::TriangleList;
	dst.bounds = src.bounds;

	// iterate over all submeshes (primitive groups) and merge vertex/index data
	const UINT numMeshes = src.meshes.num();
	mxDO(dst.parts.setNum(numMeshes));

	U32  currentVertexIndex = 0;
	U32  currentIndexNumber = 0;
	for( UINT meshIndex = 0; meshIndex < numMeshes; meshIndex++ )
	{
		const TcTriMesh& submesh = *src.meshes[ meshIndex ];
		const U32 numVertices = submesh.NumVertices();	// number of vertices in this submesh
		const U32 numIndices = submesh.NumIndices();	// number of indices in this submesh

		RawMeshPart &meshPart = dst.parts[ meshIndex ];
		meshPart.base_vertex = currentVertexIndex;
		meshPart.start_index = currentIndexNumber;
		meshPart.index_count = numIndices;
		meshPart.vertex_count = numVertices;

		const int maxBoneInfluences = numVertices * MAX_INFLUENCES;

		//StackAllocator	stackAlloc(gCore.frameAlloc);
		//int *	boneIndices = stackAlloc.AllocMany< int >( maxBoneInfluences );
		//float *	boneWeights = stackAlloc.AllocMany< float >( maxBoneInfluences );
		TRY_ALLOCATE_SCRACH( int*, boneIndices, maxBoneInfluences );
		TRY_ALLOCATE_SCRACH( float*, boneWeights, maxBoneInfluences );

		memset(boneIndices, 0, maxBoneInfluences * sizeof(boneIndices[0]));
		memset(boneWeights, 0, maxBoneInfluences * sizeof(boneWeights[0]));

		if( submesh.weights.nonEmpty() )
		{
			mxASSERT(submesh.weights.num() == numVertices);
			for( U32 vertexIndex = 0; vertexIndex < numVertices; vertexIndex++ )
			{
				const TcWeights& weights = submesh.weights[ vertexIndex ];
				const int numWeights = smallest(weights.num(), MAX_INFLUENCES);
				for( int weightIndex = 0; weightIndex < numWeights; weightIndex++ )
				{
					const int offset = vertexIndex * MAX_INFLUENCES + weightIndex;
					boneIndices[ offset ] = weights[ weightIndex ].boneIndex;
					boneWeights[ offset ] = weights[ weightIndex ].boneWeight;
				}
			}//for each vertex
		}

		// process each vertex stream
		for( UINT streamIndex = 0; streamIndex < numStreams; streamIndex++ )
		{
			const U32 streamStride = vertex_description.streamStrides[ streamIndex ];	// stride of the vertex buffer

			const VertexStream& stream = streams[ streamIndex ];				

			//
			RawVertexStream &dstVB = dstVD.streams[ streamIndex ];
			mxDO(dstVB.data.setNum( streamStride * totalVertexCount ));
			dstVB.stride = streamStride;

			const U32 currentVertexDataOffset = currentVertexIndex * streamStride;

			// first fill the vertex data with zeros
			void* dstStreamData = mxAddByteOffset( dstVB.data.raw(), currentVertexDataOffset );
			memset(dstStreamData, 0, streamStride * numVertices);

			// process each vertex stream component
			for( UINT elementIndex = 0; elementIndex < stream.components.num(); elementIndex++ )
			{
				const U32 attribIndex = stream.components[ elementIndex ];
				const NwVertexElement& attrib = vertex_description.attribsArray[ attribIndex ];
				const U32 attribOffset = vertex_description.attribOffsets[ attribIndex ];	// offset within stream
				const NwAttributeType::Enum attribType = (NwAttributeType::Enum) attrib.dataType;
				const NwAttributeUsage::Enum semantic = (NwAttributeUsage::Enum) attrib.usageType;				

				// process each vertex component
				const void* srcPtr = NULL;
				UINT srcDimension = 0;	// vector dimension: [1,2,3,4]
				bool srcDataIsFloat = true;
				bool srcIsPositive = false;
				bool normalize = attrib.normalized;
				int srcComponentStride = 4; // sizeof(float) == sizeof(int)

				switch( semantic )
				{
				case NwAttributeUsage::Position :
					srcPtr = (float*) submesh.positions.getFirstItemPtrOrNil();
					srcComponentStride = sizeof(submesh.positions[0][0]);
					srcDimension = 3;					
					break;

				case NwAttributeUsage::Color :
					mxASSERT2(attrib.usageIndex == 0, "Vertex colors > 1 are not supported!\n");
					srcPtr = (float*) submesh.colors.getFirstItemPtrOrNil();
					srcComponentStride = sizeof(submesh.colors[0][0]);
					srcDimension = 4;
					srcIsPositive = true;	// RGBA colors are in range [0..1]					
					break;

				case NwAttributeUsage::Normal :
					srcPtr = (float*) submesh.normals.getFirstItemPtrOrNil();
					srcComponentStride = sizeof(submesh.normals[0][0]);
					srcDimension = 3;
					normalize = true;	// we pack normals and tangents into Ubyte4
					break;
				case NwAttributeUsage::Tangent :
					srcPtr = (float*) submesh.tangents.getFirstItemPtrOrNil();
					srcComponentStride = sizeof(submesh.tangents[0][0]);
					srcDimension = 3;
					normalize = true;	// we pack normals and tangents into Ubyte4
					break;

				case NwAttributeUsage::TexCoord :
					mxASSERT2(attrib.usageIndex == 0, "TexCoord > 1 are not supported!\n");
					srcPtr = (float*) submesh.texCoords.getFirstItemPtrOrNil();
					srcComponentStride = 4;//sizeof(submesh.texCoords[0][0]);
					srcDimension = 2;
					break;

				case NwAttributeUsage::BoneIndices :
					srcPtr = boneIndices;
					srcComponentStride = sizeof(boneIndices[0]);
					srcDimension = 4;
					srcDataIsFloat = false;
					srcIsPositive = true;
					break;
				case NwAttributeUsage::BoneWeights :
					srcPtr = boneWeights;
					srcComponentStride = sizeof(boneWeights[0]);
					srcDimension = 4;
					srcIsPositive = true;	// bone weights are always in range [0..1]
					break;

				default:
					ptERROR("Unknown vertex attrib semantic: %d\n", semantic);
					return ERR_FAILED_TO_PARSE_DATA;
				}

				// if vertex stream component is found...
				if( srcPtr )
				{
					mxASSERT( srcDimension > 0 );
					// ...pack vertex components as needed
					for( U32 vertexIndex = 0; vertexIndex < numVertices; vertexIndex++ )
					{
						UINT vertexOffset = currentVertexDataOffset + vertexIndex*streamStride + attribOffset;
						void* dstPtr = mxAddByteOffset( dstVB.data.raw(), vertexOffset );
						if( srcDataIsFloat ) {
 							PackVertexAttribF( (float*)srcPtr, srcDimension, dstPtr, attribType, normalize, srcIsPositive );
						} else {
							mxASSERT2( !attrib.normalized, "Integer data cannot be normalized!" );
							PackVertexAttribI( (int*)srcPtr, srcDimension, dstPtr, attribType );
						}
						srcPtr = mxAddByteOffset( srcPtr, srcDimension * srcComponentStride );
					}//for each vertex
				}
			}//for each component
		}//for each stream

		void* indices = mxAddByteOffset(dstID.data.raw(), currentIndexNumber * indexStride);
		for( U32 i = 0; i < numIndices; i++ )
		{
			const U32 index = submesh.indices[i];
			if( use32indices ) {
				U32* indices32 = (U32*) indices;
				indices32[i] = currentVertexIndex + index;
			} else {
				UINT16* indices16 = (UINT16*) indices;
				indices16[i] = currentVertexIndex + index;
			}
		}

		currentVertexIndex += submesh.NumVertices();
		currentIndexNumber += submesh.NumIndices();
	}

	const UINT numBones = src.skeleton.bones.num();

	Skeleton& skeleton = dst.skeleton;

	skeleton.bones.setNum(numBones);
	skeleton.boneNames.setNum(numBones);
	skeleton.invBindPoses.setNum(numBones);

	for( UINT boneIndex = 0; boneIndex < numBones; boneIndex++ )
	{
		const TcBone& bone = src.skeleton.bones[boneIndex];

		Bone& joint = skeleton.bones[boneIndex];

		joint.orientation = bone.rotation;
		joint.position = bone.translation;
		joint.parent = bone.parent;
#if 0
		if( bone.parent >= 0 )
		{
			const Joint& parent = skeleton.joints[bone.parent];
			UNDONE;			
			//ConcatenateJointTransforms(parent.position, parent.orientation, joint.position, joint.orientation);

			//M44f parentMatrix = CalculateJointMatrix( parent.position, parent.orientation );
			//M44f boneMatrix = CalculateJointMatrix( bone.translation, bone.rotation );
			//M44f localMatrix = boneMatrix * glm::inverse(parentMatrix);

			//glm::vec3 localT(localMatrix[3]);
			//glm::quat localQ(localMatrix);
			//joint.orientation = localQ;
			//joint.position = localT;

			//M44f parentMatrix = CalculateJointMatrix( parent.position, parent.orientation );
			//M44f boneTransform = CalculateJointMatrix( bone.translation, bone.rotation );
			//M44f globalMatrix = parentMatrix * boneTransform;
			//joint.orientation = glm::quat(globalMatrix);
			//joint.position = glm::vec3(globalMatrix[3]);
		}

		//joint.orientation = glm::quat(bone.absolute);
		//joint.position = glm::vec3(bone.absolute[3]);

		joint.orientation = glm::normalize(joint.orientation);

		//ptPRINT("Joint[%u]: '%s', parent = %d  (P = %.3f, %.3f, %.3f, Q = %.3f, %.3f, %.3f, %.3f)\n",
		//	boneIndex, bone.name.raw(), bone.parent,
		//	joint.position.iX, joint.position.iY, joint.position.iZ,
		//	joint.orientation.iX, joint.orientation.iY, joint.orientation.iZ, joint.orientation.w);


		skeleton.jointNames[boneIndex] = bone.name;

		M44f jointMatrix = CalculateJointMatrix( joint.position, joint.orientation );

		skeleton.invBindPoses[boneIndex] = glm::inverse(jointMatrix);


		//ptPRINT("Bone[%u]: '%s', absolute = %s,\ncomputed = %s\n",
		//	boneIndex, bone.name.raw(),
		//	MatrixToString(bone.absolute).raw(),
		//	MatrixToString(jointMatrix).raw());


		//ptPRINT("Bone[%u]: '%s', parent = %d (P = %.3f, %.3f, %.3f, Q = %.3f, %.3f, %.3f, %.3f)\n",
		//	boneIndex, bone.name.raw(),
		//	bone.parent, bone.translation.iX, bone.translation.iY, bone.translation.iZ,
		//	bone.rotation.iX, bone.rotation.iY, bone.rotation.iZ, bone.rotation.w);

#endif

	}

	return ALL_OK;
}

}//namespace MeshLib
