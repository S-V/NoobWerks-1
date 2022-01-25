// Voxel utilities.
// @todo: move debug code somewhere else
#pragma once

#include <utility>	// std::pair
#include <Base/Math/Geometry/CubeGeometry.h>
#include <Base/Template/Containers/Blob.h>
#include <Base/Util/Color.h>	// VX::DebugView
#include <Core/VectorMath.h>	// NwView3D

class InputState;	// in Driver
class TbPrimitiveBatcher;	// in Graphics


namespace VX
{

/// Grid context - precomputed constants for indexing into 3D arrays when sampling a voxel/cell grid.
struct GridCtx
{
	// precomputed 'row' and 'deck' values (see Eric Lengyel's PhD, C4 engine):
	// so that instead of:
	// iLinearIndex = ((SIZE_X * SIZE_Y) * iZ) + (SIZE_X * iY) + iX (3 muls and 3 adds, can be made 2 muls and 2 adds by grouping common subexpressions)
	// we can do this:
	// iLinearIndex = (deckSize * iZ) + (rowSize * iY) + iX (2 muls and 2 adds)
	const U32	size1D;	//!< precomputed row size: (SIZE_X) (equals SIZE_Y and SIZE_Z) (in voxels or cells)
	const U32	size2D;	//!< precomputed deck size: (SIZE_X * SIZE_Y)
public:
	GridCtx( const UINT _gridDim )
		: size1D( _gridDim )
		, size2D( size1D * size1D )
	{}
	/// Linearizes the given cell/voxel coordinates: calculates an index into a 1D array of cells/voxels.
	mxFORCEINLINE UINT getIndex1D( UINT iX, UINT iY, UINT iZ ) const
	{
		return size2D * iZ + size1D * iY + iX;
	}
	/// Converts a flat index into a tuple of coordinates.
	mxFORCEINLINE UInt3 getIndex3D( UINT _flatIndex ) const
	{
		U32 rem = _flatIndex;

		U32 iZ = _flatIndex / size2D;
		rem -= iZ * size2D;

		U32 iY = rem / size1D;
		rem -= iY * size1D;

		U32 iX = rem;

		return UInt3( iX, iY, iZ );
	}

	template< class XYZ >
	mxFORCEINLINE UINT getIndex1D( const XYZ& _xyz ) const
	{
		return size2D * _xyz.z + size1D * _xyz.y + _xyz.x;
	}
};

/// Defines some common constants and functions.
/// Doesn't provide any storage.
/// Finally, it can be used to convert coordinate indices
/// to ordinal offsets and vice versa.
template< int _DIM_X, int _DIM_Y, int _DIM_Z >
struct Array3D_Traits
{
	enum
	{
		/// total dimensions of the data, including padding
		DIM_X = _DIM_X, DIM_Y = _DIM_Y, DIM_Z = _DIM_Z,
	};
	static const U32 TotalCount = DIM_X * DIM_Y * DIM_Z;

