#include <algorithm> // std::is_sorted
#include <limits>

#include <bx/uint32_t.h>	// strideAlign
#pragma comment( lib, "bx.lib" )

#include <Base/Base.h>
#include <Base/Build/Language.h>
#include <Base/Math/Geometry/CubeGeometry.h>
#include <Base/Template/Containers/Blob.h>

#include <Core/VectorMath.h>
#include <Core/Util/Tweakable.h>
#include <Core/Serialization/Serialization.h>
#include <Core/Memory.h>
#include <Graphics/Public/graphics_utilities.h>
#include <GPU/Private/Backend/d3d11/dds_reader.h>
#include <GPU/Public/graphics_commands.h>
#include <GPU/Public/render_context.h>
#include <Graphics/Public/graphics_shader_system.h>


/// Corners of the unit cube
/// in Direct3D's or OpenGL's Normalized Device Coordinates (NDC)
const V3f gs_NDC_Cube[8] =
{
	// near plane
	CV3f( -1.0f, -1.0f, NDC_NEAR_CLIP ),	// lower left
	CV3f( -1.0f, +1.0f, NDC_NEAR_CLIP ),	// upper left
	CV3f( +1.0f, +1.0f, NDC_NEAR_CLIP ),	// upper right
	CV3f( +1.0f, -1.0f, NDC_NEAR_CLIP ),	// lower right		

	// far plane
	CV3f( -1.0f, -1.0f, 1.0f ),	// lower left
	CV3f( -1.0f, +1.0f, 1.0f ),	// upper left
	CV3f( +1.0f, +1.0f, 1.0f ),	// upper right
	CV3f( +1.0f, -1.0f, 1.0f ),	// lower right		
};

void TextureImage_m::Init( const void* image_data, const TextureHeader_d& image_header )
{
	const NwDataFormat::Enum textureFormat = (NwDataFormat::Enum) image_header.format;

	this->data		= image_data;
	this->size		= image_header.size;
	this->width		= image_header.width;
	this->height	= image_header.height;
	this->depth		= image_header.depth;

	this->type		= (ETextureType) image_header.type;
	this->format	= textureFormat;
	this->numMips	= image_header.num_mips;
	this->arraySize	= image_header.array_size;
}

/// Get surface information for a particular format
void GetSurfaceInfo(
					const DataFormatT format,
					U32 width, U32 height,
					U32 &_textureSize,	// total size of this mip level, in bytes
					U32 &_rowCount,		// number of rows (horizontal blocks)
					U32 &_rowPitch		// (aligned) size of each row (aka pitch), in bytes
					)
{
	mxASSERT(format != NwDataFormat::Unknown);
	mxASSERT(width > 0);
	mxASSERT(height > 0);

	const DataFormat_t& formatInfo = g_DataFormats[format];

	U32 rowSize = 0;
	U32 rowCount = 0;
	U32 textureSize = 0;

	if( NwDataFormat::IsCompressed(format) )
    {
		// A block-compressed texture must be a multiple of 4 in all dimensions
		// because the block-compression algorithms operate on 4x4 texel blocks.
		// There's memory padding to make the size multiple of 4 on every level.
		const U32 numBlocksWide = largest( 1, (width + 3) / 4 );	// round up to 4
		const U32 numBlocksHigh = largest( 1, (height + 3) / 4 );	// round up to 4

		rowSize = numBlocksWide * formatInfo.blockSize;
		rowCount = numBlocksHigh;
		textureSize = rowSize * numBlocksHigh;
    }
    else
    {
		// uncompressed format
        rowSize = width * formatInfo.blockSize;
        rowCount = height;
        textureSize = rowSize * height;
    }

	_rowPitch = rowSize;
	_rowCount = rowCount;
	_textureSize = textureSize;
}

U32 CalculateTexture2DSize( UINT16 _width, UINT16 _height, DataFormatT _format, UINT8 _numMips )
{
	const UINT8 _depth = 1;

	U32 textureSize = 0;

	U32 width  = largest( _width, 1 );
	U32 height = largest( _height, 1 );
	U32 depth  = largest( _depth, 1 );

	for( int mipIndex = 0; mipIndex < _numMips; mipIndex++ )
	{
		// Determine the size of this mipmap level, in bytes.
		U32 levelSize = 0;
		// Pictures are encoded in blocks (1x1 or 4x4) and this is the height of the block matrix.
		U32 rowCount = 0;
		// And find out the size of a row of blocks, in bytes.
		U32 pitchSize = 0;

		GetSurfaceInfo( _format, width, height, levelSize, rowCount, pitchSize );
		mxUNUSED(rowCount);
		mxUNUSED(pitchSize);

		textureSize += levelSize;

		width  = largest( width/2, 1 );
		height = largest( height/2, 1 );
	}

	return textureSize;
}

ERet ParseMipLevels( const TextureImage_m& image, MipLevel_m *_mips, UINT8 _maxMips )
{
	const UINT	image_depth = (image.type == TEXTURE_CUBEMAP) ? 1 : image.depth;

	mxASSERT(image.width  >= 1);
	mxASSERT(image.height >= 1);
	mxASSERT(image_depth  >= 0);

	const DataFormat_t& formatInfo = g_DataFormats[ image.format ];

	const UINT numSides = (image.type == TEXTURE_CUBEMAP) ? 6 : 1;

	UINT iCurrentMip = 0;

	// current byte offset into the image data
	UINT offset = 0;

	for( UINT sideIndex = 0; sideIndex < numSides; sideIndex++ )
	{
		UINT width  = largest( image.width, 1 );
		UINT height = largest( image.height, 1 );
		UINT depth  = largest( image_depth, 1 );

		for( UINT mipIndex = 0; mipIndex < image.numMips; mipIndex++ )
		{
			if( iCurrentMip > _maxMips ) {
				return ERR_BUFFER_TOO_SMALL;
			}

			// Determine the size of this mipmap level, in bytes.
			U32 levelSize = 0;
			// Pictures are encoded in blocks (1x1 or 4x4) and this is the height of the block matrix.
			U32 rowCount = 0;
			// And find out the size of a row of blocks, in bytes.
			U32 rowPitch = 0;

			GetSurfaceInfo( image.format, width, height, levelSize, rowCount, rowPitch );
			mxUNUSED(rowCount);

			levelSize *= depth;

			MipLevel_m& mip = _mips[ iCurrentMip++ ];

			mip.data		= mxAddByteOffset( image.data, offset );
			mip.size		= levelSize;
			mip.pitch		= rowPitch;
			mip.width		= width;
			mip.height		= height;

			offset += levelSize;

			mxASSERT(offset <= image.size);

			width  /= 2;
			height /= 2;
			depth  /= 2;
			width  = largest( width, 1 );
			height = largest( height, 1 );
			depth  = largest( depth, 1 );
			// Note: for mipmap levels that are smaller than 4x4,
			// only the first four texels will be used for a 2x2 map,
			// and only the first texel will be used by a 1x1 block.
		}
	}
	return ALL_OK;
}


mxBEGIN_REFLECTION( AuxVertex )
	mxMEMBER_FIELD( xyz ),
	mxMEMBER_FIELD( uv ),
	mxMEMBER_FIELD( uv2 ),
	mxMEMBER_FIELD( rgba ),
	mxMEMBER_FIELD( N ),
	mxMEMBER_FIELD( T ),
mxEND_REFLECTION
void AuxVertex::buildVertexDescription( NwVertexDescription *description_ )
{
	description_->begin();
	description_->add(NwAttributeType::Float3, NwAttributeUsage::Position, 0);
	description_->add(NwAttributeType::Half2,  NwAttributeUsage::TexCoord, 0);
	description_->add(NwAttributeType::Half2,  NwAttributeUsage::TexCoord, 1);
	description_->add(NwAttributeType::UByte4, NwAttributeUsage::Color,    0, true);
	description_->add(NwAttributeType::UByte4, NwAttributeUsage::Normal,   0);
	description_->add(NwAttributeType::UByte4, NwAttributeUsage::Tangent,  0);
	description_->end();
}

void ALineRenderer::DrawWireQuad3D(
	const AuxVertex& a, const AuxVertex& b, const AuxVertex& c, const AuxVertex& d
)
{
	this->DrawLine3D( a, b );
	this->DrawLine3D( b, c );
	this->DrawLine3D( c, d );
	this->DrawLine3D( d, a );
}

void ALineRenderer::DrawLine(
	const V3f& start,
	const V3f& end,
	const RGBAf& startColor,
	const RGBAf& endColor
)
{
	AuxVertex	startVertex;
	AuxVertex	endVertex;

	startVertex.xyz = start;
	startVertex.rgba.v = startColor.ToRGBAi().u;
	endVertex.xyz = end;
	endVertex.rgba.v = endColor.ToRGBAi().u;

	this->DrawLine3D( startVertex, endVertex );
}

void ALineRenderer::DrawWireTriangle(
		const V3f& a, const V3f& b, const V3f& c,
		const RGBAf& color
	)
{
	this->DrawLine( a, b, color, color );
	this->DrawLine( b, c, color, color );
	this->DrawLine( c, a, color, color );
}

void ALineRenderer::DrawAABB(
						 const V3f& aabbMin, const V3f& aabbMax,
						 const RGBAf& color
						 )
{
	V3f corners[8];
	AABBf::getCorners(aabbMin, aabbMax, corners);

	for( U32 iEdge = 0; iEdge < 12; iEdge++ )
	{
		ECubeCorner iCornerA, iCornerB;
		Cube_Endpoints_from_Edge( (ECubeEdge) iEdge, iCornerA, iCornerB );

		AuxVertex	start;
		AuxVertex	end;

		start.xyz = corners[ iCornerA ];
		end.xyz = corners[ iCornerB ];

		start.rgba.v = color.ToRGBAi().u;
		end.rgba.v = color.ToRGBAi().u;

		this->DrawLine3D( start, end );
	}
}

void ALineRenderer::DrawCircle(
							 const V3f& origin,
							 const V3f& right,
							 const V3f& up,
							 const RGBAf& color,
							 const float radius,
							 const int numSides
							 )
{
	float	angleDelta = mxTWO_PI / (float)numSides;
	V3f	prevVertex = origin + right * radius;

	for( int iSide = 0; iSide < numSides; iSide++ )
	{
		float	fSin, fCos;
		mmSinCos( angleDelta * (iSide + 1), fSin, fCos );

		V3f	currVertex = origin + (right * fCos + up * fSin) * radius;

		DrawLine( prevVertex, currVertex, color, color );

		prevVertex = currVertex;
	}
}

void ALineRenderer::DrawSternchen(
				   const V3f& origin,
				   const V3f& right,
				   const V3f& up,
				   const RGBAf& color,
				   const float radius,
				   const int numSides	// 1 = horizontal line, 2 - cross, 3 - asterisk.
				   )
{
	float	angleDelta = mxPI / (float)numSides;
	V3f	prevVertex = origin + right * radius;

	for( int iSide = 0; iSide < numSides; iSide++ )
	{
		float	fSin, fCos;
		mmSinCos( angleDelta * (iSide + 1), fSin, fCos );

		const V3f r = (right * fCos + up * fSin) * radius;
		DrawLine( origin - r, origin + r, color, color );
	}
}

