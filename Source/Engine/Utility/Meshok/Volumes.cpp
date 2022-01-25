#include "stdafx.h"
#pragma hdrstop

#include <algorithm>	// std::sort()

#include <Base/Template/Containers/Array/DynamicArray.h>
#include <Base/Template/Containers/Array/TInplaceArray.h>
#include <Base/Template/Containers/Blob.h> 
#include <Core/Memory.h>	// scratch heap
#include <Core/Util/ScopedTimer.h>
#include <Engine/WindowsDriver.h>

#include <Meshok/Volumes.h>
//#include <Meshok/DualMarchingCubes.h>	// for debugging ambiguous cell configurations
#include <Meshok/Morton.h>
#include <Meshok/Octree.h>
#include <Meshok/Quadrics.h>

#include <GPU/Public/graphics_device.h>
#include <Graphics/Public/graphics_utilities.h>	// TbPrimitiveBatcher

#include <Rendering/Public/Core/Mesh.h>

namespace VX
{

/// @todo: vectorize and use branchless DDA?: https://www.shadertoy.com/user/fb39ca4/sort=popular&from=0&num=8
VoxelGridIterator::VoxelGridIterator(
	/// the start position of the ray
	const V3f& _origin,
	/// the direction of the ray
	const V3f& _direction,
	/// the side dimensions of a single cell
	const V3f& _cellSize
	)
{
#define ZERO_EPSILON 0.0001f

	mxASSERT( V3_IsNormalized( _direction ) );

	// 1. Initialization phase.

	const V3f invCellSize = V3_Reciprocal( _cellSize );

	const V3f absDir = V3_Abs( _direction );
	// for determining the fraction of the cell size the ray travels in each step:
	const V3f invAbsDir = {
		(absDir.x > ZERO_EPSILON) ? (1.0f / absDir.x) : 1.0f,
		(absDir.y > ZERO_EPSILON) ? (1.0f / absDir.y) : 1.0f,
		(absDir.z > ZERO_EPSILON) ? (1.0f / absDir.z) : 1.0f
	};

	// 1.1. Identify the voxel containing the ray origin.
	// these are integer coordinates:
	const Int3 startVoxel(
		(int) mmFloor( _origin.x * invCellSize.x ),
		(int) mmFloor( _origin.y * invCellSize.y ),
		(int) mmFloor( _origin.z * invCellSize.z )
	);

	m_iVoxel = startVoxel;

	// 1.2. Identify which coordinates are incremented/decremented as the ray crosses voxel boundaries.

	/// The integral steps in each primary direction when we iterate (1, 0 or -1)
	m_iStep.x = Sign( _direction.x );	// signum
	m_iStep.y = Sign( _direction.y );
	m_iStep.z = Sign( _direction.z );

	// 1.3. Determine the values of t at which the ray crosses the first voxel boundary along each dimension.

	const float minX = startVoxel.x * _cellSize.x;
	const float maxX = minX + _cellSize.x;
	m_fNextT.x = ( (m_iStep.x >= 0) ? (maxX - _origin.x) : (_origin.x - minX) ) * invAbsDir.x;

	const float minY = startVoxel.y * _cellSize.y;
	const float maxY = minY + _cellSize.y;
	m_fNextT.y = ( (m_iStep.y >= 0) ? (maxY - _origin.y) : (_origin.y - minY) ) * invAbsDir.y;

	const float minZ = startVoxel.z * _cellSize.z;
	const float maxZ = minZ + _cellSize.z;
	m_fNextT.z = ( (m_iStep.z >= 0) ? (maxZ - _origin.z) : (_origin.z - minZ) ) * invAbsDir.z;

	// 1.4. Compute how far we have to travel in the given direction (in units of t)
	// to reach the next voxel boundary (with each movement equal to the size of a cell) along each dimension.

	/// The t value which moves us from one voxel to the next
	m_fDeltaT = V3_Multiply( _cellSize, invAbsDir );

	// 2. Traversal phase is minimal.
	// ...

#undef ZERO_EPSILON
}


void ADebugView::addIsolatedVertex( const V3f& pos, const unsigned index )
{
	this->addPoint( pos, RGBAf::RED, 1.0f );
};

void ADebugView::addEdgeIntersection(
	const V3f& edge_start, const V3f& edge_end,
	const V3f& intersection_point,
	const V3f& surface_normal
	)
{
	this->addPoint( edge_start, RGBAf::WHITE, 0.1f );
	this->addPoint( edge_end, RGBAf::DARK_GREY, 0.1f );
	this->addLine( edge_start, edge_end, RGBAf::WHITE );

	this->addPoint( intersection_point, RGBAf::RED, 0.2f );
	this->addLine(
		intersection_point,
		intersection_point + surface_normal,
		RGBAf::make( surface_normal.x, surface_normal.y, surface_normal.z )
		);
};

void ADebugView::addMissedRayQuadIntersection(
	const V3f& segment_start
	, const EAxis ray_axis
	, const float max_ray_distance
	, const V3f quad_vertices[4]
	, const CubeMLf cells_bounds[4]
	)
{
	const float scale = 1.0f;

	//
	this->addPoint( segment_start, RGBAf::WHITE, scale * 0.25f );

	//
	V3f segment_end = segment_start;
	segment_end[ ray_axis ] += max_ray_distance;
	this->addPoint( segment_end, RGBAf::RED, scale * 0.5f );

	this->addLine( segment_start, segment_end, RGBAf::WHITE );


	//
	V3f prev_vertex = quad_vertices[3];
	for( int i = 0; i < 4; i++ )
	{
		this->addPoint( quad_vertices[i], dbg_cellColorInQuad(i), scale * 0.2f );

		this->addLine( prev_vertex, quad_vertices[i], RGBAf::BLACK );
		prev_vertex = quad_vertices[i];

		//
		this->addBox( cells_bounds[i].ToAabbMinMax(), dbg_cellColorInQuad(i) );
	}//for each quad vertex/cell
}

void ADebugView::addAmbiguousCell(
	ADebugView * _dbgView,
	const AABBf& _bounds,
	const U8 _signMask
	)
{
	UNDONE;
	//const Meshok::DMC::CellConfiguration& cellConfiguration = Meshok::DMC::g_LUT_TMP[ _signMask ];
	//mxASSERT(cellConfiguration.num_dual_vertices > 1);
	//const RGBAf color = RGBAf::fromRgba32( Dbg::g_DualVertexColors[ cellConfiguration.num_dual_vertices-1 ] );
	//_dbgView->addBox( _bounds, color );

	//_dbgView->addPoint( _vertex, color, 1.0f );
}

//#define DEBUG_VIEW_SCOPED_LOCK		SpinWait::Lock	scopedLock( m_CS )
#define DEBUG_VIEW_SCOPED_LOCK

DebugViewProxy::DebugViewProxy( ADebugView * client, const M44f& _worldTransform )
	: m_client( client )
	, m_worldTransform( _worldTransform )
{
//	m_CS.Initialize();
}

DebugViewProxy::~DebugViewProxy()
{
//	m_CS.Shutdown();
}

void DebugViewProxy::addLine( const V3f& _start, const V3f& _end, const RGBAf& _color )
{
	DEBUG_VIEW_SCOPED_LOCK;
	m_client->addLine( M44_TransformPoint( m_worldTransform, _start ), M44_TransformPoint( m_worldTransform, _end ), _color );
};
void DebugViewProxy::addPoint( const V3f& _point, const RGBAf& _color, float _scale /*= 1.0f*/ )
{
	DEBUG_VIEW_SCOPED_LOCK;
	m_client->addPoint( M44_TransformPoint( m_worldTransform, _point ), _color, _scale );
};
void DebugViewProxy::addClampedVertex( const V3f& _origPos, const V3f& _clampedPos )
{
	DEBUG_VIEW_SCOPED_LOCK;
	m_client->addClampedVertex(
		M44_TransformPoint( m_worldTransform, _origPos ),
		M44_TransformPoint( m_worldTransform, _clampedPos )
		);
};
void DebugViewProxy::addSharpFeatureVertex( const V3f& _point, bool _isCorner )
{
	DEBUG_VIEW_SCOPED_LOCK;
	m_client->addSharpFeatureVertex( M44_TransformPoint( m_worldTransform, _point ), _isCorner );
};
void DebugViewProxy::addPointWithNormal( const V3f& _point, const V3f& _normal )
{
	DEBUG_VIEW_SCOPED_LOCK;
	m_client->addPointWithNormal( M44_TransformPoint( m_worldTransform, _point ), _normal );
};
void DebugViewProxy::addBox( const AABBf& _bounds, const RGBAf& _color )
{
	DEBUG_VIEW_SCOPED_LOCK;
	m_client->addBox( _bounds.transformed( m_worldTransform ), _color );
};
void DebugViewProxy::addTransparentBox( const AABBf& _bounds, const float _rgba[4] )
{
	DEBUG_VIEW_SCOPED_LOCK;
	m_client->addTransparentBox( _bounds.transformed( m_worldTransform ), _rgba );
}
void DebugViewProxy::addCell(
	const AABBf& _bounds,
	const U8 _signMask,
	const int _numPoints,
	const V3f* _points,
	const V3f* _normals,
	bool _clamped	//!< true if the vertex was clamped to lie inside the cube
)
{
	DEBUG_VIEW_SCOPED_LOCK;
	TInplaceArray< V3f, 16 >	transformedPositions;
	transformedPositions.setNum(_numPoints);

	for( UINT i = 0; i < _numPoints; i++ ) {
		transformedPositions[i] = M44_TransformPoint( m_worldTransform, _points[i] );
	}

	m_client->addCell(
		_bounds.transformed( m_worldTransform ),
		_signMask,
		_numPoints,
		transformedPositions.raw(),
		_normals,
		_clamped
	);
}

void DebugViewProxy::addLabel(
	const V3f& _position,
	const RGBAf& _color,
	const char* _format, ...
)
{
	DEBUG_VIEW_SCOPED_LOCK;
	m_client->addLabel( M44_TransformPoint( m_worldTransform, _position ), _color, _format );
}

void DebugViewProxy::addVertex( DebugVertex & _vert )
{
	_vert.pos = M44_TransformPoint( m_worldTransform, _vert.pos );
	m_client->addVertex( _vert );
}

void DebugViewProxy::addTriangle( DebugTri & _tri )
{
	_tri.v[0] = M44_TransformPoint( m_worldTransform, _tri.v[0] );
	_tri.v[1] = M44_TransformPoint( m_worldTransform, _tri.v[1] );
	_tri.v[2] = M44_TransformPoint( m_worldTransform, _tri.v[2] );
	m_client->addTriangle( _tri );
}

void DebugViewProxy::addQuad( DebugQuad & _quad )
{
	DEBUG_VIEW_SCOPED_LOCK;
	_quad.v[0] = M44_TransformPoint( m_worldTransform, _quad.v[0] );
	_quad.v[1] = M44_TransformPoint( m_worldTransform, _quad.v[1] );
	_quad.v[2] = M44_TransformPoint( m_worldTransform, _quad.v[2] );
	_quad.v[3] = M44_TransformPoint( m_worldTransform, _quad.v[3] );
	_quad.AABBs[0] = _quad.AABBs[0].transformed( m_worldTransform );
	_quad.AABBs[1] = _quad.AABBs[1].transformed( m_worldTransform );
	_quad.AABBs[2] = _quad.AABBs[2].transformed( m_worldTransform );
	_quad.AABBs[3] = _quad.AABBs[3].transformed( m_worldTransform );
	m_client->addQuad( _quad );
}

void DebugViewProxy::addSphere( const V3f& _center, float _radius, const RGBAf& _color )
{
	const float scale = mmSqrt( M44_getScaling(m_worldTransform).lengthSquared() );
	m_client->addSphere( M44_TransformPoint( m_worldTransform, _center ), _radius * scale, _color );
}

void DebugViewProxy::clearAll()
{
	DEBUG_VIEW_SCOPED_LOCK;
	m_client->clearAll();
}

mxDEFINE_CLASS(DebugView::Options);
mxBEGIN_REFLECTION(DebugView::Options)
	mxMEMBER_FIELD(drawBounds),
	mxMEMBER_FIELD(drawDualGrid),
	mxMEMBER_FIELD(drawVoxelGrid),
	mxMEMBER_FIELD(drawTextLabels),