	static inline U32 FlattenIndex( U32 iX, U32 iY, U32 iZ )
	{
		return GetFlattenedIndex< DIM_X, DIM_Y, DIM_Z >( iX, iY, iZ );
	}
	static inline void UnravelFlattenedIndex( U32 flatIndex, U32 &iX, U32 &iY, U32 &iZ )
	{
		UnravelFlattenedIndex< DIM_X, DIM_Y, DIM_Z >( flatIndex, iX, iY, iZ );
	}
};

/// Represents a 3D, orthogonal block of elements.
/// The volume can be padded so that it can be indexed with negative voxel coordinates.
template<
	typename VOXEL,	// type of the stored elements
	class VOLUME_TRAITS,	// 'core' data dimensions, without padding
	// dimensions of the border area (padding):
	int _PAD_NEG_X, int _PAD_NEG_Y, int _PAD_NEG_Z,// the thickness of padding space at the negative boundary (-X,-Y,-Z axes)
	int _PAD_POS_X, int _PAD_POS_Y, int _PAD_POS_Z	// pad thickness at the positive boundaries (+X,+Y,+Z)
>
struct mxPREALIGN(16) TPaddedVolume
	: Array3D_Traits
	<
		// total size of the volume (including padding)
		_PAD_NEG_X + VOLUME_TRAITS::DIM_X + _PAD_POS_X,
		_PAD_NEG_Y + VOLUME_TRAITS::DIM_Y + _PAD_POS_Y,
		_PAD_NEG_Z + VOLUME_TRAITS::DIM_Z + _PAD_POS_Z
	>
{
	VOXEL	data[ DIM_X * DIM_Y * DIM_Z ];
//NOTE: must not define any more member fields for binary compatibility with a plain 3D array
public:
	enum {
		//@todo: refactor
		/// valid ranges for absolute (unsigned) voxel indices
		CORE_X = VOLUME_TRAITS::DIM_X,
		CORE_Y = VOLUME_TRAITS::DIM_Y,
		CORE_Z = VOLUME_TRAITS::DIM_Z,
		PAD_NEG_X = _PAD_NEG_X, PAD_NEG_Y = _PAD_NEG_Y, PAD_NEG_Z = _PAD_NEG_Z,
		PAD_POS_X = _PAD_POS_X, PAD_POS_Y = _PAD_POS_Y, PAD_POS_Z = _PAD_POS_Z,
	};

	TPaddedVolume()
	{
	}
	VOXEL& Get( int iX, int iY, int iZ )
	{
		// account for padding at chunk border
		return data[ GetAbsoluteOffset( Int3_Set( iX, iY, iZ ) ) ];
	}
	const VOXEL& Get( int iX, int iY, int iZ ) const
	{
		// account for padding at chunk border
		return data[ GetAbsoluteOffset( Int3_Set( iX, iY, iZ ) ) ];
	}
	void Set( int iX, int iY, int iZ, const VOXEL& value )
	{
		// account for padding at chunk border
		data[ GetAbsoluteOffset( Int3_Set( iX, iY, iZ ) ) ] = value;
	}
	void setAll( const VOXEL& value )
	{
		std::fill( data, data + (DIM_X * DIM_Y * DIM_Z), value);
	}

	// Sample 6 adjacent voxels
	void GetNeighbors6( int x, int y, int z, VOXEL neighbors[6] ) const
	{
		neighbors[0] = this->Get( x + 1, y, z );
		neighbors[1] = this->Get( x - 1, y, z );
		neighbors[2] = this->Get( x, y + 1, z );
		neighbors[3] = this->Get( x, y - 1, z );
		neighbors[4] = this->Get( x, y, z + 1 );
		neighbors[5] = this->Get( x, y, z - 1 );
	}
	void GetNeighbors18( int x, int y, int z, VOXEL neighbors[18] ) const
	{
		GetNeighbors6( x, y, z, neighbors );
		neighbors[ 6] = this->Get( x + 1, y + 1, z );
		neighbors[ 7] = this->Get( x + 1, y - 1, z );
		neighbors[ 8] = this->Get( x + 1, y, z + 1 );
		neighbors[ 9] = this->Get( x + 1, y, z - 1 );
		neighbors[10] = this->Get( x - 1, y + 1, z );
		neighbors[11] = this->Get( x - 1, y - 1, z );
		neighbors[12] = this->Get( x - 1, y, z + 1 );
		neighbors[13] = this->Get( x - 1, y, z - 1 );
		neighbors[14] = this->Get( x, y + 1, z + 1 );
		neighbors[15] = this->Get( x, y + 1, z - 1 );
		neighbors[16] = this->Get( x, y - 1, z + 1 );
		neighbors[17] = this->Get( x, y - 1, z - 1 );
	}
	void GetNeighbors26( int x, int y, int z, VOXEL neighbors[26] ) const
	{
		GetNeighbors18( x, y, z, neighbors );
		neighbors[18] = this->Get( x + 1, y + 1, z + 1 );
		neighbors[19] = this->Get( x - 1, y + 1, z + 1 );
		neighbors[20] = this->Get( x + 1, y + 1, z - 1 );
		neighbors[21] = this->Get( x - 1, y + 1, z - 1 );
		neighbors[22] = this->Get( x + 1, y - 1, z + 1 );
		neighbors[23] = this->Get( x - 1, y - 1, z + 1 );
		neighbors[24] = this->Get( x + 1, y - 1, z - 1 );
		neighbors[25] = this->Get( x - 1, y - 1, z - 1 );
	}

	static UInt3 GetAbsoluteCoordinates( const Int3& relativeCoords )
	{
		mxASSERT(relativeCoords.x >= -_PAD_NEG_X);
		mxASSERT(relativeCoords.y >= -_PAD_NEG_Y);
		mxASSERT(relativeCoords.z >= -_PAD_NEG_Z);
		const UInt3 result = {
			UINT(relativeCoords.x + _PAD_NEG_X),
			UINT(relativeCoords.y + _PAD_NEG_Y),
			UINT(relativeCoords.z + _PAD_NEG_Z)
		};
		return result;
	}
	static U32 GetAbsoluteOffset( const Int3& relativeCoords )
	{
		const UInt3 absolute = GetAbsoluteCoordinates( relativeCoords );
		return FlattenIndex( absolute.x, absolute.y, absolute.z );
	}

public:
	/// sequential (raster-order) iterator - written for clarity, not for speed!
	struct Iterator : Int3, NonCopyable
	{
		const TPaddedVolume &	m_volume;
		// [0..DATA_DIM)
	public:
		Iterator( const TPaddedVolume& _volume, int startX = 0, int startY = 0, int startZ = 0 )
			: m_volume( _volume )
		{
			x = startX;
			y = startY;
			z = startZ;
		}
		// returns 'true' if this iterator is valid (there are other elements after it)
		bool IsValid() const
		{
			return x < VOLUME_TRAITS::DIM_X
				&& y < VOLUME_TRAITS::DIM_Y
				&& z < VOLUME_TRAITS::DIM_Z;
		}
		VOXEL& Value() const
		{
			return m_volume.Get( x, y, z );
		}
		void MoveToNext()
		{
			++x;
			if( x == VOLUME_TRAITS::DIM_X )
			{
				x = 0;
				++y;
			}
			if( y == VOLUME_TRAITS::DIM_Y )
			{
				y = 0;
				++z;
			}
		}
		void Reset()
		{
			x = 0;
			y = 0;
			z = 0;
		}
	public:	// Overloaded operators.
		// Pre-increment.
		const Iterator& operator ++ ()
		{
			this->MoveToNext();
			return *this;
		}
		operator bool () const
		{
			return this->IsValid();
		}
		VOXEL* operator -> () const
		{
			return &this->Value();
		}
		VOXEL& operator * () const
		{
			return this->Value();
		}
	};
	//@todo: Morton iterator
public:
	typedef TPaddedVolume
	<
		VOXEL,
		VOLUME_TRAITS,
		0, 0, 0, 0, 0, 0	// no padding
	>
	VolumeWithoutPaddingT;

	/// Copy into the volume excluding padding.
	void CopyTo( VolumeWithoutPaddingT *destination ) const
	{
		VOXEL *writePointer = destination->data;
		for( Iterator it( *this ); it.IsValid(); it.MoveToNext() )
		{
			*writePointer++ = this->Get( it.x, it.y, it.z );
		}
	}

	/// Copy voxels from the volume excluding padding.
	void CopyFrom( const VolumeWithoutPaddingT& source )
	{
		const VOXEL* readPointer = source.data;
		for( Iterator it( *this ); it.IsValid(); it.MoveToNext() )
		{
			this->Set( it.x, it.y, it.z, *readPointer++ );
		}
	}

	PREVENT_COPY(TPaddedVolume);
};

#if 0
inline MaterialID GetDivergence( const PaddedVolumeT& volume, int x, int y, int z )
{
	return (data[k][j][i + 1] + data[k][j][i - 1] + data[k][j + 1][i]
	+ data[k][j - 1][i] + data[k + 1][j][i] + data[k - 1][j][i])
		- (data[k][j][i] * 6.0);
}
inline MaterialID GetAverage8( const PaddedVolumeT& volume, int x, int y, int z )
{
	return (data[k][j][i] + data[k][j][i + 1] + data[k][j + 1][i]
	+ data[k][j + 1][i + 1] + data[k + 1][j][i] + data[k + 1][j][i
		+ 1] + data[k + 1][j + 1][i] + data[k + 1][j + 1][i + 1])
		* 0.125;
}
#endif

/// The volume can be padded so that it can be indexed with negative voxel coordinates.
template<
	typename VOXEL,	// type of the stored elements
	int DIM_X, int DIM_Y,	// 'core' data dimensions, without padding
	int PADDING// dimensions of the border area (padding):
>
struct TPaddedSlice
{
	enum {
		REAL_SIZE_X = DIM_X + PADDING*2,
		REAL_SIZE_Y = DIM_Y + PADDING*2
	};