void ALineRenderer::DrawWireFrustum(
					 const M44f& _viewProjection,
					 const RGBAf& _color
					 )
{
	const M44f inverseViewProjection = M44_Inverse(_viewProjection);

	// get the world-space positions of the view frustum corners
	AuxVertex frustumCorners[8];
	for( int i = 0; i < 8; i++ )
	{
		const V4f pointH = V4f::set( gs_NDC_Cube[i], 1.0f );
		const V4f point = M44_Transform( inverseViewProjection, pointH );
		const float invW = 1.0f / point.w;
		frustumCorners[i].xyz.x = point.x * invW;
		frustumCorners[i].xyz.y = point.y * invW;
		frustumCorners[i].xyz.z = point.z * invW;

		frustumCorners[i].rgba.v = _color.ToRGBAi().u;
	}

	// near plane
	this->DrawLine3D( frustumCorners[0], frustumCorners[1] );
	this->DrawLine3D( frustumCorners[1], frustumCorners[2] );
	this->DrawLine3D( frustumCorners[2], frustumCorners[3] );
	this->DrawLine3D( frustumCorners[3], frustumCorners[0] );
	// far plane
	this->DrawLine3D( frustumCorners[4], frustumCorners[5] );
	this->DrawLine3D( frustumCorners[5], frustumCorners[6] );
	this->DrawLine3D( frustumCorners[6], frustumCorners[7] );
	this->DrawLine3D( frustumCorners[7], frustumCorners[4] );
	// middle edges
	this->DrawLine3D( frustumCorners[0], frustumCorners[4] );
	this->DrawLine3D( frustumCorners[1], frustumCorners[5] );
	this->DrawLine3D( frustumCorners[2], frustumCorners[6] );
	this->DrawLine3D( frustumCorners[3], frustumCorners[7] );
}

TbPrimitiveBatcher::TbPrimitiveBatcher()
{
}

TbPrimitiveBatcher::~TbPrimitiveBatcher()
{
}

ERet TbPrimitiveBatcher::Initialize( HInputLayout _vertexFormat )
{
	_input_layout = _vertexFormat;
	_render_state.setDefaults();
	m_viewId = 0;	
	m_topology = NwTopology::Undefined;
//	TbShader* defaultShader;
//	mxDO(GetByName(ClumpList::g_loadedClumps, "debug_lines", defaultShader));

	NwRenderState * state_NoCull = nil;
	mxDO(Resources::Load(state_NoCull, MakeAssetID("nocull")));
	_render_state = *state_NoCull;

	return ALL_OK;
}

void TbPrimitiveBatcher::Shutdown()
{
	m_batchedVertices.RemoveAll();
	m_batchedIndices.RemoveAll();
	m_topology = NwTopology::Undefined;
	m_technique = NULL;
}

ERet TbPrimitiveBatcher::reserveSpace( UINT num_vertices, UINT num_indices, UINT num_batches )
{
	mxDO(m_batchedVertices.Reserve(num_vertices));
	mxDO(m_batchedIndices.Reserve(num_indices));
	mxDO(m_batches.Reserve(num_batches));
	return ALL_OK;
}

void TbPrimitiveBatcher::setTechnique( NwShaderEffect* technique )
{
	mxASSERT_PTR(technique);
	m_technique = technique;
}

void TbPrimitiveBatcher::SetViewID( const NGpu::LayerID viewId )
{
	m_viewId = viewId;
}

void TbPrimitiveBatcher::SetStates( const NwRenderState32& states )
{
	_render_state = states;
}

void TbPrimitiveBatcher::PushState()
{
	SavedState stateToSave;
	stateToSave.technique = m_technique;
	stateToSave.states = _render_state;
	stateToSave.topology = m_topology;
	m_stateStack.add( stateToSave );
}

void TbPrimitiveBatcher::PopState()
{
	SavedState stateToRestore = m_stateStack.Pop();
	m_technique = stateToRestore.technique;
	_render_state = stateToRestore.states;
	m_topology = stateToRestore.topology;
}

void TbPrimitiveBatcher::DrawLine3D(
							 const AuxVertex& start,
							 const AuxVertex& end
							 )
{
	AuxVertex *	vertices;
	AuxIndex *	indices;
	if(mxSUCCEDED(BeginBatch( NwTopology::LineList, 2, vertices, 2, indices )))
	{
		vertices[0] = start;
		vertices[1] = end;
		indices[0] = 0;
		indices[1] = 1;
	}
}

void TbPrimitiveBatcher::DrawWireQuad3D(
	const AuxVertex& a, const AuxVertex& b, const AuxVertex& c, const AuxVertex& d
)
{
	AuxVertex *	vertices;
	AuxIndex *	indices;
	if(mxSUCCEDED(BeginBatch( NwTopology::LineList, 4, vertices, 8, indices )))
	{
		vertices[0] = a;
		vertices[1] = b;
		vertices[2] = c;
		vertices[3] = d;
		indices[0] = 0;
		indices[1] = 1;
		indices[2] = 1;
		indices[3] = 2;
		indices[4] = 2;
		indices[5] = 3;
		indices[6] = 3;
		indices[7] = 0;
	}
}

void TbPrimitiveBatcher::DrawDashedLine(
	const V3f& start,
	const V3f& end,
	const float dashSize,
	const RGBAf& startColor,
	const RGBAf& endColor
)
{
	mxSTOLEN("Unreal Engine 3");
	V3f lineDir = end - start;
	float lineLeft = V3_Length(end - start);
	lineDir /= lineLeft;

	while( lineLeft > 0.f )
	{
		V3f lineStart = end - ( lineDir * lineLeft );
		V3f lineEnd = lineStart + ( lineDir * minf(dashSize, lineLeft) );

		DrawLine( lineStart, lineEnd, startColor, endColor );

		lineLeft -= 2*dashSize;
	}
}

void TbPrimitiveBatcher::DrawAxes( float l )
{
#if 0
	// Draw axes.
	DrawLine(
		0.0f,0.0f,0.0f,
		l*1.0f,0.0f,0.0f,
		RGBAf::WHITE,
		RGBAf::RED
	);
	DrawLine(
		0.0f,0.0f,0.0f,
		0.0f,l*1.0f,0.0f,
		RGBAf::WHITE,
		RGBAf::GREEN
	);
	DrawLine(
		0.0f,0.0f,0.0f,
		0.0f,0.0f,l*1.0f,
		RGBAf::WHITE,
		RGBAf::BLUE
	);
#elif 0
	// Draw the x-axis in red
	DrawLine(
		V3_Set(0.0f,0.0f,0.0f),
		V3_Set(l*1.0f,0.0f,0.0f),
		RGBAf::RED,
		RGBAf::RED
	);
	// Draw the y-axis in green
	DrawLine(
		V3_Set(0.0f,0.0f,0.0f),
		V3_Set(0.0f,l*1.0f,0.0f),
		RGBAf::GREEN,
		RGBAf::GREEN
	);
	// Draw the z-axis in blue
	DrawLine(
		V3_Set(0.0f,0.0f,0.0f),
		V3_Set(0.0f,0.0f,l*1.0f),
		RGBAf::BLUE,
		RGBAf::BLUE
	);
#else
	DrawArrow(
		M44_FromAxes(
			V3_RIGHT,
			V3_FORWARD,
			V3_UP
		),
		RGBAf::RED, l*1.0f, l*0.1f
	);
	DrawArrow(
		M44_FromAxes(
			V3_FORWARD,
			V3_UP,
			V3_RIGHT
		),
		RGBAf::GREEN, l*1.0f, l*0.1f
	);
	DrawArrow(
		M44_FromAxes(
			V3_UP,
			V3_RIGHT,
			V3_FORWARD
		),
		RGBAf::BLUE, l*1.0f, l*0.1f
	);
#endif
}

void TbPrimitiveBatcher::DrawArrow(
							const M44f& arrowTransform,
							const RGBAf& color,
							float arrowLength,
							float headSize
							)
{
	mxSTOLEN("Unreal Engine 3");
	const V3f origin = M44_getTranslation(arrowTransform);
	const V3f endPoint = M44_TransformPoint(arrowTransform, V3_Set( arrowLength, 0.0f, 0.0f ));
	const float headScale = 0.5f;

	const RGBAf& startColor = RGBAf::WHITE;
	const RGBAf& endColor = color;

	DrawLine( origin, endPoint, startColor, endColor );
	DrawLine( endPoint, M44_TransformPoint(arrowTransform, V3_Set( arrowLength-headSize, +headSize*headScale, +headSize*headScale )), endColor, endColor );
	DrawLine( endPoint, M44_TransformPoint(arrowTransform, V3_Set( arrowLength-headSize, +headSize*headScale, -headSize*headScale )), endColor, endColor );
	DrawLine( endPoint, M44_TransformPoint(arrowTransform, V3_Set( arrowLength-headSize, -headSize*headScale, +headSize*headScale )), endColor, endColor );
	DrawLine( endPoint, M44_TransformPoint(arrowTransform, V3_Set( arrowLength-headSize, -headSize*headScale, -headSize*headScale )), endColor, endColor );
}

// based on DXSDK, June 2010, XNA collision detection sample
void TbPrimitiveBatcher::DrawGrid(
						   const V3f& origin,
						   const V3f& axisX,
						   const V3f& axisY,
						   int numXDivisions,
						   int numYDivisions,
						   const RGBAf& color
						   )
{
	numXDivisions = Max( 1, numXDivisions );
	numYDivisions = Max( 1, numYDivisions );

	float invXDivisions2 = 2.0f / numXDivisions;
	float invYDivisions2 = 2.0f / numYDivisions;

	for( int i = 0; i <= numXDivisions; i++ )
	{
		float fPercent = (i * invXDivisions2) - 1.0f;	// [-1..+1]
		V3f offsetX = origin + axisX * fPercent;

		V3f pointA = offsetX - axisY;
		V3f pointB = offsetX + axisY;

		DrawLine( pointA, pointB, color, color );
	}

	for( int i = 0; i <= numYDivisions; i++ )
	{
		float fPercent = (i * invYDivisions2) - 1.0f;	// [-1..+1]
		V3f offsetY = origin + axisY * fPercent;

		V3f pointA = offsetY - axisX;
		V3f pointB = offsetY + axisX;

		DrawLine( pointA, pointB, color, color );
	}
}

