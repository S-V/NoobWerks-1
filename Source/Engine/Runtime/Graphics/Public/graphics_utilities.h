// Auxiliary renderer - immediate-mode emulation.
// Used mostly for debugging and editor purposes.
#pragma once

#include <Base/Base.h>
#include <Base/Util/Color.h>
#include <GPU/Public/graphics_device.h>
#include <Graphics/Public/graphics_formats.h>

class NwShaderEffect;

/*
=======================================================================
	UTILITIES
=======================================================================
*/

/// a helper struct for creating hw textures
struct TextureImage_m
{
	const void *	data;	//!< pointer to raw image data [optional]
	U32				size;	//!< total size of raw image data
	U16				width;	//!< texture width (in texels)
	U16				height;	//!< texture height (in texels)
	U16				depth;	//!< depth of a volume texture

	ETextureType	type;
	DataFormatT		format;	//!< engine-specific texture format
	U8				numMips;//!< mip level count
	U8				arraySize;//!< Number of textures in the texture array.

public:
	void Init( const void* image_data, const TextureHeader_d& image_header );
};

/// a helper struct for parsing mipmap levels from texture data
struct MipLevel_m
{
	const void *data;	//!< pointer to data of this mipmap level
	U32			size;	//!< size of this mipmap level, in bytes
	U32			pitch;	//!< size of a row of blocks, in bytes
	U16			width;	//!< width of this mipmap level (in texels)
	U16			height;	//!< height of this mipmap level (in texels)
};

/// returns surface information for a particular format
void GetSurfaceInfo(
					const DataFormatT format,
					U32 width, U32 height,
					U32 &_textureSize,	// total size of this mip level, in bytes
					U32 &_rowCount,		// number of rows (horizontal blocks)
					U32 &_rowPitch		// (aligned) size of each row (aka pitch), in bytes
					);

mxDEPRECATED
U32 CalculateTexture2DSize( U16 _width, U16 _height, DataFormatT _format, U8 _numMips );

// see imageGetSize() in bimg/image.cpp
//U32 calculateTextureSize( U16 _width, U16 _height, U16 depth
//						 , DataFormatT _format
//						 , U8 _numLayers
//						 , bool hasMiplevels
//						 , bool isCubemap );

/// parses the texture mipmap levels into the user-supplied array
ERet ParseMipLevels( const TextureImage_m& _image, MipLevel_m *_mips, U8 _maxMips );


//=====================================================================
//	UTILITY FUNCTIONS FOR RAPID TESTING AND PROTOTYPING
//	THEY SHOULD BE AVOIDED IN PERFORMANCE CRITICAL CODE
//=====================================================================

//TbCBufferBinding* FxSlow_FindCBufferBinding( TbShader* shader, const char* name );
//TbResourceBinding* FxSlow_FindResourceBinding( TbShader* shader, const char* name );
//ERet FxSlow_SetSampler( TbShader* shader, const char* name, HSamplerState handle );
//ERet FxSlow_SetInput( TbShader* shader, const char* name, HShaderInput handle );
//ERet FxSlow_SetOutput( TbShader* shader, const char* name, HShaderOutput handle );
//ERet FxSlow_SetUniform( TbShader* shader, const char* name, const void* data );
///// flushes all changed constant buffers to the GPU
//ERet FxSlow_Commit( NGpu::NwRenderContext & context, const NGpu::LayerID view_id, TbShader* shader, const NGpu::SortKey sortKey );


#pragma pack (push,1)
/// Vertex type used for auxiliary rendering (editor/debug visualization/etc).
//@TODO: reduce to 16 bytes
struct AuxVertex: CStruct
{
	V3f		xyz;	//12 POSITION
	Half2	uv;		//4 TEXCOORD0
	Half2	uv2;	//4 TEXCOORD1
	UByte4	rgba;	//4 COLOR
	UByte4	N;		//4 NORMAL
	UByte4	T;		//4 TANGENT
	//32
public:
	mxDECLARE_REFLECTION;
	static void buildVertexDescription( NwVertexDescription *description_ );
};
ASSERT_SIZEOF(AuxVertex, 32);
#pragma pack (pop)

/// Index type used for auxiliary rendering (editor/debug visualization/etc).
typedef U32 AuxIndex;


/// interface for doing "immediate-mode" line/wireframe rendering (used for debug viz/editor gizmo)
struct ALineRenderer
{
	/// Draws a world-space line
	virtual void DrawLine3D(
		const AuxVertex& start, const AuxVertex& end
	) = 0;

	/// can be overridden for more efficiency
	virtual void DrawWireQuad3D(
		const AuxVertex& a, const AuxVertex& b, const AuxVertex& c, const AuxVertex& d
	);

public:

	/// Draws a world-space line
	void DrawLine(
		const V3f& start,
		const V3f& end,
		const RGBAf& startColor = RGBAf::WHITE,
		const RGBAf& endColor = RGBAf::WHITE
	);

	void DrawWireTriangle(
		const V3f& a, const V3f& b, const V3f& c,
		const RGBAf& color = RGBAf::WHITE
	);

	/// wireframe axis-aligned bounding box
	void DrawAABB(
		const V3f& aabbMin, const V3f& aabbMax,
		const RGBAf& color = RGBAf::WHITE
	);
	void DrawAABB(
		const AABBf& aabb,
		const RGBAf& color = RGBAf::WHITE
	)
	{
		this->DrawAABB( aabb.min_corner, aabb.max_corner, color );
	}

	void DrawCircle(
		const V3f& origin,
		const V3f& right,
		const V3f& up,
		const RGBAf& color,
		const float radius,
		const int numSides = 16
	);

	// draws an asterisk
	void DrawSternchen(
		const V3f& origin,
		const V3f& right,
		const V3f& up,
		const RGBAf& color,
		const float radius,
		const int numSides = 3	// 1 = horizontal line, 2 - cross, 3 - asterisk.
	);

	void DrawWireFrustum(
		const M44f& _viewProjection,
		const RGBAf& _color = RGBAf::WHITE
	);

	void drawFrame( const V3f& origin
		, const V3f& axisX, const V3f& axisY, const V3f& axisZ
		, const float scale = 1 )
	{
		this->DrawLine( origin, origin + axisX*scale, RGBAf::RED, RGBAf::RED );
		this->DrawLine( origin, origin + axisY*scale, RGBAf::GREEN, RGBAf::GREEN );
		this->DrawLine( origin, origin + axisZ*scale, RGBAf::BLUE, RGBAf::BLUE );
	}

protected:
	virtual ~ALineRenderer() {}
};

/// interface for doing "immediate-mode" rendering (used for debug viz/editor gizmo)
struct ADebugDraw : ALineRenderer
{
	virtual void DrawSolidTriangle3D(
		const AuxVertex& a, const AuxVertex& b, const AuxVertex& c
	) = 0;

	virtual void DrawSolidQuad3D(
		const AuxVertex& a, const AuxVertex& b, const AuxVertex& c, const AuxVertex& d
	) = 0;

	virtual void setTechnique( NwShaderEffect* technique ) = 0;
	virtual void SetViewID( const NGpu::LayerID viewId ) = 0;
	virtual void SetStates( const NwRenderState32& states ) = 0;
	virtual void PushState() = 0;
	virtual void PopState() = 0;

	virtual ~ADebugDraw() {}
};

/*
--------------------------------------------------------------
	TbPrimitiveBatcher

	primitive-batching immediate mode renderer;
	designed for convenience, not for speed;
	is meant to be used for debug visualization, etc.
--------------------------------------------------------------
*/
class TbPrimitiveBatcher: public ADebugDraw, TSingleInstance< TbPrimitiveBatcher > {
public:
	TbPrimitiveBatcher();
	~TbPrimitiveBatcher();

	ERet Initialize( HInputLayout _vertexFormat );
	void Shutdown();

	ERet reserveSpace( UINT num_vertices, UINT num_indices, UINT num_batches );

	// Fixed-function pipeline emulation

	virtual void setTechnique( NwShaderEffect* technique ) override;
	virtual void SetViewID( const NGpu::LayerID viewId ) override;
	virtual void SetStates( const NwRenderState32& states ) override;
	virtual void PushState() override;
	virtual void PopState() override;

	// don't forget to call this function at the end of each frame
	// to flush any batched primitives and prevent flickering

	ERet Flush();

	//=== Wireframe mode

	virtual void DrawLine3D(
		const AuxVertex& start, const AuxVertex& end
	) override;

	//void DrawTriangle3D(
	//	const AuxVertex& a, const AuxVertex& b, const AuxVertex& c,
	//	const RGBAf& color = RGBAf::WHITE
	//);
	void DrawDashedLine(
		const V3f& start,
		const V3f& end,
		const float dashSize,
		const RGBAf& startColor = RGBAf::WHITE,
		const RGBAf& endColor = RGBAf::WHITE
	);
	void DrawAxes( float length = 1.0f );

	//void DrawWireBox(
	//	const V3f& center,
	//	const float sideLength,
	//	const RGBAf& color = RGBAf::WHITE
	//);