	VOXEL	data[ REAL_SIZE_X * REAL_SIZE_Y ];
//NOTE: must not define any more member fields for binary compatibility with a plain 2D array
public:
	TPaddedSlice()
	{
	}
	VOXEL& Get( int iX, int iY )
	{
		// account for padding at chunk border
		return data[ GetAbsoluteOffset( iX, iY ) ];
	}
	const VOXEL& Get( int iX, int iY ) const
	{
		// account for padding at chunk border
		return data[ GetAbsoluteOffset( iX, iY ) ];
	}
	void Set( int iX, int iY, const VOXEL& value )
	{
		// account for padding at chunk border
		data[ GetAbsoluteOffset( iX, iY ) ] = value;
	}
	void setAll( const VOXEL& value )
	{
		std::fill( data, data + sizeof(data), value );
	}

	static U32 GetAbsoluteOffset( int iX, int iY )
	{
		mxASSERT(iX >= -PADDING && iX < DIM_X+PADDING);
		mxASSERT(iY >= -PADDING && iY < DIM_Y+PADDING);
		const UINT iAbsoluteX = UINT( iX + PADDING );
		const UINT iAbsoluteY = UINT( iY + PADDING );
		return (REAL_SIZE_X * iAbsoluteY) + iAbsoluteX;
	}
};

/// Represents Hermite data for a single cubic cell.
struct HermiteDataSample
{
	/// a cube can have 12 intersecting edges at max
	enum { MAX_POINTS = 12 };