void TbPrimitiveBatcher::DrawSolidBox(
	const V3f& origin, const V3f& halfSize,
	const RGBAf& color
)
{
	enum { NUM_VERTICES = 8 };
	enum { NUM_TRIANGLES = 12 };
	enum { NUM_INDICES = NUM_TRIANGLES * 3 };

	V3f corners[ NUM_VERTICES ];
	for( int i = 0; i < NUM_VERTICES; i++ )
	{
		corners[i].x = (i & 1) ? halfSize.x : -halfSize.x;
		corners[i].y = (i & 2) ? halfSize.y : -halfSize.y;
		corners[i].z = (i & 4) ? halfSize.z : -halfSize.z;
		corners[i] += origin;
	}

	static const V3f vertexNormals[ NUM_VERTICES ] = 
	{
		CV3f( -mxINV_SQRT_3, -mxINV_SQRT_3, -mxINV_SQRT_3	),
		CV3f(  mxINV_SQRT_3, -mxINV_SQRT_3, -mxINV_SQRT_3	),
		CV3f( -mxINV_SQRT_3,  mxINV_SQRT_3, -mxINV_SQRT_3	),
		CV3f(  mxINV_SQRT_3,  mxINV_SQRT_3, -mxINV_SQRT_3	),

		CV3f( -mxINV_SQRT_3, -mxINV_SQRT_3,  mxINV_SQRT_3	),
		CV3f(  mxINV_SQRT_3, -mxINV_SQRT_3,  mxINV_SQRT_3	),
		CV3f( -mxINV_SQRT_3,  mxINV_SQRT_3,  mxINV_SQRT_3	),
		CV3f(  mxINV_SQRT_3,  mxINV_SQRT_3,  mxINV_SQRT_3	),
	};

	static const UINT8 triangleIndices[ NUM_INDICES ] =
	{
		// front
		0, 1, 5,
		0, 5, 4,
		// back
		2, 6, 7,
		2, 7, 3,

		// top
		4, 5, 7,
		4, 7, 6,
		// bottom
		0, 2, 3,
		0, 3, 1,

		// left
		0, 4, 6,
		0, 6, 2,
		// right
		1, 3, 7,
		1, 7, 5,
	};
#if 0
	// if you're standing in the center of a box
	// the planes' normals are directed outwards...
	enum EBoxPlaneSide
	{
		SIDE_IN_FRONT	= 0,
		SIDE_BEHIND		= 1,

		SIDE_ABOVE		= 2,
		SIDE_BENEATH	= 3,

		SIDE_LEFT		= 4,
		SIDE_RIGHT		= 5,

		NUM_SIDES		= 6
	};

	const V3f planeNormals[6] = {
		{  0.0f, -1.0f,  0.0f },	// front
		{  0.0f, +1.0f,  0.0f },	// back
		{  0.0f,  0.0f, +1.0f },	// top
		{  0.0f,  0.0f, -1.0f },	// bottom
		{ -1.0f, -1.0f,  0.0f },	// left
		{ +1.0f, -1.0f,  0.0f },	// right
	};
#endif

	AuxVertex *	vertices;
	AuxIndex *	indices;
	BeginBatch( NwTopology::TriangleList, NUM_VERTICES, vertices, NUM_INDICES, indices );

	for( int i = 0; i < NUM_VERTICES; i++ )
	{
		AuxVertex& v = vertices[i];
		v.xyz = corners[i];
		v.N = PackNormal( vertexNormals[i] );
		v.rgba.v = color.ToRGBAi().u;
	}

	for( int i = 0; i < NUM_INDICES; i++ )
	{
		indices[i] = triangleIndices[i];
	}
}

//void DrawAABB( const rxAABB& box, const RGBAf& color )
//{
//	XMMATRIX matWorld = XMMatrixScaling( box.Extents.x, box.Extents.y, box.Extents.z );
//	XMVECTOR position = XMLoadFloat3( &box.center );
//	matWorld.r[3] = XMVectorSelect( matWorld.r[3], position, XMVectorSelectControl( 1, 1, 1, 0 ) );
//
//	DrawCube( matWorld, color );
//}
//
//void DrawOBB( const rxOOBB& box, const RGBAf& color )
//{
//	XMMATRIX matWorld = XMMatrixRotationQuaternion( XMLoadFloat4( &box.Orientation ) );
//	XMMATRIX matScale = XMMatrixScaling( box.Extents.x, box.Extents.y, box.Extents.z );
//	matWorld = XMMatrixMultiply( matScale, matWorld );
//	XMVECTOR position = XMLoadFloat3( &box.center );
//	matWorld.r[3] = XMVectorSelect( matWorld.r[3], position, XMVectorSelectControl( 1, 1, 1, 0 ) );
//
//	DrawCube( matWorld, color );
//}

void TbPrimitiveBatcher::DrawSolidTriangle3D(
	const AuxVertex& a, const AuxVertex& b, const AuxVertex& c
	)
{
	AuxVertex *	vertices;
	AuxIndex *	indices;
	if(mxSUCCEDED(BeginBatch( NwTopology::TriangleList, 3, vertices, 3, indices )))
	{
		vertices[0] = a;
		vertices[1] = b;
		vertices[2] = c;

		indices[0] = 0;
		indices[1] = 1;
		indices[2] = 2;
	}
}

void TbPrimitiveBatcher::DrawSolidQuad3D(
	const AuxVertex& a, const AuxVertex& b, const AuxVertex& c, const AuxVertex& d
)
{
	AuxVertex *	vertices;
	AuxIndex *	indices;
	if(mxSUCCEDED(BeginBatch( NwTopology::TriangleList, 4, vertices, 6, indices )))
	{
		vertices[0] = a;
		vertices[1] = b;
		vertices[2] = c;
		vertices[3] = d;

		indices[0] = 0;
		indices[1] = 1;
		indices[2] = 2;
		indices[3] = 0;
		indices[4] = 2;
		indices[5] = 3;
	}
}

void TbPrimitiveBatcher::DrawSolidCircle(
									   const V3f& origin,
									   const V3f& right,
									   const V3f& up,
									   const RGBAf& color,
									   float radius,
									   int numSides
									   )
{
	mxASSERT( mmAbs(radius) > 1e-4f );
	mxASSERT( numSides > 2 );

	const RGBAi rgba32 = color.ToRGBAi();

	//
	const int numVertices = numSides;
	const int numTriangles = numSides - 2;	
	const int numIndices = numTriangles * 3;

	//
	AuxVertex *	vertices;
	AuxIndex *	indices;
	if(mxSUCCEDED(BeginBatch( NwTopology::TriangleList, numVertices, vertices, numIndices, indices )))
	{
		const float angleDelta = mxTWO_PI / (float)numSides;
		for( int iPoint = 0; iPoint < numVertices; iPoint++ )
		{
			float	fSin, fCos;
			mmSinCos( angleDelta * (iPoint + 1), fSin, fCos );

			AuxVertex & vertex = vertices[ iPoint ];
			vertex.xyz = origin + (right * fCos + up * fSin) * radius;
			vertex.rgba.v = rgba32.u;
		}
		for( int iTri = 0; iTri < numTriangles; iTri++ )
		{
			indices[ iTri*3 + 0 ] = 0;
			indices[ iTri*3 + 1 ] = iTri + 1;
			indices[ iTri*3 + 2 ] = iTri + 2;
		}
	}
}

/// Creates a UVSphere in Blender terms.
/// NOTE: Z is up (X - right, Y - forward).
/// NOTE: the density of vertices is greater around the poles, and UV coords are distorted.
/// see http://paulbourke.net/geometry/circlesphere/
/// http://vterrain.org/Textures/spherical.html
///
void TbPrimitiveBatcher::DrawSolidSphere(
	const V3f& center, float radius,
	const RGBAf& color,
	int numStacks,	// horizontal layers
	int numSlices	// radial divisions
)
{
	UByte4 rgbaColor;
	rgbaColor.v = color.ToRGBAi().u;

	// This approach is much like a globe in that there is a certain number of vertical lines of latitude
	// and horizontal lines of longitude which break the sphere up into many rectangular (4 sided) parts.
	// Note however there will be a point at the top and bottom (north and south pole)
	// and polygons attached to these will be triangular (3 sided).
	// These are easiest to generate
	// and the number of polygons will be equal to: LONGITUDE_LINES * (LATITUDE_LINES + 1).
	// Of these (LATITUDE_LINES*2) will be triangular and connected to a pole, and the rest rectangular.

	// calculate the total number of vertices and indices
	const int numRings = numStacks-1;	// do not count the poles as rings
	const int numRingVertices = numSlices + 1;	// number of vertices in each horizontal ring
	const int totalVertices = numRings * numRingVertices + 2;	// number of vertices in each ring plus poles
	const int totalIndices = (numSlices * (numStacks-2)) * 6 + (numSlices*3)*2;	// quads in each ring plus triangles in poles

	AuxVertex *	vertices;
	AuxIndex *	indices;
	if(mxFAILED(BeginBatch( NwTopology::TriangleList, totalVertices, vertices, totalIndices, indices )))
		return;

	int	createdVertices = 0;
	int	createdIndices = 0;

	// use polar coordinates to generate slices of the sphere

    //       Z   Phi (Zenith)
    //       |  /
    //       | / /'
    //       |  / '
    //       | /  '
    //       |/___'________Y
    //      / / - .
    //     /   /  '/
    //    /     / ' Theta (Azimuth)
    //   X       /'

	const float phiStep = mxPI / (float) numStacks;	// vertical angle delta

	// Compute vertices for each stack ring.
	for ( int i = 1; i <= numRings; ++i )
	{
		const float phi = i * phiStep;	// polar angle (vertical) or inclination, latitude, [0..PI]

		float sinPhi, cosPhi;
		mmSinCos( phi, sinPhi, cosPhi );

		// vertices of ring
		const float thetaStep = mxTWO_PI / numSlices;	// horizontal angle delta
		for ( int j = 0; j <= numSlices; ++j )
		{
			const float theta = j * thetaStep;	// azimuthal angle (horizontal) or azimuth, longitude, [0..2*PI]

			float sinTheta, cosTheta;			
			mmSinCos( theta, sinTheta, cosTheta );

			AuxVertex & vertex = vertices[ createdVertices++ ];

			// spherical to cartesian
			vertex.xyz.x = radius * sinPhi * cosTheta;
			vertex.xyz.y = radius * sinPhi * sinTheta;
			vertex.xyz.z = radius * cosPhi;
			vertex.xyz += center;
			vertex.rgba = rgbaColor;

			V3f normal = V3_Normalized(vertex.xyz);
			vertex.N = PackNormal(normal.x, normal.y, normal.z);

			V3f tangent;
			// partial derivative of P with respect to theta
			tangent.x = -radius * sinPhi * sinTheta;
			tangent.y = radius * sinPhi * cosTheta;
			tangent.z = 0.0f;
			//vertex.T = PackNormal(tangent.x, tangent.y, tangent.z);

			V2f	uv;
			uv.x = theta * mxINV_TWOPI;	// theta / (2 * PI)
			uv.y = phi * mxINV_PI;	// phi / PI

			vertex.uv = V2_To_Half2( uv );
		}
	}

	// square faces between intermediate points:

	// Compute indices for inner stacks (not connected to poles).
	for ( int i = 0; i < numStacks-2; ++i )	// number of horizontal layers, excluding caps
	{
		for( int j = 0; j < numSlices; ++j )	// number of radial divisions
		{
			// one quad, two triangles (CCW is front)
			indices[ createdIndices++ ] = numRingVertices * i     + j;	// 1
			indices[ createdIndices++ ] = numRingVertices * i     + j+1;	// 2
			indices[ createdIndices++ ] = numRingVertices * (i+1) + j;	// 3

			indices[ createdIndices++ ] = numRingVertices * (i+1) + j;	// 3
			indices[ createdIndices++ ] = numRingVertices * i     + j+1;	// 2
			indices[ createdIndices++ ] = numRingVertices * (i+1) + j+1;	// 4
		}
	}

	const AuxIndex northPoleIndex = createdVertices;
	const AuxIndex southPoleIndex = northPoleIndex + 1;

	// poles: note that there will be texture coordinate distortion
	AuxVertex & northPole = vertices[ createdVertices++ ];
	northPole.xyz = center;
	northPole.xyz.z += radius;
	northPole.uv = V2_To_Half2(V2_Set(0.0f, 1.0f));
	northPole.N = PackNormal( 0.0f, 0.0f, 1.0f );
	//northPole.T = PackNormal( 1.0f, 0.0f, 0.0f );
	northPole.rgba = rgbaColor;

	AuxVertex & southPole = vertices[ createdVertices++ ];
	southPole.xyz = center;
	southPole.xyz.z -= radius;
	southPole.uv = V2_To_Half2(V2_Set(0.0f, 1.0f));
	southPole.N = PackNormal( 0.0f, 0.0f, -1.0f );
	//southPole.T = PackNormal( 1.0f, 0.0f, 0.0f );
	southPole.rgba = rgbaColor;

	// triangle faces connecting to top and bottom vertex:

	// Compute indices for top stack.  The top stack was written 
	// first to the vertex buffer.
	for( int i = 0; i < numSlices; ++i )
	{
		indices[ createdIndices++ ] = northPoleIndex;
		indices[ createdIndices++ ] = i + 1;
		indices[ createdIndices++ ] = i;
	}

	// Compute indices for bottom stack.  The bottom stack was written
	// last to the vertex buffer, so we need to offset to the index
	// of first vertex in the last ring.
	int baseIndex = ( numRings - 1 ) * numRingVertices;
	for( int i = 0; i < numSlices; ++i )
	{
		indices[ createdIndices++ ] = southPoleIndex;
		indices[ createdIndices++ ] = baseIndex + i;
		indices[ createdIndices++ ] = baseIndex + i + 1;
	}

	mxASSERT(totalVertices == createdVertices);
	mxASSERT(totalIndices == createdIndices);
}