	mxMEMBER_FIELD(drawHermiteData),
	//mxMEMBER_FIELD(drawCellVertices),
	mxMEMBER_FIELD(drawClampedVertices),
	mxMEMBER_FIELD(drawSharpFeatures),

	mxMEMBER_FIELD(drawOctree),
	mxMEMBER_FIELD(drawClipMaps),
	mxMEMBER_FIELD(drawAuxiliary),

	mxMEMBER_FIELD(clamped_vertices_radius),
	mxMEMBER_FIELD(sharp_features_radius),
	mxMEMBER_FIELD(cell_vertices_radius),
	mxMEMBER_FIELD(cell_corners_radius),
	mxMEMBER_FIELD(length_of_normals),

	mxMEMBER_FIELD(colored_points_radius),
mxEND_REFLECTION;
DebugView::Options::Options()
{
	drawTextLabels = true;

	clamped_vertices_radius = 0.2f;
	sharp_features_radius = 0.3f;
	cell_vertices_radius = 0.4f;
	cell_corners_radius = 0.2f;
	length_of_normals = 1.0f;

	colored_points_radius = 0.2f;
}

DebugView::DebugView( AllocatorI & _allocator )
	: m_coloredLines(_allocator)
	, _text_labels(_allocator)
	, m_coloredPoints(_allocator)
	, m_clampedVerts(_allocator)
	, m_sharpFeatures(_allocator)
	, m_normals(_allocator)
	, m_dualCells(_allocator)
	, m_verts(_allocator)
	, m_tris(_allocator)
	, m_quads(_allocator)
	, m_hitVerts(_allocator)
	, m_hitTris(_allocator)
	, m_hitQuads(_allocator)
	, dualEdges(_allocator)
	, wireframeAABBs(_allocator)
	, transparentAABBs(_allocator)
	, m_spheres(_allocator)
{
}

void DebugView::clearAll()
{
	_text_labels.RemoveAll();
	m_coloredPoints.RemoveAll();

	m_coloredLines.RemoveAll();
	m_clampedVerts.RemoveAll();
	m_sharpFeatures.RemoveAll();
	m_normals.RemoveAll();
	wireframeAABBs.RemoveAll();
	transparentAABBs.RemoveAll();
	m_dualCells.RemoveAll();
	m_spheres.RemoveAll();

	m_verts.RemoveAll();
	m_tris.RemoveAll();
	m_quads.RemoveAll();

	m_hitVerts.RemoveAll();
	m_hitTris.RemoveAll();
	m_hitQuads.RemoveAll();
}

void DebugView::addLine( const V3f& _start, const V3f& _end, const RGBAf& _color )
{
	SColoredLine	newLine;
	newLine.start = _start;
	newLine.end = _end;
	newLine.color = _color;
	m_coloredLines.add( newLine );
}
void DebugView::addPoint( const V3f& _point, const RGBAf& _color, float _scale /*= 1.0f*/ )
{
	SColoredPoint	newPoint;
	newPoint.position = _point;
	newPoint.scale = _scale;
	newPoint.color = _color;
	m_coloredPoints.add( newPoint );
}
void DebugView::addClampedVertex( const V3f& _origPos, const V3f& _clampedPos )
{
	m_clampedVerts.add( std::make_pair( _origPos, _clampedPos ) );
}
void DebugView::addSharpFeatureVertex( const V3f& _point, bool _isCorner )
{
	m_sharpFeatures.add( std::make_pair( _point, _isCorner ) );
}
void DebugView::addPointWithNormal( const V3f& _point, const V3f& _normal )
{
	mxASSERT(V3_IsNormalized(_normal));
	m_normals.add( std::make_pair( _point, _normal ) );
}
void DebugView::addBox( const AABBf& _bounds, const RGBAf& _color )
{
	wireframeAABBs.add( std::make_pair( _bounds, _color ) );
}
void DebugView::addTransparentBox( const AABBf& _bounds, const float _rgba[4] )
{
	transparentAABBs.add( std::make_pair( _bounds, RGBAf::fromFloatPtr( _rgba ) ) );
}

void DebugView::addCell(
	const AABBf& _bounds,
	const U8 _signMask,
	const int _numPoints,
	const V3f* _points,
	const V3f* _normals,
	bool _clamped
)
{
	SDualCell	dualCell;
	dualCell.bounds = _bounds.scaled(0.99f);	// so that edges of neighboring cells are visible too
	//dualCell.vertex = _vertex;
	dualCell.signs = _signMask;
	dualCell.clamped = _clamped;
	mxASSERT(_numPoints <= SDualCell::MAX_POINTS);
	dualCell.numPoints = smallest( _numPoints, SDualCell::MAX_POINTS ); 
	TCopyArray(dualCell.points, _points, dualCell.numPoints);
	TCopyArray(dualCell.normals, _normals, dualCell.numPoints);

	m_dualCells.add( dualCell );
}

void DebugView::addLabel(
	const V3f& _position,
	const RGBAf& _color,
	const char* _format, ...
	)
{
	char	textBuffer[ 64 ];

	va_list argList;
	va_start( argList, _format );
	_vsnprintf_s( textBuffer, sizeof(textBuffer), _TRUNCATE, _format, argList );
	va_end( argList );

	textBuffer[ sizeof(textBuffer)-1 ] = '\0';

	TextLabel	new_vertex;
	{
		new_vertex.position = _position;
		new_vertex.normal_for_culling = CV3f(1);
		new_vertex.color = _color;
		Str::CopyS( new_vertex.text, textBuffer );
	}
	_text_labels.add( new_vertex );
}

void DebugView::addText(
						const V3f& position,
						const RGBAf& color,
						const V3f* normal_for_culling,
						const float font_scale,	// 1 by default
						const char* fmt, ...
						)
{
	char	textBuffer[ 256 ];

	va_list argList;
	va_start( argList, fmt );
	_vsnprintf_s( textBuffer, sizeof(textBuffer), _TRUNCATE, fmt, argList );
	va_end( argList );

	textBuffer[ sizeof(textBuffer)-1 ] = '\0';

	TextLabel	new_vertex;
	{
		new_vertex.position = position;
		new_vertex.normal_for_culling = normal_for_culling ? *normal_for_culling : CV3f(0);
		new_vertex.font_scale = font_scale;
		new_vertex.color = color;
		Str::CopyS( new_vertex.text, textBuffer );
	}
	_text_labels.add( new_vertex );
}

void DebugView::addVertex( DebugVertex & _vert )
{
	m_verts.add( _vert );
}
void DebugView::addTriangle( DebugTri & _tri )
{
	m_tris.add( _tri );
}
void DebugView::addQuad( DebugQuad & _quad )
{
	m_quads.add( _quad );
}

void DebugView::addSphere( const V3f& _center, float _radius, const RGBAf& _color )
{
	Sphere	newSphere;
	newSphere.center = _center;
	newSphere.radius = _radius;
	newSphere.color = _color;
	m_spheres.add( newSphere );
}

//template< typename REAL >	// prefer the 'double' type
bool RayHitsSphere(
				   const V3f& _rayOrigin, const V3f& _rayDirection,
				   const V3f& _sphereCenter, const float _sphereRadius
				   )
{
	float a = V3_LengthSquared( _rayDirection );
	float b = _rayDirection * (_rayOrigin - _sphereCenter) * 2.0;
	float c = V3_LengthSquared( _rayOrigin - _sphereCenter ) - _sphereRadius * _sphereRadius;
	float d = b*b - 4.0*a*c;
	return d >= 0;
}

void DebugView::CastDebugRay( const V3f& _localOrigin, const V3f& _direction )
{
	m_hitVerts.RemoveAll();
	m_hitTris.RemoveAll();
	m_hitQuads.RemoveAll();

#if 0
	// Test vertices.
	if( m_verts.num() )
	{
		float minT = BIG_NUMBER;
		int minIdx = -1;

		for( int iVert = 0; iVert < m_verts.num(); iVert++ )
		{
			const DebugVertex& vert = m_verts[ iVert ];
			float	hitTime;
			V3f		hitPos;
			if( Sphere_IntersectsRay(vert.pos, vert.radius, _localOrigin, _direction, 999.0f, hitTime, hitPos) )
			{
				if( hitTime < minT ) {
					minT = hitTime;
					minIdx = iVert;
				}
			}
		}//For each vertex.

		if( minIdx != -1 )
		{
			const DebugVertex& hitVert = m_verts[ minIdx ];
			if( hitVert.msg.Length() ) {
				const char* s = hitVert.msg.c_str();
				ptWARN("HitVertex[%u]: %s", minIdx, s);
			}
			m_hitVerts.add( minIdx );
		}
	}//Verts
#endif

	// Test triangles.
	if( m_tris.num() )
	{
		float minT = BIG_NUMBER;

		for( UINT iTri = 0; iTri < m_tris.num(); iTri++ )
		{
			const DebugTri& tri = m_tris[ iTri ];

			float t = BIG_NUMBER;
			if( RayTriangleIntersection( tri.v[0], tri.v[1], tri.v[2], _localOrigin, _direction, &t ) )
			{
				HitPrimitive hitTri;
				hitTri.index = iTri;
				hitTri.time = t;
				m_hitTris.add( hitTri );

				if( t < minT ) {
					minT = t;
				}
			}
		}//for

		if( m_hitTris.num() )
		{
			DBGOUT("Hit %u tris:", m_hitQuads.num());

			std::sort( m_hitTris.begin(), m_hitTris.end() );

			if( m_hitTris.num() > 1 ) {
				mxBeep(500, 100);
			}
			m_hitTris.setNum(1);

			for( UINT iHitTri = 0; iHitTri < m_hitQuads.num(); iHitTri++ )
			{
				const HitPrimitive& triRef = m_hitQuads[ iHitTri ];
				const DebugTri& tri = m_tris[ triRef.index ];

				if( tri.label.length() ) {
					const char* s = tri.label.c_str();
					ptWARN("[%u]: %s", iHitTri, s);
				}
			}//For each hit triangle
		}//if( m_hitTris.num() )
	}//Tris

	if( m_quads.num() )
	{
		float minT = BIG_NUMBER;

		for( UINT iQuad = 0; iQuad < m_quads.num(); iQuad++ )
		{
			const DebugQuad& quad = m_quads[ iQuad ];

			float t1 = BIG_NUMBER, t2 = BIG_NUMBER;
			if( RayTriangleIntersection( quad.v[0], quad.v[1], quad.v[2], _localOrigin, _direction, &t1 )
				||
				RayTriangleIntersection( quad.v[0], quad.v[2], quad.v[3], _localOrigin, _direction, &t2 )
				)
			{
				float t = minf(t1,t2);

				HitPrimitive hitQuad;
				hitQuad.index = iQuad;
				hitQuad.time = t;
				m_hitQuads.add( hitQuad );

				if( t < minT ) {
					minT = t;
					//_debugData.iSelectedQuad = iQuad;
				}
			}
		}//for

		if( m_hitQuads.num() )
		{
			std::sort( m_hitQuads.begin(), m_hitQuads.end() );

			if( m_hitQuads.num() > 1 ) {
				mxBeep(500, 1000);
				DBGOUT("\nHit %u quads!:", m_hitQuads.num());
				//m_hitQuads.setNum(1);
			} else {
				DBGOUT("\nHit a quad:");
			}

			for( UINT iHitQuad = 0; iHitQuad < m_hitQuads.num(); iHitQuad++ )
			{
				const HitPrimitive& quadRef = m_hitQuads[ iHitQuad ];
				const DebugQuad& quad = m_quads[ quadRef.index ];

				if( quad.label.length() ) {
					const char* s = quad.label.c_str();
					ptWARN("[%u]: %s", iHitQuad, s);
				}
				ptWARN("[%u]: edge %u, signs: %u, %u, %u, %u",
					iHitQuad, quad.edge, quad.signs[0], quad.signs[1], quad.signs[2], quad.signs[3]);

				//DBGOUT("[%u]: cells: %u, %u, %u, %u, verts: %u, %u, %u, %u",
				//	iHitQuad, quad.cells[0], quad.cells[1], quad.cells[2], quad.cells[3],
				//	quad.vtxind[0], quad.vtxind[1], quad.vtxind[2], quad.vtxind[3]);

				DBGOUT("[%u]: Morton codes: %u, %u, %u, %u",
					iHitQuad, quad.codes[0], quad.codes[1], quad.codes[2], quad.codes[3]);
UNDONE;
				//LogStream(LL_Warn) << "Unpacked Morton codes: "
				//	<< Morton32_DecodeCell(quad.codes[0]) << ","
				//	<< Morton32_DecodeCell(quad.codes[1]) << ","
				//	<< Morton32_DecodeCell(quad.codes[2]) << ","
				//	<< Morton32_DecodeCell(quad.codes[3]) << ";";

				//DBGOUT("[%u]: Vertex counts: %u, %u, %u, %u",
				//	iHitQuad, quad.vtxnum[0], quad.vtxnum[1], quad.vtxnum[2], quad.vtxnum[3]);

				//DBGOUT("[%u]: vertex indices: %u, %u, %u, %u",
				//	iHitQuad, quad.vtxind[0], quad.vtxind[1], quad.vtxind[2], quad.vtxind[3]);

				//LogStream(LL_Info) << "[" << iHitQuad << "]: positions: "
				//	<< quad.v[0] << ", " << quad.v[1] << ", " << quad.v[2] << ", " << quad.v[3];

				//LogStream(LL_Info) << "[" << iHitQuad << "]: positions2: "
				//	<< quad.v2[0] << ", " << quad.v2[1] << ", " << quad.v2[2] << ", " << quad.v2[3];
			}//For each hit quad
		}//if( m_hitQuads.num() )
	}//Quads
}

void DebugView::Draw(
				  const NwView3D& _camera,
				  TbPrimitiveBatcher & _renderer,
				  const InputState& input_state,
				  const Options& _options /*= Options()*/
				  ) const
{
	_renderer.PushState();

	// enable depth testing
	NwRenderState * renderState = nil;
	Resources::Load(renderState, MakeAssetID("default"));
	if( renderState ) {
		_renderer.SetStates( *renderState );
	}

	this->draw_internal(_camera, _renderer, _options);

	_renderer.PopState();
}

void DebugView::draw_internal(
	const NwView3D& _camera,
	TbPrimitiveBatcher & _renderer,
	const Options& _options
	) const
{
	const V3f up = _camera.getUpDir();

	if( _options.drawClampedVertices )
	{
		for( UINT i = 0; i < m_clampedVerts.num(); i++ )
		{
			const std::pair< V3f, V3f >& p = m_clampedVerts[ i ];
			//AuxVertex v;
			//v.xyz = point;
			//v.rgba.v = RGBAf::RED.ToRGBAi().u;
			//_renderer.DrawPoint( v );
//			_renderer.DrawSolidCircle( p.first, _camera.right_dir, up, RGBAf::RED, _options.clamped_vertices_radius, 4 );
//			_renderer.DrawSolidCircle( p.second, _camera.right_dir, up, RGBAf::BLUE, _options.clamped_vertices_radius, 4 );
			_renderer.DrawLine( p.first, p.second, RGBAf::WHITE, RGBAf::RED );
		}
	}

	if( _options.drawSharpFeatures )
	{
		for( UINT i = 0; i < m_sharpFeatures.num(); i++ )
		{
			const std::pair< V3f, bool >& pair = m_sharpFeatures[ i ];
			const bool isSharpCorner = pair.second;
			_renderer.DrawSolidCircle( pair.first, _camera.right_dir, up, isSharpCorner ? RGBAf::RED : RGBAf::YELLOW, _options.sharp_features_radius, 4 );
		}
	}

	for( UINT i = 0; i < m_normals.num(); i++ )
	{
		const std::pair< V3f, V3f >& pair = m_normals[ i ];
		const RGBAf lineColor = RGBAf::fromVec(Vector4_Set( pair.second, 1.0f ));
		_renderer.DrawLine( pair.first, pair.first + pair.second * _options.length_of_normals, RGBAf::WHITE, lineColor );
	}

	if( _options.drawHermiteData )
	{
		const NwView3D& debugCamera = this->savedCameraParams;
		for( U32 iCell = 0; iCell < m_dualCells.num(); iCell++ )
		{
			const SDualCell& cell = m_dualCells[ iCell ];

#if 0
			float tnear = +BIG_NUMBER, tfar = -BIG_NUMBER;
			if( -1 != intersectRayAABB( cell.bounds.mins, cell.bounds.maxs, debugCamera.origin, debugCamera.look, tnear, tfar )
				&& tnear < 5
				)
#endif
			{
				//_renderer.DrawAABB( cell.bounds, RGBAf::ORANGE );

				// show active edges
				UNDONE;
#if 0
				const U32 edgeMask = CUBE_ACTIVE_EDGES[ cell.signs ];
				for( int iEdge = 0; iEdge < 12; iEdge++ )
				{
					const UINT iCornerA = CUBE_EDGE[iEdge][0];
					const UINT iCornerB = CUBE_EDGE[iEdge][1];

					const V3f cornerA = {
						(iCornerA & MASK_POS_X) ? cell.bounds.max_corner.x : cell.bounds.min_corner.x,
						(iCornerA & MASK_POS_Y) ? cell.bounds.max_corner.y : cell.bounds.min_corner.y,
						(iCornerA & MASK_POS_Z) ? cell.bounds.max_corner.z : cell.bounds.min_corner.z
					};
					const V3f cornerB = {
						(iCornerB & MASK_POS_X) ? cell.bounds.max_corner.x : cell.bounds.min_corner.x,
						(iCornerB & MASK_POS_Y) ? cell.bounds.max_corner.y : cell.bounds.min_corner.y,
						(iCornerB & MASK_POS_Z) ? cell.bounds.max_corner.z : cell.bounds.min_corner.z
					};

					const RGBAf edgeColor( ( edgeMask & BIT(iEdge) ) ? RGBAf::RED : RGBAf::GREEN );

					_renderer.DrawLine( cornerA, cornerB, edgeColor, edgeColor );
				}
#endif

				// draw intersection points with normals
				for( UINT i = 0; i < cell.numPoints; i++ )
				{
					const V3f point = cell.points[ i ];
					const V3f normal = cell.normals[ i ];
					const RGBAf lineColor = RGBAf::fromVec(Vector4_Set( normal, 1.0f ));
					_renderer.DrawLine( point, point + normal * _options.length_of_normals, RGBAf::WHITE, lineColor );
				}

				// draw the cell's corners
				for( UINT iCorner = 0; iCorner < 8; iCorner++ )
				{
					if( cell.signs & BIT(iCorner) )
					{
						// this corner is inside the surface
						const V3f cornerPosition = {
							(iCorner & MASK_POS_X) ? cell.bounds.max_corner.x : cell.bounds.min_corner.x,
							(iCorner & MASK_POS_Y) ? cell.bounds.max_corner.y : cell.bounds.min_corner.y,
							(iCorner & MASK_POS_Z) ? cell.bounds.max_corner.z : cell.bounds.min_corner.z
						};
						_renderer.DrawSolidCircle( cornerPosition, _camera.right_dir, up, RGBAf::indigo, _options.cell_corners_radius, 4 );
					}
				}

				// draw the cell's representative vertex
				//_renderer.DrawSolidCircle( cell.vertex, _camera.right_dir, up, cell.clamped ? RGBAf::RED : RGBAf::GREEN, _options.cell_vertices_radius, 4 );
			}
		}//for
	}//draw dual cells




	//for( UINT i = 0; i < _text_labels.num(); i++ )
	//{
	//	const TextLabel& point = _text_labels[ i ];
	//	_renderer.DrawSolidCircle( point.position, _camera.right_dir, up, point.color, 0.1f, 4 );
	//}


	for( UINT i = 0; i < m_coloredPoints.num(); i++ )
	{
		const SColoredPoint& point = m_coloredPoints[ i ];
		_renderer.DrawSolidCircle( point.position, _camera.right_dir, up, point.color, _options.colored_points_radius * point.scale, 4 );
	}

	// red orange yellow green blue indigo violet
	const RGBAf rainbow[7] = {
		RGBAf::RED, RGBAf::ORANGE, RGBAf::YELLOW, RGBAf::GREEN, RGBAf::blue, RGBAf::indigo, RGBAf::violet
	};

	for( UINT i = 0; i < m_tris.num(); i++ )
	{
		const DebugTri& tri = m_tris[ i ];
		AuxVertex v[3];
		v[0].xyz = tri.v[0];
		v[1].xyz = tri.v[1];
		v[2].xyz = tri.v[2];
		const RGBAf color = rainbow[ i%mxCOUNT_OF(rainbow) ];
		v[0].rgba.v = color.ToRGBAi().u;
		v[1].rgba.v = color.ToRGBAi().u;
		v[2].rgba.v = color.ToRGBAi().u;
		_renderer.DrawSolidTriangle3D( v[0], v[1], v[2] );
	}

	for( UINT i = 0; i < this->adjacentPoints.num(); i++ )
	{
		const V3f& point = this->adjacentPoints[ i ];
		RGBAf color = (i > 0) ? RGBAf::RED : RGBAf::YELLOW;
		_renderer.DrawSolidCircle( point, _camera.right_dir, up, color, 0.1, 4 );
	}

	for( UINT i = 0; i < this->dualEdges.num(); i++ )
	{
		const std::pair< V3f, V3f >& edge = this->dualEdges[ i ];
		_renderer.DrawLine( edge.first, edge.second, RGBAf::RED, RGBAf::RED );
	}

	for( UINT i = 0; i < m_coloredLines.num(); i++ )
	{
		const SColoredLine& line = m_coloredLines[ i ];
		_renderer.DrawLine( line.start, line.end, line.color );
	}

	if( _options.drawBounds )
	{
		for( UINT i = 0; i < this->wireframeAABBs.num(); i++ )
		{
			const std::pair< AABBf, RGBAf >& bounds = this->wireframeAABBs[ i ];
			_renderer.DrawAABB( bounds.first, bounds.second );
		}
	}

	for( UINT i = 0; i < m_spheres.num(); i++ )
	{
		const Sphere& sphere = m_spheres[ i ];
		_renderer.DrawCircle( sphere.center, _camera.right_dir, up, sphere.color, sphere.radius );
	}
	

	for( UINT iQuad = 0; iQuad < m_hitQuads.num(); iQuad++ )
	{
		const HitPrimitive& quadRef = m_hitQuads[ iQuad ];
		const DebugQuad& quad = m_quads[ quadRef.index ];

		AuxVertex	v0, v1, v2, v3;
		v0.xyz = quad.v[0];
		v1.xyz = quad.v[1];
		v2.xyz = quad.v[2];
		v3.xyz = quad.v[3];
		v0.rgba.v = RGBAf::RED.ToRGBAi().u;
		v1.rgba.v = RGBAf::RED.ToRGBAi().u;
		v2.rgba.v = RGBAf::RED.ToRGBAi().u;
		v3.rgba.v = RGBAf::RED.ToRGBAi().u;
		_renderer.DrawWireQuad3D( v0, v1, v2, v3 );

		_renderer.DrawAABB(quad.AABBs[0], RGBAf::RED);
		_renderer.DrawAABB(quad.AABBs[1], RGBAf::GREEN);
		_renderer.DrawAABB(quad.AABBs[2], RGBAf::BLUE);
		_renderer.DrawAABB(quad.AABBs[3], RGBAf::ORANGE);

		const V3f up = _camera.getUpDir();
		_renderer.DrawSolidCircle( quad.v[0], _camera.right_dir, up, RGBAf::RED, _options.cell_vertices_radius, 4 );
		_renderer.DrawSolidCircle( quad.v[1], _camera.right_dir, up, RGBAf::GREEN, _options.cell_vertices_radius, 4 );
		_renderer.DrawSolidCircle( quad.v[2], _camera.right_dir, up, RGBAf::BLUE, _options.cell_vertices_radius, 4 );
		_renderer.DrawSolidCircle( quad.v[3], _camera.right_dir, up, RGBAf::ORANGE, _options.cell_vertices_radius, 4 );
	}

	for( UINT iHitVert = 0; iHitVert < m_hitVerts.num(); iHitVert++ )
	{
		const DebugVertex& hitVert = m_verts[ iHitVert ];
		_renderer.DrawSolidCircle( hitVert.pos, _camera.right_dir, up, RGBAf::RED, hitVert.radius, 8 );
	}
}

void Dbg_ControlDebugView( ADebugView & _dbgView, const InputState& _inputState )
{
	if( (_inputState.modifiers & KeyModifier_Ctrl) )
	{
		if( _inputState.keyboard.held[KEY_X] ) {
			_dbgView.clearAll();
		}
	}
}

void DbgShowVolume(
				   const AVolumeSampler& _volume
				   , const AABBf& _volumeBounds
				   , const int resolution_cells
				   , VX::ADebugView &_debugView
				   , bool showVoxels
				   )
{
	const V3f cellSize = _volumeBounds.size() / (float)resolution_cells;
	for( int iCellZ = 0; iCellZ < resolution_cells; iCellZ++ )	// for each horizontal layer
	{
		for( int iCellY = 0; iCellY < resolution_cells; iCellY++ )
		{
			for( int iCellX = 0; iCellX < resolution_cells; iCellX++ )
			{
				AABBf cellBounds;
				cellBounds.min_corner = V3_Multiply( CV3f( iCellX, iCellY, iCellZ ), cellSize );
				cellBounds.max_corner = cellBounds.min_corner + cellSize;

				QEF_Solver::Input solverInput;	// HermiteDataSample volumeSample;
				const U32 cornerSigns = _volume.SampleHermiteData(
					iCellX, iCellY, iCellZ,
					solverInput
					);

				if( vxNONTRIVIAL_CELL( cornerSigns ) )	// If this is an active cell.
				{
					_debugView.addCell(cellBounds, cornerSigns, solverInput.num_points, solverInput.positions, solverInput.normals, false);

				}//If the current cell is active.

				const U32 boundaryFacesMask =
					( (iCellX == resolution_cells-1) << CubeFace::PosX ) |
					( (iCellX == 0)                  << CubeFace::NegX ) |
					( (iCellY == resolution_cells-1) << CubeFace::PosY ) |
					( (iCellY == 0)                  << CubeFace::NegY ) |
					( (iCellZ == resolution_cells-1) << CubeFace::PosZ ) |
					( (iCellZ == 0)                  << CubeFace::NegZ ) ;

				if(showVoxels && (cornerSigns || boundaryFacesMask))
				{
					_debugView.addCell(cellBounds, cornerSigns, 0, nil, nil, false);
				}

			}//For each cell along X.
		}//For each cell along Y.
	}//For each cell along Z.
}

const RGBAf dbg_cellColorInQuad( const UINT cell_index_in_quad )
{
	static const RGBAf quad_cell_colors[4] = {
		RGBAf::RED,
		RGBAf::GREEN,
		RGBAf::BLUE,
		RGBAf::YELLOW//ORANGE
	};
	return quad_cell_colors[ cell_index_in_quad ];
}


TPtr< ADebugView >	g_DbgView = &ADebugView::ms_dummy;

namespace Dbg
{
	// From SpaceEngineers, VRage engine [2015]
	const float LOD_COLORS[16][4] =
	{
		{  1, 0, 0, 1 },
		{  0, 1, 0, 1 },
		{  0, 0, 1, 1 },

		{  1, 1, 0, 1 },
		{  0, 1, 1, 1 },
		{  1, 0, 1, 1 },

		{  0.5f, 0, 1, 1 },
		{  0.5f, 1, 0, 1 },
		{  1, 0, 0.5f, 1 },
		{  0, 1, 0.5f, 1 },

		{  1, 0.5f, 0, 1 },
		{  0, 0.5f, 1, 1 },

		{  0.5f, 1, 1, 1 },
		{  1, 0.5f, 1, 1 },
		{  1, 1, 0.5f, 1 },
		{  0.5f, 0.5f, 1, 1 },
	};

//Color.Red,
//Color.Green,
//Color.Blue,
//Color.Yellow,
//Color.Purple,
//Color.Cyan,
//Color.White,
//Color.CornflowerBlue,
//Color.Chartreuse,
//Color.Coral,