	int	num_points;	//!< == number of intersecting edges [0..12]
	V3f	positions[MAX_POINTS];	//!< intersection points
	V3f	normals[MAX_POINTS];	//!< normals at intersection points
	U32	materials[8];	//!< material IDs at the cell's corners (Z-order)

	/// cube_edge => point_index
	int	edge_remap[12];	//!< maps 'sparse' cube edges to 'dense' arrays above

public:
	HermiteDataSample()
	{
		num_points = 0;
		for( int i = 0; i < mxCOUNT_OF(edge_remap); i++ ) {
			edge_remap[i] = ~0;
		}
	}
};

/// Computes the so-called 'Hermite data' for cube-based contouring methods:
/// exact intersection points (of the contour with the edges of the cube)
/// together with their normals.
struct AVolumeSampler
{
	virtual bool IsInside(
		int iVoxelX, int iVoxelY, int iVoxelZ
	) const = 0;

	/// Examines the cell at {(x,y,z), (x+1,y+1,z+1)}
	/// and collects intersection points with their normals
	/// on active (i.e. crossing the isosurface) edges of the cell.
	/// Returns the signs at the cell's corners.
	virtual U32 SampleHermiteData(
		int iCellX, int iCellY, int iCellZ,
		HermiteDataSample & _sample
	) const = 0;