void TbPrimitiveBatcher::DrawSolidArrow(
									  const M44f& arrowTransform,
									  const RGBAf& color,
									  float arrowLength,
									  float headSize
									  )
{
	enum { AXIS_ARROW_SEGMENTS = 6 };
mxREFACTOR(:);
	// Pre-compute the vertices for drawing axis arrows.
	static float AXIS_ARROW_RADIUS = 0.09f * 1;
	static float ARROW_BASE_HEIGHT = 1.8f * 1;
	static float ARROW_TOTAL_LENGTH = 2.0f * 1;
	static float ARROW_CONE_HEIGHT = 1.6f * 1;


	V3f	m_axisRoot = V3_Set( 0,0,0 );
	V3f	m_arrowBase = V3_Set( 0,0,ARROW_BASE_HEIGHT );
	V3f	m_arrowHead = V3_Set( 0,0,ARROW_TOTAL_LENGTH );
	V3f	m_segments[ AXIS_ARROW_SEGMENTS + 1 ];

	// Generate the axis arrow cone.

	// Generate the vertices for the base of the cone

	const float angleDelta = mxTWO_PI / (float)AXIS_ARROW_SEGMENTS;
	for( int iSegment = 0; iSegment <= AXIS_ARROW_SEGMENTS ; iSegment++ )
	{
		const float theta = iSegment * angleDelta;	// in radians

		float	s, c;
		mmSinCos( theta, s, c );

		m_segments[ iSegment ] = V3_Set( AXIS_ARROW_RADIUS * s, AXIS_ARROW_RADIUS * c, ARROW_CONE_HEIGHT );
	}



	V3f transformedHead = M44_TransformPoint( arrowTransform, m_arrowHead );

	// draw the 'stem'
	mxTODO("Draw cylinder as triangle list");
	DrawLine( M44_getTranslation(arrowTransform), transformedHead, color );

	// draw the arrow head
	for( int iSegment = 0 ; iSegment < AXIS_ARROW_SEGMENTS ; iSegment++ )
	{
		const V3f p0 = M44_TransformPoint( arrowTransform, m_segments[ iSegment ] );
		const V3f p1 = M44_TransformPoint( arrowTransform, m_segments[ iSegment + 1 ] );

		AuxVertex	a, b, c;
		a.xyz = p0;
		b.xyz = p1;
		c.xyz = transformedHead;

		a.rgba.v = color.ToRGBAi().u;
		b.rgba.v = color.ToRGBAi().u;
		c.rgba.v = color.ToRGBAi().u;

		// Draw the base triangle of the cone.
		// NOTE: no need because we disable backface culling
		//renderer.DrawTriangle3D( a, b, c );

		// Draw the top triangle of the cone.
		DrawSolidTriangle3D( a, b, c );
	}
}

mxUNDONE
#if 0
void TbPrimitiveBatcher::DrawRing( const XMFLOAT3& Origin, const XMFLOAT3& MajorAxis, const XMFLOAT3& MinorAxis, const RGBAf& Color )
{
   static const DWORD dwRingSegments = 32;

    XMFLOAT3 verts[ dwRingSegments + 1 ];

    XMVECTOR vOrigin = XMLoadFloat3( &Origin );
    XMVECTOR vMajor = XMLoadFloat3( &MajorAxis );
    XMVECTOR vMinor = XMLoadFloat3( &MinorAxis );

    float fAngleDelta = XM_2PI / ( float )dwRingSegments;
    // Instead of calling cos/sin for each segment we calculate
    // the sign of the angle delta and then incrementally calculate sin
    // and cosine from then on.
    XMVECTOR cosDelta = XMVectorReplicate( cosf( fAngleDelta ) );
    XMVECTOR sinDelta = XMVectorReplicate( sinf( fAngleDelta ) );
    XMVECTOR incrementalSin = XMVectorZero();
    static const XMVECTOR initialCos =
    {
        1.0f, 1.0f, 1.0f, 1.0f
    };
    XMVECTOR incrementalCos = initialCos;
    for( DWORD i = 0; i < dwRingSegments; i++ )
    {
        XMVECTOR Pos;
        Pos = XMVectorMultiplyAdd( vMajor, incrementalCos, vOrigin );
        Pos = XMVectorMultiplyAdd( vMinor, incrementalSin, Pos );
        XMStoreFloat3( ( XMFLOAT3* )&verts[i], Pos );
        // Standard formula to rotate a vector.
        XMVECTOR newCos = incrementalCos * cosDelta - incrementalSin * sinDelta;
        XMVECTOR newSin = incrementalCos * sinDelta + incrementalSin * cosDelta;
        incrementalCos = newCos;
        incrementalSin = newSin;
    }
    verts[ dwRingSegments ] = verts[0];

    // Copy to vertex buffer
    assert( (dwRingSegments+1) <= MAX_VERTS );

    XMFLOAT3* pVerts = NULL;
    HRESULT hr;
    V( g_pVB->Lock( 0, 0, (void**)&pVerts, D3DLOCK_DISCARD ) )
    memcpy( pVerts, verts, sizeof(verts) );
    V( g_pVB->Unlock() )

    // Draw ring
    D3DXCOLOR clr = Color;
    g_pEffect9->SetFloatArray( g_Color, clr, 4 );
    g_pEffect9->CommitChanges();
    pd3dDevice->DrawPrimitive( D3DPT_LINESTRIP, 0, dwRingSegments );
}
void TbPrimitiveBatcher::DrawSphere( const XNA::Sphere& sphere, const RGBAf& Color )
{
	const XMFLOAT3 Origin = sphere.center;
	const float fRadius = sphere.Radius;

	DrawRing( pd3dDevice, Origin, XMFLOAT3( fRadius, 0, 0 ), XMFLOAT3( 0, 0, fRadius ), Color );
	DrawRing( pd3dDevice, Origin, XMFLOAT3( fRadius, 0, 0 ), XMFLOAT3( 0, fRadius, 0 ), Color );
	DrawRing( pd3dDevice, Origin, XMFLOAT3( 0, fRadius, 0 ), XMFLOAT3( 0, 0, fRadius ), Color );
}
void TbPrimitiveBatcher::DrawRay( const XMFLOAT3& Origin, const XMFLOAT3& Direction, BOOL bNormalize, const RGBAf& Color )
{
    XMFLOAT3 verts[3];
    memcpy( &verts[0], &Origin, 3 * sizeof( float ) );

    XMVECTOR RayOrigin = XMLoadFloat3( &Origin );
    XMVECTOR RayDirection = XMLoadFloat3( &Direction );
    XMVECTOR NormDirection = XMVector3Normalize( RayDirection );
    if( bNormalize )
        RayDirection = NormDirection;

    XMVECTOR PerpVector;
    XMVECTOR CrossVector = XMVectorSet( 0, 1, 0, 0 );
    PerpVector = XMVector3Cross( NormDirection, CrossVector );

    if( XMVector3Equal( XMVector3LengthSq( PerpVector ), XMVectorSet( 0, 0, 0, 0 ) ) )
    {
        CrossVector = XMVectorSet( 0, 0, 1, 0 );
        PerpVector = XMVector3Cross( NormDirection, CrossVector );
    }
    PerpVector = XMVector3Normalize( PerpVector );

    XMStoreFloat3( ( XMFLOAT3* )&verts[1], XMVectorAdd( RayDirection, RayOrigin ) );
    PerpVector = XMVectorScale( PerpVector, 0.0625f );
    NormDirection = XMVectorScale( NormDirection, -0.25f );
    RayDirection = XMVectorAdd( PerpVector, RayDirection );
    RayDirection = XMVectorAdd( NormDirection, RayDirection );
    XMStoreFloat3( ( XMFLOAT3* )&verts[2], XMVectorAdd( RayDirection, RayOrigin ) );
    
    // Copy to vertex buffer
    assert( 3 <= MAX_VERTS );
    XMFLOAT3* pVerts = NULL;
    HRESULT hr;
    V( g_pVB->Lock( 0, 0, (void**)&pVerts, D3DLOCK_DISCARD ) )
    memcpy( pVerts, verts, sizeof(verts) );
    V( g_pVB->Unlock() )

    // Draw ray
    D3DXCOLOR clr = Color;
    g_pEffect9->SetFloatArray( g_Color, clr, 4 );
    g_pEffect9->CommitChanges();
    pd3dDevice->DrawPrimitive( D3DPT_LINESTRIP, 0, 2 );
}
#endif