	// 'wireframe' arrow
	void DrawArrow(
		const M44f& arrowTransform,
		const RGBAf& color,
		float arrowLength = 1.f,
		float headSize = 0.1f
	);

	void DrawGrid(
		const V3f& origin,	// center of the grid
		const V3f& axisX,	// length = half size along X axis
		const V3f& axisY,	// length = half size along Y axis
		int numXDivisions,
		int numYDivisions,
		const RGBAf& color
	);
	//void DrawCube( const M44f& worldMatrix, const RGBAf& color );
	//void DrawOBB( const rxOOBB& box, const RGBAf& color );
	//void DrawFrustum( const XNA::Frustum& frustum, const RGBAf& color );
	//void DrawViewFrustum( const ViewFrustum& frustum, const RGBAf& color );

	virtual void DrawWireQuad3D(
		const AuxVertex& a, const AuxVertex& b, const AuxVertex& c, const AuxVertex& d
	) override;

	//=== Solid mode

mxREFACTOR("these functions cannot be placed into ADebugDraw, because they use BeginBatch()");
	virtual void DrawSolidTriangle3D(
		const AuxVertex& a, const AuxVertex& b, const AuxVertex& c
	) override;

	virtual void DrawSolidQuad3D(
		const AuxVertex& a, const AuxVertex& b, const AuxVertex& c, const AuxVertex& d
	) override;

	void DrawSolidBox(
		const V3f& origin, const V3f& halfSize,
		const RGBAf& color = RGBAf::WHITE
	);

	void DrawSolidCircle(
		const V3f& origin,
		const V3f& right,
		const V3f& up,
		const RGBAf& color,
		float radius,
		int numSides = 16
	);

	void DrawSolidSphere(
		const V3f& center, float radius,
		const RGBAf& color = RGBAf::WHITE,
		int numStacks = 16, int numSlices = 16
	);
mxUNDONE
	//void DrawSolidCylinder(
	//	const V3f& center, float radius,
	//	const RGBAf& color = RGBAf::WHITE,
	//	int numStacks = 16, int numSlices = 16
	//);

	void DrawSolidArrow(
		const M44f& arrowTransform,
		const RGBAf& color,
		float arrowLength = 1.f,
		float headSize = 0.1f
	);

	//void DrawSolidAABB( const rxAABB& box, const RGBAf& color = RGBAf::WHITE );
	mxUNDONE
#if 0
	void DrawRing( const XMFLOAT3& Origin, const XMFLOAT3& MajorAxis, const XMFLOAT3& MinorAxis, const RGBAf& Color );
	void DrawSphere( const XNA::Sphere& sphere, const RGBAf& Color );
	void DrawRay( const XMFLOAT3& Origin, const XMFLOAT3& Direction, BOOL bNormalize, const RGBAf& Color );
#endif

	// accepts sprite's coordinates in world space
	void DrawSprite(
		const M44f& cameraWorldMatrix,
		const V3f& spriteOrigin,
		const float spriteSizeX = 1.0f, const float spriteSizeY = 1.0f,
		const RGBAf& color = RGBAf::WHITE
	);

	void DrawConvexPolygon(
		const int num_vertices
		, AuxVertex *&vertices_	// allocated internally, you must copy data into it
	);

	void DrawPoint( const AuxVertex& _p );

private:
	TArray< AuxVertex >		m_batchedVertices;
	TArray< AuxIndex >		m_batchedIndices;

	struct Batch {
		NwRenderState32	state;
		NwTopology::Enum	topo;
		U32	startVertex;
		U32	vertex_count;
		U32	start_index;
		U32	index_count;
	};
	TArray< Batch >			m_batches;

	TPtr< NwShaderEffect >	m_technique;
	NGpu::SortKey				m_viewId;
	NwRenderState32			_render_state;
	HInputLayout			_input_layout;
	NwTopology::Enum			m_topology;

	struct SavedState {
		NwShaderEffect *	technique;
		NwRenderState32		states;
		NwTopology::Enum		topology;
	};
	TFixedArray< SavedState, 16 >	m_stateStack;

private:
	ERet BeginBatch( NwTopology::Enum topology, U32 numVertices, AuxVertex *& vertices, U32 numIndices, AuxIndex *& indices );

	static const U32 MAX_VERTEX_COUNT = 65535 * 4;
	//( 1 << (BYTES_TO_BITS(sizeof(AuxIndex))-1) ); //std::numeric_limits<AuxIndex>::max() - 1;

	static ERet RenderBatchedPrimitives(
		NGpu::NwRenderContext & context,
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
		);
};

/// this can be used for drawing translation gizmo arrows / coordinate frames
struct AxisArrowGeometry
{
	enum { AXIS_ARROW_SEGMENTS = 6 };