	/// Returns Hermite data (i.e. exact intersection point (as directed distance) and normal)
	/// for the given edge of the cell.
	virtual void GetEdgeIntersection(
		int iCellX, int iCellY, int iCellZ,	//!< coordinates of the cell's minimal corner (i.e. the center of the voxel)
		int iCellEdge,	//!< index of the cell's edge, [0..12)
		float &_distance, V3f &_normal	//!< directed distance along positive axis direction and unit normal
	) const
	{
		UNDONE;
	}

	/// Returns Hermite data (i.e. exact intersection point (as directed distance) and normal).
	virtual bool getEdgeIntersection(
		int iCellX, int iCellY, int iCellZ	//!< coordinates of the cell's minimal corner (or the voxel at that corner)
		, const EAxis edgeAxis	//!< axis index [0..3)
		, float &distance_
		, V3f &normal_	//!< directed distance along the positive axis direction and unit normal to the surface
	) const
	{
		UNDONE;
		return false;
	}

	//virtual bool NeedsSubdivision(
	//	int iCellX, int iCellY, int iCellZ
	//) const
	//{
	//	return true;
	//}
};

/**
 * An iterator for a cubic grid of voxels.
 *
 * The iterator traverses all voxels along a specified direction starting from
 * a given position.
 * NOTE: the starting point of the ray must lie inside the voxel grid!
 */
mxBIBREF(
"Ray casting technique described in paper:"
"A Fast Voxel Traversal Algorithm for Ray Tracing - John Amanatides, Andrew Woo [1987]"
"http://www.cse.yorku.ca/~amana/research/grid.pdf"
);
class VoxelGridIterator {
public:
	VoxelGridIterator(
		/// the start position of the ray
		const V3f& _origin,
		/// the direction of the ray
		const V3f& _direction,
		/// the side dimensions of a single cell
		const V3f& _cellSize
	);

	// Post-increment
	VoxelGridIterator operator++();

	// Dereferencing
	Int3 operator*();
	const Int3 & operator*() const;

private:
	/// The integral steps in each direction when we iterate (1, 0 or -1)
	/*const*/ Int3 m_iStep;

	/// The t value which moves us from one voxel to the next
	/*const*/ V3f m_fDeltaT;

	/// The t value which will get us to the next voxel
	V3f m_fNextT;

	/// The integer indices for the current voxel
	Int3 m_iVoxel;
};

inline
VoxelGridIterator VoxelGridIterator::operator++()
{
	// find the minimum of tMaxX, tMaxY and tMaxZ during each iteration.
	// This minimum will indicate how much we can travel along the ray and still remain in the current voxel.
	const int iMin = Min3Index( m_fNextT.x, m_fNextT.y, m_fNextT.z );

	m_iVoxel[ iMin ] += m_iStep[ iMin ];
	m_fNextT[ iMin ] += m_fDeltaT[ iMin ];

	return *this;
}

inline
Int3 VoxelGridIterator::operator*() {
	return m_iVoxel;
}

inline
const Int3 & VoxelGridIterator::operator*() const {
	return m_iVoxel;
}


/// a helper class for debug visualization
mxREFACTOR("accept double coords - the worlds can be large!");
struct ADebugView
{
	virtual void addLine( const V3f& _start, const V3f& _end, const RGBAf& _color )
	{};
	virtual void addPoint( const V3f& _point, const RGBAf& _color, float _scale = 1.0f )
	{};

	///AddWireframeAABB
	virtual void addBox( const AABBf& _bounds, const RGBAf& _color )
	{};

	///may not be properly implemented
	virtual void addTransparentBox( const AABBf& _bounds, const float _rgba[4] )
	{
		this->addBox( _bounds, RGBAf::fromFloatPtr( _rgba ) );
	}


	// Mesh errors

	virtual void addIsolatedVertex( const V3f& pos, const unsigned index );