#if 0
void TbPrimitiveBatcher::DrawSprite(
	const M44f& cameraWorldMatrix,
	const V3f& spriteOrigin,
	const float spriteSizeX, const float spriteSizeY,
	const RGBAf& color
)
{
	const V3f spriteX = cameraWorldMatrix[0].ToVec3() * spriteSizeX;
	const V3f spriteY = cameraWorldMatrix[1].ToVec3() * spriteSizeY;

	const U32 rgbaColor = color.ToRGBA32();

	BeginBatch( Topology::TriangleList, 4, 6 );

	const U32 iBaseVertex = m_batchedVertices.num();

	AuxVertex & v0 = m_batchedVertices.AddNew();
	AuxVertex & v1 = m_batchedVertices.AddNew();
	AuxVertex & v2 = m_batchedVertices.AddNew();
	AuxVertex & v3 = m_batchedVertices.AddNew();

	v0.xyz			= spriteOrigin - spriteX - spriteY;	// bottom left
	v0.uv0.x 		= 0.0f;
	v0.uv0.y 		= 1.0f;
	v0.rgba.v 	= rgbaColor;

	v1.xyz			= spriteOrigin - spriteX + spriteY;	// top left
	v1.uv0.x 		= 0.0f;
	v1.uv0.y 		= 0.0f;
	v1.rgba.v 	= rgbaColor;

	v2.xyz			= spriteOrigin + spriteX + spriteY;	// top right
	v2.uv0.x 		= 1.0f;
	v2.uv0.y 		= 0.0f;
	v2.rgba.v 	= rgbaColor;

	v3.xyz			= spriteOrigin + spriteX - spriteY;	// bottom right
	v3.uv0.x 		= 1.0f;
	v3.uv0.y 		= 1.0f;
	v3.rgba.v 	= rgbaColor;

	// indices:
	// 0,	1,	2,
	// 0,	2,	3,

	m_batchedIndices.add( 0 );
	m_batchedIndices.add( 1 );
	m_batchedIndices.add( 2 );

	m_batchedIndices.add( 0 );
	m_batchedIndices.add( 2 );
	m_batchedIndices.add( 3 );
}
#endif

void TbPrimitiveBatcher::DrawPoint( const AuxVertex& _p )
{
	AuxVertex *	vertices;
	AuxIndex *	indices;
	BeginBatch( NwTopology::PointList, 1, vertices, 0, indices );
	// point lists are rendered as isolated points
	vertices[0] = _p;
}

void TbPrimitiveBatcher::DrawConvexPolygon(
	const int num_vertices
	, AuxVertex *&vertices_	// allocated internally, you must copy data into it
	)
{
	mxASSERT( num_vertices > 2 );
	// draw as a triangle fan
	const int numTriangles = num_vertices - 2;	
	const int numIndices = numTriangles * 3;

	AuxVertex *	vertices;
	AuxIndex *	indices;
	BeginBatch( NwTopology::TriangleList, num_vertices, vertices, numIndices, indices );

	vertices_ = vertices;

	for( int iTri = 0; iTri < numTriangles; iTri++ )
	{
		indices[ iTri*3 + 0 ] = 0;
		indices[ iTri*3 + 1 ] = iTri + 1;
		indices[ iTri*3 + 2 ] = iTri + 2;
	}
}

ERet TbPrimitiveBatcher::RenderBatchedPrimitives(
	NGpu::NwRenderContext & render_context,
	const AuxVertex* allVertices,
	const U32 _totalVertexCount,
	const AuxIndex* allIndices,
	const U32 _totalIndexCount,
	const HProgram program,
	const NwRenderState32& _states,
	const HInputLayout _vtxFmt,
	const NwTopology::Enum _topology,
	const Batch** _batches,
	const UINT _numBatches
	)
{
	NwTransientBuffer	VB;
	NwTransientBuffer	IB;

	NGpu::CommandBufferWriter	cmd_writer( render_context._command_buffer );

//cmd_writer.dbgPrintf(0,GFX_SRCFILE_STRING);
	//
	NGpu::Cmd_Draw	cmd;

	cmd.program = program;

	if( _totalIndexCount )
	{
		mxDO(NGpu::AllocateTransientBuffers(
			VB, _totalVertexCount, sizeof(allVertices[0]),
			IB, _totalIndexCount, sizeof(allIndices[0])
			));
		cmd.VB = VB.buffer_handle;
		cmd.IB = IB.buffer_handle;
	}
	else
	{
		mxDO(NGpu::allocateTransientVertexBuffer(
			VB, _totalVertexCount, sizeof(allVertices[0])
			));
		cmd.VB = VB.buffer_handle;

		IB.data = nil;
		IB.base_index = 0;
		IB.buffer_handle.SetNil();
	}

	AuxVertex *dstVerticesStart = (AuxVertex*) VB.data;
	AuxIndex *dstIndicesStart = (AuxIndex*) IB.data;

	U32	numCopiedVertices = 0;
	U32	numCopiedIndices = 0;

	for( UINT iMergedBatch = 0; iMergedBatch < _numBatches; iMergedBatch++ )
	{
		const Batch& mergedBatch = *_batches[ iMergedBatch ];

		const AuxVertex* srcVertices = allVertices + mergedBatch.startVertex;
		const AuxIndex* srcIndices = allIndices + mergedBatch.start_index;

		memcpy( dstVerticesStart + numCopiedVertices, srcVertices, mergedBatch.vertex_count * sizeof(allVertices[0]) );

		AuxIndex *dstIndices = dstIndicesStart + numCopiedIndices;
		for( UINT iIndex = 0; iIndex < mergedBatch.index_count; iIndex++ ) {
			dstIndices[ iIndex ] = srcIndices[ iIndex ] + numCopiedVertices;
		}

		numCopiedVertices += mergedBatch.vertex_count;
		numCopiedIndices += mergedBatch.index_count;
	}

	//
	cmd_writer.SetRenderState(_states);

	cmd.input_layout = _vtxFmt;
	cmd.primitive_topology = _topology;
	cmd.use_32_bit_indices = sizeof(allIndices[0]) == sizeof(U32);

	cmd.base_vertex = VB.base_index;
	cmd.vertex_count = _totalVertexCount;
	cmd.start_index = IB.base_index;
	cmd.index_count = _totalIndexCount;

	IF_DEBUG sprintf(cmd.dbgname, "TbPrimitiveBatcher");
	IF_DEBUG cmd.src_loc = GFX_SRCFILE_STRING;

	cmd_writer.Draw( cmd );

	return ALL_OK;
}

ERet TbPrimitiveBatcher::Flush()
{
	const UINT num_batches = m_batches.num();

	if( !num_batches )
	{
		return ALL_OK;
	}

	mxASSERT(m_batchedVertices.num() > 0); 

	const AuxVertex* allVertices = m_batchedVertices.raw();
	const AuxIndex* allIndices = m_batchedIndices.raw();

	NGpu::NwRenderContext & render_context = NGpu::getMainRenderContext();

	NGpu::NwPushCommandsOnExitingScope	applyShaderCommands(
		render_context,
		NGpu::buildSortKey( m_viewId, 1 )
		nwDBG_CMD_SRCFILE_STRING
		);

	mxDO(m_technique->pushShaderParameters( render_context._command_buffer ));

	struct BatchSorter
	{
		inline bool operator () ( const Batch* a, const Batch* b ) {
			// sort by render state
			if( a->state.u != b->state.u ) {
				return a->state.u < b->state.u;
			}
			// order by topology
			return a->topo < b->topo;
		}
	};

	const Batch *	fast_storage_for_sorted_batches[4096];  // > 2K batches per frame
	DynamicArray< const Batch* >	sortedBatches( MemoryHeaps::temporary() );
	sortedBatches.initializeWithExternalStorageAndCount( fast_storage_for_sorted_batches, mxCOUNT_OF(fast_storage_for_sorted_batches) );

	// Sort batches by render states, topology, etc.

	mxDO(sortedBatches.setCountExactly( num_batches ));

	for( UINT iBatch = 0; iBatch < num_batches; iBatch++ )
	{
		const Batch& batch = m_batches[ iBatch ];
		sortedBatches[ iBatch ] = &batch;
	}

	std::sort( sortedBatches.begin(), sortedBatches.end(), BatchSorter() );

	// merge identical batches together in one draw call
	U32	accumulatedBatchesStart = 0;
	// these are for knowing how much space to allocate in the transient vertex/index buffer
	U32	numAccumulatedVertices = 0;
	U32	numAccumulatedIndices = 0;

	const HProgram program = m_technique->getDefaultVariant().program_handle;

	NwRenderState32	currentStates = sortedBatches[0]->state;
	NwTopology::Enum	currentTopology = sortedBatches[0]->topo;

	for( UINT iBatch = 0; iBatch < sortedBatches.num(); iBatch++ )
	{
		const Batch& batch = *sortedBatches[ iBatch ];

		if( currentStates.u == batch.state.u && currentTopology == batch.topo )
		{
			numAccumulatedVertices += batch.vertex_count;
			numAccumulatedIndices += batch.index_count;
		}
		else
		{
			const UINT numMergedBatches = iBatch - accumulatedBatchesStart;
			RenderBatchedPrimitives(
				render_context, allVertices, numAccumulatedVertices, allIndices, numAccumulatedIndices,
				program, currentStates, _input_layout, currentTopology,
				&sortedBatches[ accumulatedBatchesStart ], numMergedBatches
				);
			// start with a new batch (with the current 'batch-breaker')
			accumulatedBatchesStart = iBatch;
			numAccumulatedVertices = batch.vertex_count;
			numAccumulatedIndices = batch.index_count;
			currentStates = batch.state;
			currentTopology = batch.topo;
		}
	}

	if( accumulatedBatchesStart < sortedBatches.num() )
	{
		const UINT numMergedBatches = sortedBatches.num() - accumulatedBatchesStart;
		RenderBatchedPrimitives(
			render_context, allVertices, numAccumulatedVertices, allIndices, numAccumulatedIndices,
			program, currentStates, _input_layout, currentTopology,
			&sortedBatches[ accumulatedBatchesStart ], numMergedBatches
			);
	}

	m_batchedVertices.RemoveAll();
	m_batchedIndices.RemoveAll();
	m_batches.RemoveAll();

	return ALL_OK;
}

ERet TbPrimitiveBatcher::BeginBatch( NwTopology::Enum topology, U32 numVertices, AuxVertex *& _batchVertices, U32 numIndices, AuxIndex *& _batchIndices )
{
	mxASSERT(numVertices > 0);
	//mxASSERT(numIndices > 0);	// numIndices can be zero if non-indexed rendering, e.g. point list

	const U32 iBaseVertex = m_batchedVertices.num();
	mxASSERT(iBaseVertex < std::numeric_limits<AuxIndex>::max());

	const U32 iBaseIndex = m_batchedIndices.num();

	const U32 newVertexCount = iBaseVertex + numVertices;
	const U32 newIndexCount = iBaseIndex + numIndices;

	const bool bNeedToFlush = (newVertexCount > MAX_VERTEX_COUNT);
	if( bNeedToFlush ) {
		this->Flush();
	}

	mxDO(m_batchedVertices.setNum( newVertexCount ));
	mxDO(m_batchedIndices.setNum( newIndexCount ));

	_batchVertices = &m_batchedVertices[ iBaseVertex ];
	_batchIndices = &m_batchedIndices[ iBaseIndex ];

	Batch	newBatch;
	newBatch.state = _render_state;
	newBatch.topo = topology;
	newBatch.startVertex = iBaseVertex;
	newBatch.vertex_count = numVertices;
	newBatch.start_index = iBaseIndex;
	newBatch.index_count = numIndices;
	m_batches.add( newBatch );

	return ALL_OK;
}