	const U32 g_LoD_colors[8] =
	{
		RGBAi::RED,
		RGBAi::GREEN,
		RGBAi::BLUE,
		RGBAi::YELLOW,

		RGBAi::MAGENTA,
		RGBAi::DARKOLIVEGREEN,
		RGBAi::VIOLET,
		RGBAi::ORANGE,
	};

	//const U32 g_OctreeLevelColors[8] = {
	//	RGBAi::RED,
	//	RGBAi::ORANGE,
	//	RGBAi::YELLOW,
	//	RGBAi::GREEN,
	//	RGBAi::LIGHTBLUE,
	//	RGBAi::BLUE,
	//	RGBAi::VIOLET,
	//	RGBAi::WHITE,
	//};

	const U32 g_OctantColors[8] =
	{
		RGBAi::RED,
		RGBAi::GREEN,
		RGBAi::BLUE,
		RGBAi::YELLOW,

		RGBAi::ORANGE,
		RGBAi::LIGHTSEAGREEN,//LIGHT_YELLOW_GREEN,
		RGBAi::VIOLET,//MAGENTA,
		RGBAi::GRAY,
	};

	const U32 g_QuadrantColors[4] =
	{
		RGBAi::RED,
		RGBAi::GREEN,
		RGBAi::BLUE,
		RGBAi::ANTIQUEWHITE,
	};