	V3f	m_axisRoot;
	V3f	m_arrowBase;
	V3f	m_arrowHead;
	V3f	m_segments[ AXIS_ARROW_SEGMENTS + 1 ];

public:
	AxisArrowGeometry();

	void BuildGeometry();

	void Draw( TbPrimitiveBatcher& renderer, const RGBAf& color ) const;
	void Draw( TbPrimitiveBatcher& renderer, const M44f& transform, const RGBAf& color ) const;
};

/// Scaling so gizmo stays same size no matter what perspective/fov/distance
static inline float GetGizmoScale( const V3f& eyePos, const V3f& objPos )
{
	// Scale the gizmo relative to the distance.
	float distance = V3_Length(eyePos - objPos);
	float scaleFactor = distance;
	return scaleFactor;
}

void DrawGizmo(
			   const AxisArrowGeometry& gizmo
			   , const M44f& local_to_world
			   , const V3f& spectator_position
			   , TbPrimitiveBatcher & renderer
			   , float gizmo_scale = 0.2f
			   );

ERet ReallocateBuffer( HBuffer* handle, EBufferType type, U32 oldSize, U32 newSize, const void* data );

struct NwDynamicBuffer: NonCopyable
{
	U32		m_current;	//!< Current buffer offset.
	U32		m_capacity;	//!< Allocated buffer size.
	U32		m_type;		//!< EBufferType
	HBuffer	m_handle;	//!< Dynamic buffer handle.
public:
	NwDynamicBuffer();
	~NwDynamicBuffer();

	ERet Initialize( EBufferType bufferType, U32 bufferSize, const char* name = nil );
	void Shutdown();

	bool HasSpace( U32 stride, U32 count );
	/// Returns offset from the start of the data.
	ERet Allocate( U32 stride, U32 count, U32 *offset );
	void Reset();

	U32	SpaceRemaining() const;
};

/// A wrapper for a dynamic vertex buffer and a dynamic index buffer.
struct DynamicMesh
{
	NwDynamicBuffer	m_VB;	//!< Dynamic vertex buffer.
	NwDynamicBuffer	m_IB;	//!< Dynamic index buffer.
};

/*
=======================================================================
	DEBUG FONT
=======================================================================
*/
mxSTOLEN("DirectXTK - the DirectX Tool Kit");
#if 0
namespace Fonts
{
#pragma pack(push,1)
	struct FontHeader_d
	{
		U32		magic;			// 'FONT'
		FLOAT32		lineSpacing;	// vertical spacing
		U32		defaultChar;	// Default character
		U32		numGlyphs;
		// followed by:
		// FontGlyph glyphs[ numGlyphs ];
		// BitmapHeader;
		// texture data.
	};
	struct SubRect: CStruct
	{
		U32		left;	//!< The upper-left corner x-coordinate.
		U32		top;	//!< The upper-left corner y-coordinate.
		U32		right;	//!< The lower-right corner x-coordinate.
		U32		bottom;	//!< The lower-right corner y-coordinate.
	public:
		mxDECLARE_CLASS(SubRect,CStruct);
		mxDECLARE_REFLECTION;
	};
	struct Glyph: CStruct
	{
		U32		Character;  //!< unicode character code
		SubRect		Subrect;    //!< texture rect, in texels
		FLOAT32		XOffset;    //!< dunno, something in texels
		FLOAT32		YOffset;    //!< small letters are shifted down by this value (in texels)
		FLOAT32		XAdvance;   //!< character spacing - negative closer together, positive further apart.
	public:
		mxDECLARE_CLASS(Glyph,CStruct);
		mxDECLARE_REFLECTION;
	};
	struct BitmapHeader
	{
		U32		width;	//!< in texels
		U32		height;	//!< in texels
		U32		format;	//!< DXGI_FORMAT
		U32		pitch;
		U32		rows;
		// followed by:
		// BYTE imageData[ pitch * rows ];
	};
#pragma pack(pop)

	struct SpriteFont: CStruct
	{
		TBuffer< Glyph >	m_glyphs;
		const Glyph *		m_default;		//!< initialized upon loading
		float				m_spacing;		//!< horizontal spacing, zero by default
		float				m_lineSpacing;	//!< vertical spacing
		HTexture			m_textureAtlas;	//!< initialized upon loading
		float				m_invTextureWidth;
		float				m_invTextureHeight;
	public:
		mxDECLARE_CLASS(SpriteFont,CStruct);
		mxDECLARE_REFLECTION;

		SpriteFont();
		~SpriteFont();

		ERet Load( AReader& stream );
		void Shutdown();