/*
-----------------------------------------------------------------------------
-----------------------------------------------------------------------------
*/
AxisArrowGeometry::AxisArrowGeometry()
{
	BuildGeometry();
}

void AxisArrowGeometry::BuildGeometry()
{
	// Pre-compute the vertices for drawing axis arrows.
	static float AXIS_ARROW_RADIUS = 0.09f * 1;
	HOT_VAR(AXIS_ARROW_RADIUS);

	static float ARROW_BASE_HEIGHT = 1.8f * 1;
	HOT_VAR(ARROW_BASE_HEIGHT);

	static float ARROW_TOTAL_LENGTH = 2.0f * 1;
	HOT_VAR(ARROW_TOTAL_LENGTH);

	//const float ARROW_CONE_HEIGHT = arrowHeadVertex.z - arrowBaseVertex.z;

	static float ARROW_CONE_HEIGHT = 1.6f * 1;
	HOT_VAR(ARROW_CONE_HEIGHT);

	m_axisRoot = V3_Set( 0,0,0 );
	m_arrowBase = V3_Set( 0,0,ARROW_BASE_HEIGHT );
	m_arrowHead = V3_Set( 0,0,ARROW_TOTAL_LENGTH );

	// Generate the axis arrow cone

	// Generate the vertices for the base of the cone

	for( UINT iSegment = 0 ; iSegment <= AXIS_ARROW_SEGMENTS ; iSegment++ )
	{
		const float theta = iSegment * (mxTWO_PI / AXIS_ARROW_SEGMENTS);	// in radians

		float	s, c;
		mmSinCos( theta, s, c );

		m_segments[ iSegment ] = V3_Set( AXIS_ARROW_RADIUS * s, AXIS_ARROW_RADIUS * c, ARROW_CONE_HEIGHT );
	}
}

void AxisArrowGeometry::Draw( TbPrimitiveBatcher& renderer, const RGBAf& color ) const
{
	// draw the 'stem'
	mxTODO("Draw cylinder as triangle list");
	renderer.DrawLine( V3_Zero(), m_arrowHead, color );

	// draw the arrow head
	for( UINT iSegment = 0 ; iSegment < AXIS_ARROW_SEGMENTS ; iSegment++ )
	{
		const V3f& p0 = m_segments[ iSegment ];
		const V3f& p1 = m_segments[ iSegment + 1 ];

		AuxVertex	a, b, c;
		a.xyz = p0;
		b.xyz = p1;
		c.xyz = m_arrowHead;

		a.rgba.v = color.ToRGBAi().u;
		b.rgba.v = color.ToRGBAi().u;
		c.rgba.v = color.ToRGBAi().u;

		// Draw the base triangle of the cone.
		// NOTE: no need because we disable backface culling
		//renderer.DrawTriangle3D( a, b, c );

		// Draw the top triangle of the cone.
		renderer.DrawSolidTriangle3D( a, b, c );
	}
}

void AxisArrowGeometry::Draw( TbPrimitiveBatcher& renderer, const M44f& transform, const RGBAf& color ) const
{
	V3f transformedHead = M44_TransformPoint( transform, m_arrowHead );

	// draw the 'stem'
	mxTODO("Draw cylinder as triangle list");
	renderer.DrawLine( M44_getTranslation(transform), transformedHead, color );

	// draw the arrow head
	for( UINT iSegment = 0 ; iSegment < AXIS_ARROW_SEGMENTS ; iSegment++ )
	{
		const V3f& p0 = M44_TransformPoint( transform, m_segments[ iSegment ] );
		const V3f& p1 = M44_TransformPoint( transform, m_segments[ iSegment + 1 ] );

		AuxVertex	a, b, c;
		a.xyz = p0;
		b.xyz = p1;
		c.xyz = transformedHead;

		a.rgba.v = color.ToRGBAi().u;
		b.rgba.v = color.ToRGBAi().u;
		c.rgba.v = color.ToRGBAi().u;

		// Draw the base triangle of the cone.
		// NOTE: no need because we disable backface culling
		//renderer.DrawTriangle3D( a, b, c );

		// Draw the top triangle of the cone.
		renderer.DrawSolidTriangle3D( a, b, c );
	}
}

void DrawGizmo(
			   const AxisArrowGeometry& gizmo
			   , const M44f& local_to_world
			   , const V3f& spectator_position
			   , TbPrimitiveBatcher & renderer
			   , float gizmo_scale
			   )
{
	const V3f translation_vector = M44_getTranslation(local_to_world);

	float scale = GetGizmoScale(spectator_position, translation_vector);
	scale *= gizmo_scale;	//0.2

	//
	M44f S = M44_Scaling(scale,scale,scale);

	//
	V3f axisX = V4_As_V3(local_to_world[0]);
	V3f axisY = V4_As_V3(local_to_world[1]);
	V3f axisZ = V4_As_V3(local_to_world[2]);

	//
	M44f matX = S*M44_FromAxes(axisY,axisZ,axisX);
	M44_SetTranslation(&matX, translation_vector);

	M44f matY = S*M44_FromAxes(axisZ,axisX,axisY);
	M44_SetTranslation(&matY, translation_vector);

	M44f matZ = S*M44_FromAxes(axisX,axisY,axisZ);
	M44_SetTranslation(&matZ, translation_vector);

	//
	gizmo.Draw(renderer, matX, RGBAf::RED);
	gizmo.Draw(renderer, matY, RGBAf::GREEN);
	gizmo.Draw(renderer, matZ, RGBAf::BLUE);
}

#if 0
ERet ReallocateBuffer( HBuffer* handle, EBufferType type, U32 oldSize, U32 newSize, const void* data )
{
	bool reCreateBuffer = handle->IsNil() || (newSize > oldSize);

	if( reCreateBuffer )
	{
		if( handle->IsValid() )
		{
			GL::DeleteBuffer( *handle );
		}
		*handle = GL::CreateBuffer( type, newSize );
	}

	GL::UpdateBuffer( GL::GetMainContext(), *handle, newSize, data );

	return ALL_OK;
}
#endif

NwDynamicBuffer::NwDynamicBuffer()
{
	m_current = 0;
	m_capacity = 0;
	m_handle.SetNil();
	m_type = 0;
}

NwDynamicBuffer::~NwDynamicBuffer()
{
	mxASSERT(m_handle.IsNil());
}

ERet NwDynamicBuffer::Initialize( EBufferType bufferType, U32 bufferSize, const char* name )
{
	mxASSERT(bufferSize > 0);

	NwBufferDescription	buffer_description(_InitDefault);
	buffer_description.type = bufferType;
	buffer_description.size = bufferSize;
	buffer_description.flags |= NwBufferFlags::Dynamic;

	m_handle = NGpu::CreateBuffer(
		buffer_description
		, nil
		IF_DEVELOPER , (name ? name : "Dynamic")
		);
	m_capacity = bufferSize;
	m_type = bufferType;
	return ALL_OK;
}

void NwDynamicBuffer::Shutdown()
{
	NGpu::DeleteBuffer( m_handle );	m_handle.SetNil();
	m_current = 0;
	m_capacity = 0;
	m_type = 0;
}

bool NwDynamicBuffer::HasSpace( U32 stride, U32 count )
{
	const U32 oldOffset = bx::strideAlign( m_current, stride );
	const U32 numBytes = stride * count;
	const U32 newOffset = oldOffset + numBytes;
	return newOffset <= m_capacity;
}

ERet NwDynamicBuffer::Allocate( U32 stride, U32 count, U32 *offset )
{
	const U32 alignedOffset = bx::strideAlign( m_current, stride );
	const U32 newOffset = alignedOffset + stride * count;
	if( newOffset <= m_capacity ) {
		*offset = alignedOffset;
		m_current = newOffset;
//DEVOUT("[%s] allocate '%u' items, '%u' bytes each: [%u -> %u]", BufferTypeT_Type().FindString( m_type ), count, stride, alignedOffset, newOffset);
		return ALL_OK;
	}
	return ERR_OUT_OF_MEMORY;
}

void NwDynamicBuffer::Reset()
{
	m_current = 0;
}

U32	NwDynamicBuffer::SpaceRemaining() const
{
	return m_capacity - m_current;
}

#if 0
namespace Fonts
{
	static const U32 SPRITE_FONT_MAGIC = 'TNOF';    // 'FONT'

	// Comparison operators make our sorted glyph vector work with std::binary_search and lower_bound.
	static inline bool operator < ( const Glyph& left, const Glyph& right)
	{
		return left.Character < right.Character;
	}
	static inline bool operator < ( wchar_t left, const Glyph& right )
	{
		return left < right.Character;
	}
	static inline bool operator < ( const Glyph& left, wchar_t right )
	{
		return left.Character < right;
	}

	static DataFormat::Enum GetEngineFormat( U32 nativeFormat )
	{
		enum {
			__DXGI_FORMAT_R8G8B8A8_UNORM = 28,
			__DXGI_FORMAT_B4G4R4A4_UNORM = 115,
			__DXGI_FORMAT_BC2_UNORM = 74
		};

		if( nativeFormat == __DXGI_FORMAT_R8G8B8A8_UNORM ) {
			return DataFormat::RGBA8;
		}
		if( nativeFormat == __DXGI_FORMAT_BC2_UNORM ) {
			return DataFormat::BC2;
		}

		return DataFormat::Unknown;
	}

	mxDEFINE_CLASS(SubRect);
	mxBEGIN_REFLECTION(SubRect)
		mxMEMBER_FIELD(left),
		mxMEMBER_FIELD(top),
		mxMEMBER_FIELD(right),
		mxMEMBER_FIELD(bottom),
	mxEND_REFLECTION

	mxDEFINE_CLASS(Glyph);
	mxBEGIN_REFLECTION(Glyph)
		mxMEMBER_FIELD(Character),
		mxMEMBER_FIELD(Subrect),
		mxMEMBER_FIELD(XOffset),
		mxMEMBER_FIELD(YOffset),
		mxMEMBER_FIELD(XAdvance),
	mxEND_REFLECTION

