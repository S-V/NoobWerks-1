// ozz-animation
// http://guillaumeblanc.github.io/ozz-animation/
#pragma once

//
#include <ozz/base/log.h>
#include <ozz/base/io/stream.h>
#include <ozz/base/io/archive.h>
//
#include "ozz/base/maths/simd_math.h"
#include "ozz/base/maths/soa_transform.h"
#include "ozz/base/maths/vec_float.h"
#include "ozz/base/maths/quaternion.h"
//
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/animation/runtime/blending_job.h>
#include "ozz/animation/runtime/local_to_model_job.h"
#include "ozz/animation/runtime/skeleton.h"
#include "ozz/animation/runtime/track.h"
#include "ozz/geometry/runtime/skinning_job.h"

#include <ozz/animation/runtime/skeleton_utils.h>	// GetJointLocalBindPose()
//
#include "ozz/base/maths/simd_math.h"
#include "ozz/base/maths/soa_transform.h"
#include "ozz/base/maths/vec_float.h"
//

#include <Core/Assets/AssetManagement.h>	// AssetReader


namespace Rendering
{

inline
const ozz::math::Float3 toOzzVec( const V3f& v )
{ return ozz::math::Float3(v.x, v.y, v.z); }



static mxFORCEINLINE
const M44f& ozzMatricesToM44f( const ozz::math::Float4x4& m )
{
	return *reinterpret_cast< const M44f* >( &m );
}

static mxFORCEINLINE
const M44f* ozzMatricesToM44f( const ozz::math::Float4x4* ozz_matrices )
{
	return reinterpret_cast< const M44f* >( ozz_matrices );
}



template< typename T >
inline
const ozz::Range<T> ozz_range_from_array( TArray<T> & a )
{
	return ozz::Range<T>( a.begin(), a.end() );
}

template< typename T >
inline
const ozz::Range<T> ozz_range_from_array( DynamicArray<T> & a )
{
	return ozz::Range<T>( a.begin(), a.end() );
}

int getJointIndexByName(
						const char* joint_name
						, const ozz::animation::Skeleton& skeleton
						);

enum
{
	OZZ_SKELETON_VERSION = 2,
	OZZ_ANIMATION_VERSION = 6,

	// ozz data should be stored at 16-byte aligned offsets
	OZZ_DATA_ALIGNMENT = 16
};

///
class ozzAssetReader: public ozz::io::Stream
{
	AssetReader &	_asset_reader;

public:
	ozzAssetReader( AssetReader & asset_reader )
		: _asset_reader( asset_reader )
	{}

	// Tests whether a file is opened.
	virtual bool opened() const { return true; }

	// Reads _size bytes of data to _buffer from the stream. _buffer must be big
	// enough to store _size bytes. The position indicator of the stream is
	// advanced by the total amount of bytes read.
	// Returns the number of bytes actually read, which may be less than _size.
	virtual size_t Read(void* _buffer, size_t _size) override
	{
		return _asset_reader.Read( _buffer, _size ) == ALL_OK ? _size : 0;
	}

	// Writes _size bytes of data from _buffer to the stream. The position
	// indicator of the stream is advanced by the total number of bytes written.
	// Returns the number of bytes actually written, which may be less than _size.
	virtual size_t Write(const void* _buffer, size_t _size) override
	{
		mxUNREACHABLE;
		return 0;
	}

	// Sets the position indicator associated with the stream to a new position
	// defined by adding _offset to a reference position specified by _origin.
	// Returns a zero value if successful, otherwise returns a non-zero value.
	virtual int Seek(int _offset, Origin _origin) override
	{
		mxUNREACHABLE;
		return 0;
	}

	// Returns the current value of the position indicator of the stream.
	// Returns -1 if an error occurs.
	virtual int Tell() const override
	{
		return _asset_reader.Tell();
	}

	// Returns the current size of the stream.
	virtual size_t Size() const override
	{
		return _asset_reader.Length();
	}
};

///
class ozzMemoryReader: public ozz::io::Stream
{
public:
	ozzMemoryReader( const void* data, size_t size )
		: _data( data ), _size( size )
	{
		_read_cursor = 0;UNDONE;
	}

	// Tests whether a file is opened.
	virtual bool opened() const { return true; }

	// Reads _size bytes of data to _buffer from the stream. _buffer must be big
	// enough to store _size bytes. The position indicator of the stream is
	// advanced by the total amount of bytes read.
	// Returns the number of bytes actually read, which may be less than _size.
	virtual size_t Read(void* _buffer, size_t _size) override
	{
		mxUNREACHABLE;
		return 0;
	}

	// Writes _size bytes of data from _buffer to the stream. The position
	// indicator of the stream is advanced by the total number of bytes written.
	// Returns the number of bytes actually written, which may be less than _size.
	virtual size_t Write(const void* _buffer, size_t _size) override
	{
		mxUNREACHABLE;
		return 0;
	}

	// Sets the position indicator associated with the stream to a new position
	// defined by adding _offset to a reference position specified by _origin.
	// Returns a zero value if successful, otherwise returns a non-zero value.
	virtual int Seek(int _offset, Origin _origin) override
	{
		mxUNREACHABLE;
		return 0;
	}

	// Returns the current value of the position indicator of the stream.
	// Returns -1 if an error occurs.
	virtual int Tell() const override
	{
		mxUNREACHABLE;
		return 0;
	}

	// Returns the current size of the stream.
	virtual size_t Size() const override
	{
		mxUNREACHABLE;
		return 0;
	}

private:
	const void *_data;

	// The cursor position in the buffer of data.
	U32			_read_cursor;

	const U32	_size;
};

}//namespace Rendering