	virtual void addEdgeIntersection(
		const V3f& edge_start, const V3f& edge_end,
		const V3f& intersection_point,
		const V3f& surface_normal
		);


	// QEF errors

	/// for showing vertices which were clamped to cell bounds (failed QEF solutions)
	virtual void addClampedVertex( const V3f& _origPos, const V3f& _clampedPos )
	{};

	/// for drawing vertices located on sharp features
	virtual void addSharpFeatureVertex( const V3f& _point, bool _isCorner = true )
	{};

	/// for visualizing Hermite data
	virtual void addPointWithNormal( const V3f& _point, const V3f& _normal )
	{};


	// Misc errors

	virtual void addMissedRayQuadIntersection(
		const V3f& segment_start
		, const EAxis ray_axis
		, const float max_ray_distance
		, const V3f quad_vertices[4]
		, const CubeMLf cells_bounds[4]
		);


	// Cell configuration

	/// for drawing dual cells with Hermite data, etc.
	virtual void addCell(
		const AABBf& _bounds,
		const U8 _signMask,
		const int _numPoints,
		const V3f* _points,
		const V3f* _normals,
		bool _clamped	//!< true if the vertex was clamped to lie inside the cube
	)
	{};

	static void addAmbiguousCell(
		ADebugView * _dbgView,
		const AABBf& _bounds,
		const U8 _signMask
	);

	// Hit testing
	struct DebugVertex {
		V3f			pos;
		float		radius;
		String64	msg;	//!< debug text
	};
	virtual void addVertex( DebugVertex & _vert )
	{}

	struct DebugTri {
		V3f			v[3];	//!< triangle vertices
		String64	label;	//!< debug text
	};
	virtual void addTriangle( DebugTri & _tri )
	{}

	/// for debugging cube-based dual contouring algorithms
	struct DebugQuad {
		V3f			v[4];		//!< quad vertices
		V3f			v2[4];		//!< normalized vertex positions
		U32			vtxind[4];	//!< vertex indices
		U8			vtxnum[4];	//!< number of dual vertices in each cell
		U8			signs[4];	//!< sign configurations
		U32			cells[4];	//!< indices of leaf cells
		U32			codes[4];	//!< 30-bit Morton codes
		AABBf		AABBs[4];	//!< cells' bounds
		String64	label;		//!< debug text
		U8			edge;		//!< cube edge
	public:
		DebugQuad() { mxZERO_OUT(*this); }
	};

	virtual void addQuad( DebugQuad & _quad )
	{}

	virtual void addSphere( const V3f& _center, float _radius, const RGBAf& _color )
	{}

	// Debug text

	mxDEPRECATED
	virtual void addLabel(
		const V3f& _position,
		const RGBAf& _color,
		const char* _format, ...
		)
	{};

	virtual void addText(
		const V3f& position,
		const RGBAf& color,
		const V3f* normal_for_culling,
		const float font_scale,	// 1 by default
		const char* fmt, ...
		)
	{}
	
	virtual void clearAll()
	{}

public:
	static ADebugView ms_dummy;	//!< 'null instance' to avoid null pointer checks
};

/// adds an absolute offset to all primitives
class DebugViewProxy : public ADebugView
{
	const M44f	m_worldTransform;	//!< local-to-world transform
	TPtr< ADebugView >	m_client;
//	SpinWait	m_CS;

public:
	DebugViewProxy( ADebugView * client, const M44f& _worldTransform );
	~DebugViewProxy();

	virtual void addLine( const V3f& _start, const V3f& _end, const RGBAf& _color ) override;
	virtual void addPoint( const V3f& _point, const RGBAf& _color, float _scale = 1.0f ) override;

	/// for showing vertices which were clamped to cell bounds (failed QEF solutions)
	virtual void addClampedVertex( const V3f& _origPos, const V3f& _clampedPos ) override;
	/// for drawing vertices located on sharp features
	virtual void addSharpFeatureVertex( const V3f& _point, bool _isCorner = true ) override;
	/// for visualizing Hermite data
	virtual void addPointWithNormal( const V3f& _point, const V3f& _normal ) override;