		const Glyph* FindGlyph( UNICODECHAR character ) const;
		bool ContainsCharacter( UNICODECHAR character ) const;
		void SetDefaultCharacter( UNICODECHAR character );

		float GetLineSpacing() const;
		void MeasureString( _In_z_ wchar_t const* text, float *width, float *height );

		template< typename WHAT >    // where WHAT has operator () ( const Glyph& glyph, float x, float y, U32 index, void* userData )
		void TForEachGlyph( _In_z_ wchar_t const* text, WHAT action, void* userData ) const;

		typedef void GlyphCallback( const Glyph& glyph, float x, float y, U32 index, void* userData );
		void ForEachGlyph( _In_z_ wchar_t const* text, GlyphCallback* callback, void* userData ) const;
	};

	class FontRenderer
	{
		//int				m_written;	// write offset, bytes
		//int				m_capacity;	// buffer size, in bytes

		HBuffer			m_vertexBuffer;
		HInputLayout	m_vertexLayout;
	public:
		FontRenderer();

		ERet Initialize( int _capacity = mxKiB(64) );
		void Shutdown();

		ERet Flush();

		// x and y are in viewport coordinates of the text sprite's top left corner
		ERet RenderText(
			GL::RenderContext* _context, GL::Cmd_Draw& batch,
			U16 screen_width, U16 screen_height,
			const SpriteFont* font,
			U16 x, U16 y,
			const char* text, U16 length = 0
		);

		ERet DrawTextW(
			GL::RenderContext* _context, GL::Cmd_Draw& batch,
			U16 screen_width, U16 screen_height,
			const SpriteFont* font,
			U16 x, U16 y,
			const wchar_t* text
		);
	};

}// namespace Fonts
#endif


ERet push_FullScreenTriangle(
							 NGpu::NwRenderContext & context,
							 const NwRenderState32& state,
							 const NwShaderEffect* technique,
							 const HProgram shaderProgram
							 );

ERet LoadTextureArray(
	NGpu::NwRenderContext & context,
	const NwTexture2DDescription& _textureArrayDesc,
	const HTexture _textureArrayHandle,
	const AssetID* _textureAssetIds,
	UByte4 *average_colors_,
	AllocatorI & _scratch,
	const UINT _startIndex = 0
	);

///
class TbDebugLineRenderer
	: public ALineRenderer
	, public  TGlobal< TbDebugLineRenderer >
{
	struct DebugLine
	{
		AuxVertex	start;
		AuxVertex	end;
	};
	TArray< DebugLine >	_debug_lines;

	struct WireTri
	{
		V3f		v0,v1,v2;	//36
		RGBAf	color;
	};
	TArray< WireTri >	_wire_tris;

	struct SolidTri
	{
		V3f		v0,v1,v2;	//36
		UByte4	rgba;
	};
	TArray< SolidTri >	_solid_tris;

	struct WireAABB
	{
		AABBf	aabb;	//24
		RGBAf	color;
	};
	TArray< WireAABB >	_wire_aabbs;

public:
	TbDebugLineRenderer()
	{
		//
	}

	void ReserveSpace( UINT num_lines, UINT num_tris, UINT num_aabbs )
	{
		_debug_lines.Reserve( num_lines );
		_wire_tris.Reserve( num_tris );
		_solid_tris.Reserve( num_tris );
		_wire_aabbs.Reserve( num_aabbs );
	}

	virtual void DrawLine3D(
		const AuxVertex& start, const AuxVertex& end
	) override
	{
		DebugLine debug_line = { start, end };
		_debug_lines.add( debug_line );
	}

	void addWireTriangle(
		const V3f& a, const V3f& b, const V3f& c,
		const RGBAf& color
		)
	{
		WireTri debug_tri = { a, b, c, color };
		_wire_tris.add( debug_tri );
	}

	void addSolidTriangle(
		const V3f& a, const V3f& b, const V3f& c,
		const RGBAf& color
		)
	{
		SolidTri debug_tri = { a, b, c, color.ToRGBAi().u };
		_solid_tris.add( debug_tri );
	}

	void addWireAABB(
		const AABBf& aabb,
		const RGBAf& color
	)
	{
		WireAABB debug_aabb = { aabb, color };
		_wire_aabbs.add( debug_aabb );
	}

	void flush( TbPrimitiveBatcher & renderer );
};




class ADebugTextRenderer
{
public:
	virtual ~ADebugTextRenderer() {}

	virtual void drawText(
		const V3f& position,
		const RGBAf& color,
		const V3f* normal_for_culling,
		const float font_scale,	// 1 by default
		const char* fmt, ...
		) = 0;
};