	mxDEFINE_CLASS(SpriteFont);
	mxBEGIN_REFLECTION(SpriteFont)
		mxMEMBER_FIELD(m_glyphs),
		mxMEMBER_FIELD(m_spacing),
		mxMEMBER_FIELD(m_lineSpacing),
		mxMEMBER_FIELD(m_invTextureWidth),
		mxMEMBER_FIELD(m_invTextureHeight),
	mxEND_REFLECTION
	SpriteFont::SpriteFont()
	{
		m_default = nil;
		m_spacing = 0.0f;
		m_lineSpacing = 0.0f;
		m_textureAtlas.SetNil();
	}
	SpriteFont::~SpriteFont()
	{
	}
	ERet SpriteFont::Load( AReader& stream )
	{
		FontHeader_d	header;
		mxDO(stream.Get( header ));

		// Validate the header.
		if( header.magic != SPRITE_FONT_MAGIC ) {
			ptERROR("Invalid font library!/n");
			return ERR_INVALID_PARAMETER;
		}

		// Read the (sorted) glyph data.
		m_glyphs.setNum( header.numGlyphs );
		mxDO(stream.Read( m_glyphs.raw(), m_glyphs.rawSize() ));
#if __cplusplus >= 201103L
		// std::is_sorted() was added in C++11
		mxASSERT2(std::is_sorted( m_glyphs.raw(), m_glyphs.raw() + m_glyphs.num() ), "Glyphs must be in ascending codepoint order");
#endif

		// Read font properties.
		m_lineSpacing = header.lineSpacing;
		
		this->SetDefaultCharacter( (UNICODECHAR)header.defaultChar );

		// Read the texture data.
		BitmapHeader	bitmap;
		mxDO(stream.Get( bitmap ));

		const U32 imageSize = bitmap.pitch * bitmap.rows;

		//StackAllocator    tempAlloc( gCore.frameAlloc );
		//void* imageData = tempAlloc.AllocA( imageSize );
		TRY_ALLOCATE_SCRACH(char*, imageData, imageSize );
		mxDO(stream.Read( imageData, imageSize ));

		const NwDataFormat::Enum textureFormat = GetEngineFormat(bitmap.format);
		chkRET_X_IF_NOT(textureFormat != NwDataFormat::Unknown, ERR_INVALID_PARAMETER);

		NwTexture2DDescription	texDesc;
		IF_DEVELOPER texDesc.name.setReference("SpriteFont");
		texDesc.format = textureFormat;
		texDesc.width = bitmap.width;
		texDesc.height = bitmap.height;
		texDesc.numMips = 1;		

		m_textureAtlas = NGpu::CreateTexture(texDesc, imageData);

		m_invTextureWidth = 1.0f / bitmap.width;
		m_invTextureHeight = 1.0f / bitmap.height;

		return ALL_OK;
	}

	void SpriteFont::Shutdown()
	{
		if( m_textureAtlas.IsValid() ) {
			NGpu::DeleteTexture( m_textureAtlas );
			m_textureAtlas.SetNil();
		}
	}

	const Glyph* SpriteFont::FindGlyph( UNICODECHAR character ) const
	{
		const Glyph* glyph = std::lower_bound( m_glyphs.raw(), m_glyphs.raw() + m_glyphs.num(), character );
		if( glyph != nil && glyph->Character == character ) {
			return glyph;
		}
		return m_default;
	}

	bool SpriteFont::ContainsCharacter( UNICODECHAR character ) const
	{
		return std::binary_search( m_glyphs.raw(), m_glyphs.raw() + m_glyphs.num(), character );
	}

	void SpriteFont::SetDefaultCharacter( UNICODECHAR character )
	{
		m_default = this->FindGlyph( character );
		if( !m_default ) {
			ptWARN("Couldn't find default glyph!/n");
			m_default = m_glyphs.GetItemPtr( 0 );
		}
	}

	float SpriteFont::GetLineSpacing() const
	{
		return m_lineSpacing;
	}

	// The core glyph layout algorithm, shared between DrawString and MeasureString.
	template< typename WHAT >    // where WHAT has operator () ( const Glyph& glyph, float x, float y, U32 index, void* userData )
	void SpriteFont::TForEachGlyph( _In_z_ wchar_t const* _text, WHAT _action, void* _userData ) const
	{
		float x = 0;
		float y = 0;
		U32 index = 0;

		for( ; *_text; _text++ )
		{
			wchar_t character = *_text;

			switch( character )
			{
			case '/r':
				// Skip carriage returns.
				continue;

			case '/n':
				// New line.
				x = 0;
				y += m_lineSpacing;
				break;

			default:
				{
					// Output this character.
					const Glyph* glyph = this->FindGlyph( character );

					x += glyph->XOffset;

					if( x < 0 ) {
						x = 0;
					}

					if( !iswspace( character )
						|| ((glyph->Subrect.right - glyph->Subrect.left) > 1)
						|| ((glyph->Subrect.bottom - glyph->Subrect.top) > 1)
						)
					{
						_action( *glyph, x, y, index++, _userData );
					}

					x += glyph->Subrect.right - glyph->Subrect.left + glyph->XAdvance + m_spacing;
				}
				break;
			}
		}
	}

	void SpriteFont::ForEachGlyph( _In_z_ wchar_t const* text, GlyphCallback* callback, void* userData ) const
	{
		struct CallbackWrapper
		{
			GlyphCallback *    m_callback;
			CallbackWrapper( GlyphCallback* _callback )
				: m_callback( _callback )
			{}
			void operator () ( const Glyph& glyph, float x, float y, U32 index, void* userData )
			{
				(*m_callback)( glyph, x, y, index, userData );
			}
		};
		this->TForEachGlyph( text, CallbackWrapper(callback), userData );
	}

	void SpriteFont::MeasureString( _In_z_ wchar_t const* text, float *width, float *height )
	{
		*width = 0;
		*height = 0;

		struct MeasureString
		{
			const float lineSpacing;
			float        maxWidth;
			float        maxHeight;
		public:
			MeasureString( float _lineSpacing )
				: lineSpacing( _lineSpacing ), maxWidth( 0.0f ), maxHeight( 0.0f )
			{}
			void operator () ( const Glyph& glyph, float x, float y, U32 index, void* userData )
			{
				float w = (float)(glyph.Subrect.right - glyph.Subrect.left);
				float h = (float)(glyph.Subrect.bottom - glyph.Subrect.top) + glyph.YOffset;

				h = std::max(h, lineSpacing);

				maxWidth = std::max(maxWidth, x + w);
				maxHeight = std::max(maxHeight, y + h);
			}
		};
		MeasureString    measureString( m_lineSpacing );
		this->TForEachGlyph( text, measureString, nil );

		*width = measureString.maxWidth;
		*height = measureString.maxHeight;
	}

	// each single character corresponds to exactly one vertex:
	// we do point sprite => quad expansion in geometry shader.
	struct TextVertex
	{
		V4f	xy_wh;
		V4f	tl_br;	// UV texture coords for top left and bottom right corners
	};
	enum { MAX_TEXT_VERTS = 512 };

	FontRenderer::FontRenderer()
	{
	}

	ERet FontRenderer::Initialize( int _capacity )
	{
		mxASSERT( _capacity > 0 );

		NwVertexDescription	vertexDesc;
		IF_DEVELOPER vertexDesc.name.setReference("DebugText");
		{			
			vertexDesc.begin();
			vertexDesc.add(NwAttributeType::Float4, NwAttributeUsage::TexCoord, 0, false);
			vertexDesc.add(NwAttributeType::Float4, NwAttributeUsage::TexCoord, 1, false);
			vertexDesc.End();
		}
		m_vertexLayout = NGpu::createInputLayout( vertexDesc );

		NwBufferDescription	buffer_description(_InitDefault);
		IF_DEVELOPER buffer_description.name.setReference("DebugText");
		{			
			buffer_description.type = Buffer_Vertex;		
			buffer_description.size = _capacity;
			buffer_description.dynamic = true;
		}
		m_vertexBuffer = NGpu::CreateBuffer( buffer_description );

		return ALL_OK;
	}
	void FontRenderer::Shutdown()
	{
		NGpu::DeleteBuffer(m_vertexBuffer);
		m_vertexBuffer.SetNil();

		NGpu::destroyInputLayout(m_vertexLayout);
		m_vertexLayout.SetNil();
	}
	ERet FontRenderer::Flush()
	{
		//UINT16		x, y;
		//U32		length;
		//wchar_t *	text;

		//while( m_textBatcher.TryRead( x, y, length, text ) )
		//{
		//	//
		//}

		UNDONE;
		//if( m_written )
		//{
		//	mxASSERT( m_written < m_capacity );

		//	//

		//	m_written = 0;
		//}
		return ALL_OK;
	}

	struct ParamData
	{
		const SpriteFont *	font;
		TextVertex *		verts;
		float				startX;
		float				startY;
		float				invScreenWidth;
		float				invScreenHeight;		
	};
	static void AddCharacterVertex( const Glyph& glyph, float x, float y, U32 index, void* param )
	{
		mxASSERT_PTR(param);
		const ParamData& data = *(const ParamData*) param;
		const SpriteFont* font = data.font;
		const float invTextureWidth = font->m_invTextureWidth;
		const float invTextureHeight = font->m_invTextureHeight;

		x += data.startX;
		y += data.startY;

		// Calculate the X and Y pixel position on the screen to start drawing to.
		float glyphLeftX, glyphTopY;
		ViewportToNDC( data.invScreenWidth, data.invScreenHeight, x, y, glyphLeftX, glyphTopY );

		const U32 glyphWidth = glyph.Subrect.right - glyph.Subrect.left;
		const U32 glyphHeight = glyph.Subrect.bottom - glyph.Subrect.top;

		// this controls the size of the character sprite
		float glyphScalingX, glyphScalingY;
		glyphScalingX = data.invScreenWidth;
		glyphScalingY = data.invScreenHeight;
		glyphScalingX *= 2.0f;
		glyphScalingY *= 2.0f;
		glyphLeftX += x * glyphScalingX;
		glyphTopY -= (y + glyph.YOffset) * glyphScalingY;

		const float glyphSizeX = (float)glyphWidth * glyphScalingX;
		const float glyphSizeY = (float)glyphHeight * glyphScalingY;

		TextVertex& vertex = data.verts[ index ];

		vertex.xy_wh = V4f::set(
			glyphLeftX,
			glyphTopY,
			glyphSizeX,
			glyphSizeY
			);

		vertex.tl_br = V4f::set(
			glyph.Subrect.left * invTextureWidth,
			glyph.Subrect.top * invTextureHeight,
			glyph.Subrect.right * invTextureWidth,
			glyph.Subrect.bottom * invTextureHeight
			);
	}

	// x and y are in viewport coordinates of the text sprite's top left corner
	ERet FontRenderer::RenderText(
		NGpu::NwRenderContext* _context, NGpu::Cmd_Draw& batch,
		UINT16 screenWidth, UINT16 screenHeight,
		const SpriteFont* font,
		UINT16 x, UINT16 y,
		const char* text, UINT16 length
		)
	{
		if( !length ) {
			length = strlen(text);
		}
		if( !length ) {
			return ALL_OK;
		}
		if( length > MAX_TEXT_VERTS ) {
			return ERR_BUFFER_TOO_SMALL;
		}

		//StackAllocator	tempAlloc( gCore.frameAlloc );
		//wchar_t * wideCharData = tempAlloc.AllocMany< wchar_t >( length + 1 );
		TRY_ALLOCATE_SCRACH(wchar_t*, wideCharData, length + 1 );
		mbstowcs( wideCharData, text, length );
		wideCharData[ length ] = 0;

		{
			ParamData	data;
			data.font = font;
			data.verts = (TextVertex*) NGpu::MapBuffer(_context, m_vertexBuffer, Map_Write_Discard);
			data.startX = x;
			data.startY = y;
			data.invScreenWidth = 1.0f / (float)screenWidth;
			data.invScreenHeight = 1.0f / (float)screenHeight;

			font->ForEachGlyph( wideCharData, &AddCharacterVertex, &data );

			NGpu::UnmapBuffer(_context, m_vertexBuffer);
		}

		batch.inputLayout = m_vertexLayout;
		batch.topology = NwTopology::PointList;

		batch.VB[0] = m_vertexBuffer;

		batch.base_vertex = 0;
		batch.vertex_count = length;
		batch.start_index = 0;
		batch.index_count = 0;

		return ALL_OK;
	}