	const U32 g_DualVertexColors[4] =
	{
		//RGBAi::YELLOW,
		//RGBAi::GREEN,
		//RGBAi::BLUE,
		//RGBAi::RED,
		RGBAi::GREEN,
		RGBAi::YELLOW,
		RGBAi::ORANGE,
		RGBAi::RED,
	};

	const U32 g_cellTypeColors[1+6+12] =
	{
		RGBAi::WHITE,	// ECellType::Internal
		// boundary vertices on chunk faces:
		RGBAi::RED,	// FacePosX, // +X
		RGBAi::GREEN,	// FaceNegX, // -X
		RGBAi::BLUE,	// FacePosY, // +Y
		RGBAi::YELLOW,	// FaceNegY, // -Y
		RGBAi::Pink,	// FacePosZ, // +Z
		RGBAi::SeaGreen,	// FaceNegZ, // -Z
		// boundary vertices on chunk edges:
		RGBAi::DarkBlue,	// Edge0,
		RGBAi::DarkCyan,	// Edge1,
		RGBAi::DarkGoldenrod,	// Edge2,
		RGBAi::DarkGray,	// Edge3,
		RGBAi::DarkGreen,	// Edge4,
		RGBAi::DarkMagenta,	// Edge5,
		RGBAi::DarkOrange,	// Edge6,
		RGBAi::DarkRed,	// Edge7,
		RGBAi::DarkSalmon,	// Edge8,
		RGBAi::DarkSeaGreen,	// Edge9,
		RGBAi::DarkTurquoise,	// EdgeA,
		RGBAi::DarkViolet,	// EdgeB,
	};

	const U32 g_cubeFaceColors[6] =
	{
		RGBAi::RED,	// FacePosX, // +X
		RGBAi::GREEN,	// FaceNegX, // -X
		RGBAi::BLUE,	// FacePosY, // +Y
		RGBAi::YELLOW,	// FaceNegY, // -Y
		RGBAi::WHITE,	// FacePosZ, // +Z
		RGBAi::BLACK,	// FaceNegZ, // -Z
	};
}//namespace Dbg

}//namespace VX

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