	virtual void addBox( const AABBf& _bounds, const RGBAf& _color ) override;
	virtual void addTransparentBox( const AABBf& _bounds, const float _rgba[4] ) override;

	/// for drawing dual cells with Hermite data, etc.
	virtual void addCell(
		const AABBf& _bounds,
		const U8 _signMask,
		const int _numPoints,
		const V3f* _points,
		const V3f* _normals,
		bool _clamped	//!< true if the vertex was clamped to lie inside the cube
	) override;

	virtual void addLabel(
		const V3f& _position,
		const RGBAf& _color,
		const char* _format, ...
	) override;

	virtual void addVertex( DebugVertex & _vert ) override;
	virtual void addTriangle( DebugTri & _tri ) override;
	virtual void addQuad( DebugQuad & _quad ) override;

	virtual void addSphere( const V3f& _center, float _radius, const RGBAf& _color ) override;

	virtual void clearAll() override;
};

struct DebugView : ADebugView, NonCopyable
{
	/// options for debug visualization of the voxel->polygons converter
	struct Options : CStruct
	{
		// General:
		bool	drawBounds;	//!< Draw AABBs?
		bool	drawDualGrid;
		bool	drawVoxelGrid;
		bool	drawTextLabels;

		// Dual Contouring:
		bool	drawHermiteData;	//!< Draw dual cells with Hermite data?
		//bool	drawCellVertices;
		bool	drawClampedVertices;
		bool	drawSharpFeatures;

		bool	drawOctree;
		bool	drawClipMaps;
		bool	drawAuxiliary;

		float	clamped_vertices_radius;
		float	sharp_features_radius;
		float	cell_vertices_radius;
		float	cell_corners_radius;
		float	length_of_normals;

		float	colored_points_radius;

	public:
		mxDECLARE_CLASS(Options, CStruct);
		mxDECLARE_REFLECTION;
		Options();
	};

public:
	DebugView( AllocatorI & _allocator );

	virtual void addLine( const V3f& _start, const V3f& _end, const RGBAf& _color ) override;
	virtual void addPoint( const V3f& _point, const RGBAf& _color, float _scale = 1.0f ) override;
	virtual void addBox( const AABBf& _bounds, const RGBAf& _color ) override;
	virtual void addTransparentBox( const AABBf& _bounds, const float _rgba[4] ) override;
	virtual void addClampedVertex( const V3f& _origPos, const V3f& _clampedPos ) override;
	virtual void addSharpFeatureVertex( const V3f& _point, bool _isCorner = true ) override;
	virtual void addPointWithNormal( const V3f& _point, const V3f& _normal ) override;
	virtual void clearAll() override;

	// Dual Contouring
	virtual void addCell(
		const AABBf& _bounds,
		const U8 _signMask,
		const int _numPoints,
		const V3f* _points,
		const V3f* _normals,
		bool _clamped
	) override;

	virtual void addLabel(
		const V3f& _position,
		const RGBAf& _color,
		const char* _format, ...
		) override;

	virtual void addText(
		const V3f& position,
		const RGBAf& color,
		const V3f* normal_for_culling,
		const float font_scale,	// 1 by default
		const char* fmt, ...
		) override;

	/// NOTE: text labels are not rendered!
	virtual void Draw(
		const NwView3D& _camera,
		TbPrimitiveBatcher & _renderer,
		const InputState& input_state,
		const Options& _options = Options()
	) const;

	virtual void addVertex( DebugVertex & _vert ) override;
	virtual void addTriangle( DebugTri & _tri ) override;
	virtual void addQuad( DebugQuad & _quad ) override;

	/// e.g. select cells, mesh quads, etc.
	virtual void CastDebugRay( const V3f& _localOrigin, const V3f& _direction );