	ERet FontRenderer::DrawTextW(
		NGpu::NwRenderContext* _context, NGpu::Cmd_Draw& batch,
		UINT16 screenWidth, UINT16 screenHeight,
		const SpriteFont* font,
		UINT16 x, UINT16 y,
		const wchar_t* text
		)
	{
		mxASSERT_PTR(text);
		const size_t length = wcslen( text );
		if( !length ) {
			return ALL_OK;
		}
		if( length > MAX_TEXT_VERTS ) {
			return ERR_BUFFER_TOO_SMALL;
		}

		{
			ParamData	data;
			data.font = font;
			data.verts = (TextVertex*) NGpu::MapBuffer(_context, m_vertexBuffer, Map_Write_Discard);
			data.startX = x;
			data.startY = y;
			data.invScreenWidth = 1.0f / screenWidth;
			data.invScreenHeight = 1.0f / screenHeight;

			font->ForEachGlyph( text, &AddCharacterVertex, &data );

			NGpu::UnmapBuffer(_context, m_vertexBuffer);
		}

		batch.inputLayout = m_vertexLayout;
		batch.topology = NwTopology::PointList;

		batch.VB[0] = m_vertexBuffer;

		batch.base_vertex = 0;
		batch.vertex_count = length;
		batch.start_index = 0;
		batch.index_count = 0;

		return ALL_OK;
	}

}//namespace Fonts
#endif

// look into "_screen_shader.h" to see how exactly the triangle is rendered
ERet push_FullScreenTriangle(
							 NGpu::NwRenderContext & render_context,
							 const NwRenderState32& state,
							 const NwShaderEffect* technique,
							 const HProgram shaderProgram
							 )
{
	NGpu::CommandBufferWriter	cmd_writer( render_context._command_buffer );

	//
	mxDO(technique->pushShaderParameters( render_context._command_buffer ));

	//
	NGpu::Cmd_Draw	batch;

	batch.SetMeshState(
		HInputLayout::MakeNilHandle(),
		HBuffer::MakeNilHandle(),
		HBuffer::MakeNilHandle(),
		NwTopology::TriangleList,
		false
		);

	batch.program = shaderProgram;

	batch.base_vertex = 0;
	batch.vertex_count = 3;
	batch.start_index = 0;
	batch.index_count = 0;

	IF_DEBUG batch.src_loc = GFX_SRCFILE_STRING;

	cmd_writer.SetRenderState(state);

	cmd_writer.Draw( batch );

	return ALL_OK;
}

mxSTOLEN("bimg, image.cpp, void decodeBlockDxt1(uint8_t _dst[16*4], const uint8_t _src[8])")

static
uint8_t bitRangeConvert(uint32_t _in, uint32_t _from, uint32_t _to)
{
	using namespace bx;
	uint32_t tmp0   = uint32_sll(1, _to);
	uint32_t tmp1   = uint32_sll(1, _from);
	uint32_t tmp2   = uint32_dec(tmp0);
	uint32_t tmp3   = uint32_dec(tmp1);
	uint32_t tmp4   = uint32_mul(_in, tmp2);
	uint32_t tmp5   = uint32_add(tmp3, tmp4);
	uint32_t tmp6   = uint32_srl(tmp5, _from);
	uint32_t tmp7   = uint32_add(tmp5, tmp6);
	uint32_t result = uint32_srl(tmp7, _from);

	return uint8_t(result);
}

// http://www.matejtomcik.com/Public/KnowHow/DXTDecompression/

static
void decodeBlockDxt(uint8_t _dst[16*4], const uint8_t _src[8])
{
	uint8_t colors[4*3];

	uint32_t c0 = _src[0] | (_src[1] << 8);
	colors[0] = bitRangeConvert( (c0>> 0)&0x1f, 5, 8);
	colors[1] = bitRangeConvert( (c0>> 5)&0x3f, 6, 8);
	colors[2] = bitRangeConvert( (c0>>11)&0x1f, 5, 8);

	uint32_t c1 = _src[2] | (_src[3] << 8);
	colors[3] = bitRangeConvert( (c1>> 0)&0x1f, 5, 8);
	colors[4] = bitRangeConvert( (c1>> 5)&0x3f, 6, 8);
	colors[5] = bitRangeConvert( (c1>>11)&0x1f, 5, 8);

	colors[6] = (2*colors[0] + colors[3]) / 3;
	colors[7] = (2*colors[1] + colors[4]) / 3;
	colors[8] = (2*colors[2] + colors[5]) / 3;

	colors[ 9] = (colors[0] + 2*colors[3]) / 3;
	colors[10] = (colors[1] + 2*colors[4]) / 3;
	colors[11] = (colors[2] + 2*colors[5]) / 3;

	for (uint32_t ii = 0, next = 8*4; ii < 16*4; ii += 4, next += 2)
	{
		int idx = ( (_src[next>>3] >> (next & 7) ) & 3) * 3;

		//NOTE: swapped the order - looks like the original code unpacks colors into BGR, but we need RGB
		//_dst[ii+0] = colors[idx+0];
		//_dst[ii+1] = colors[idx+1];
		//_dst[ii+2] = colors[idx+2];
		_dst[ii+0] = colors[idx+2];
		_dst[ii+1] = colors[idx+1];
		_dst[ii+2] = colors[idx+0];
	}
}

ERet LoadTextureArray(
	NGpu::NwRenderContext & context,
	const NwTexture2DDescription& _textureArrayDesc,
	const HTexture _textureArrayHandle,
	const AssetID* _textureAssetIds,
	UByte4 *average_colors_,
	AllocatorI & _scratch,
	const UINT _startIndex /*= 0*/
	)
{
	const bool generateMipMaps = (_textureArrayDesc.flags & NwTextureCreationFlags::GENERATE_MIPS);
	mxASSERT2(!generateMipMaps, "Mips should be generated offline!");

	DynamicArray< MipLevel_m >	mipLevels( _scratch );

	// For each array slice...
	for( UINT iTexture = _startIndex; iTexture < _textureArrayDesc.arraySize; iTexture++ )
	{
		AssetReader	streamReader;
		AssetKey assetKey;
		assetKey.id = _textureAssetIds[ iTexture ];
		assetKey.type = NwTexture::metaClass().GetTypeGUID();
		mxTRY( Resources::OpenFile( assetKey, &streamReader, AssetPackage::OBJECT_DATA ) );

		const U32 textureSize = streamReader.Length();

		void* textureData = _scratch.Allocate( textureSize, EFFICIENT_ALIGNMENT );
		mxENSURE( textureData != nil, ERR_OUT_OF_MEMORY, "" );

		mxDO( streamReader.Read( textureData, textureSize ) );

		const U32 magicNum = *(U32*) textureData;
		mxASSERT( magicNum == TEXTURE_FOURCC );

		const TextureHeader_d* header = static_cast< TextureHeader_d* >(textureData);
		const void* imageData = header + 1;

		//
		TextureImage_m image;
		image.Init( imageData, *header );

		if( image.format != _textureArrayDesc.format ||
			image.width < _textureArrayDesc.width || image.height < _textureArrayDesc.height
			)
		{
			ptERROR( "'%s'('%s'): All textures in a texture array must have the same format '%s' and size %dx%d!",
				AssetId_ToChars( assetKey.id ), DataFormatT_Type().FindString( image.format ),
				DataFormatT_Type().FindString( _textureArrayDesc.format ), _textureArrayDesc.width, _textureArrayDesc.height
				);
			return ERR_INVALID_PARAMETER;
		}

		UINT iFirstMip = 0;	// for skipping too detailed mip levels
		if( image.numMips > _textureArrayDesc.numMips )
		{
			ptWARN( "'%s'[%u] has Mip-Map levels: %u, while texture array has %u",
				AssetId_ToChars( assetKey.id ), iTexture, image.numMips, _textureArrayDesc.numMips );
			// start with the matching mip
			iFirstMip = image.numMips - _textureArrayDesc.numMips;
		}

		const UINT numMipsToCopy = image.numMips;

		mxDO( mipLevels.setNum( numMipsToCopy ) );
		mxDO( ParseMipLevels( image, mipLevels.raw(), numMipsToCopy ) );

		if( average_colors_ )
		{
			const MipLevel_m& coarsest_top_level_mip = mipLevels[ image.numMips - 1 ];	// 1x1

			UByte4	decoded_colors[16] = {0};
			decodeBlockDxt( (U8*)decoded_colors, (U8*)coarsest_top_level_mip.data );

			average_colors_[iTexture] = decoded_colors[0];
		}

		for( U8 iMip = iFirstMip; iMip < numMipsToCopy; iMip++ )
		{
			const MipLevel_m& mip = mipLevels[ iMip ];

			NwTextureRegion	updateRegion;
			updateRegion.x = 0;
			updateRegion.y = 0;
			updateRegion.z = 0;
			updateRegion.width = mip.width;
			updateRegion.height = mip.height;
			updateRegion.depth = 1;
			updateRegion.mipMapLevel = iMip - iFirstMip;
			updateRegion.arraySlice = iTexture;

			//
			const NGpu::Memory* temp = NGpu::Allocate( mip.size );
			mxENSURE( temp, ERR_OUT_OF_MEMORY, "Failed to alloc mip (%u bytes)", mip.size );

			memcpy( temp->data, mip.data, mip.size );

			NGpu::UpdateTexture(
				_textureArrayHandle,
				updateRegion,
				temp
			);

		}//For each Mip-Map level.

		_scratch.Deallocate( textureData );
	}//For each texture in the texture array.

	return ALL_OK;
}

void TbDebugLineRenderer::flush( TbPrimitiveBatcher & renderer )
{
	for( UINT i = 0 ; i < _debug_lines.num(); i++ )
	{
		const DebugLine& debug_line = _debug_lines[i];

		renderer.DrawLine3D( debug_line.start, debug_line.end );
	}

	_debug_lines.RemoveAll();

	//
	for( UINT i = 0 ; i < _wire_tris.num(); i++ )
	{
		const WireTri& tri = _wire_tris[i];

		renderer.DrawWireTriangle( tri.v0, tri.v1, tri.v2, tri.color );
	}

	_wire_tris.RemoveAll();

	//
	for( UINT i = 0 ; i < _solid_tris.num(); i++ )
	{
		const SolidTri& tri = _solid_tris[i];

		AuxVertex	v0;
		v0.xyz = tri.v0;
		v0.rgba = tri.rgba;

		AuxVertex	v1;
		v1.xyz = tri.v1;
		v1.rgba = tri.rgba;

		AuxVertex	v2;
		v2.xyz = tri.v2;
		v2.rgba = tri.rgba;

		renderer.DrawSolidTriangle3D( v0, v1, v2 );
	}

	_solid_tris.RemoveAll();

	//
	for( UINT i = 0 ; i < _wire_aabbs.num(); i++ )
	{
		const WireAABB& box = _wire_aabbs[i];

		renderer.DrawAABB( box.aabb, box.color );
	}

	_wire_aabbs.RemoveAll();
}