	virtual void addSphere( const V3f& _center, float _radius, const RGBAf& _color ) override;

public:
	///
	void draw_internal(
		const NwView3D& _camera,
		TbPrimitiveBatcher & _renderer,
		const Options& _options = Options()
		) const;

public:
	// General:
	struct SColoredLine {
		V3f		start;
		V3f		end;
		RGBAf	color;
	};
	DynamicArray< SColoredLine >	m_coloredLines;

	struct SColoredPoint {
		V3f		position;
		float	scale;
		RGBAf	color;
	};
	DynamicArray< SColoredPoint >	m_coloredPoints;

	///
	struct TextLabel
	{
		V3f			position;
		V3f			normal_for_culling;
		float		font_scale;
		RGBAf		color;
		String64	text;
	};
	DynamicArray< TextLabel >		_text_labels;

	// Specific to Dual Contouring:
	DynamicArray< std::pair< V3f, int > >	m_sharpFeatures;	//!< vertices located on sharp features: position + feature type (true = corner, false = edge)
	DynamicArray< std::pair< V3f, V3f > >	m_clampedVerts;	//!< failed QEF solutions, clamped to cell bounds

	DynamicArray< std::pair< V3f, V3f > > m_normals;	//!< points with normals

	struct SDualCell
	{
		enum { MAX_POINTS = 12 };
		AABBf	bounds;
		U8		signs;
		bool	clamped;
		U8		numPoints;
		V3f		points[MAX_POINTS];
		V3f		normals[MAX_POINTS];
	};
	DynamicArray< SDualCell >	m_dualCells;

	DynamicArray< DebugVertex >	m_verts;
	DynamicArray< DebugTri >	m_tris;
	DynamicArray< DebugQuad >	m_quads;	//!< DC quads

	struct HitPrimitive {
		UINT	index;
		float	time;
	public:
		bool operator < ( const HitPrimitive& other ) const {
			return this->time < other.time;
		}
	};
	DynamicArray< int >				m_hitVerts;
	DynamicArray< HitPrimitive >	m_hitTris;
	DynamicArray< HitPrimitive >	m_hitQuads;

	// for convenient selection & third-person view
	NwView3D	savedCameraParams;
	//V3f	savedCameraPosition;
	//V3f	savedCameraDirection;
	//savedCameraFrustum;

	V3f point;
	TArray< V3f > adjacentPoints;

	DynamicArray< std::pair< V3f, V3f > > dualEdges;	//!< DC grid edges

	DynamicArray< std::pair< AABBf, RGBAf > > wireframeAABBs;
	DynamicArray< std::pair< AABBf, RGBAf > > transparentAABBs;

	struct Sphere {
		V3f center;
		float radius;
		RGBAf color;
	};
	DynamicArray< Sphere > m_spheres;

	TArray< U32 >	skippedCells;
};

/// useful for debugging
void Dbg_ControlDebugView( ADebugView & _dbgView, const InputState& input_state );

void DbgShowVolume(
				   const AVolumeSampler& _volume
				   , const AABBf& _volumeBounds
				   , const int resolution_cells
				   , VX::ADebugView &_debugView
				   , bool showVoxels = false
				   );

const RGBAf dbg_cellColorInQuad( const UINT cell_index_in_quad );

/// global debug view; usually works with world-space coords
extern TPtr< ADebugView >	g_DbgView;

namespace Dbg
{
	extern const float LOD_COLORS[16][4];

	/// Octree/Clipmap levels
	extern const U32 g_LoD_colors[8];	//!< tree depth/level colors

	extern const U32 g_OctantColors[8];	//!< for each octree octant/child id

	/// e.g. for coloring 4 full-resolution cells face-adjacent to a half-resolution cell
	extern const U32 g_QuadrantColors[4];

	/// for coloring cells' dual vertices
	extern const U32 g_DualVertexColors[4];

	/// colors of internal, boundary face and boundary edge cell/vertices (ECellType)
	extern const U32 g_cellTypeColors[1+6+12];

	extern const U32 g_cubeFaceColors[6];

}//namespace Dbg

}//namespace VX
