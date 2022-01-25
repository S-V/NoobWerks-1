#pragma once

/*
===============================================================================
	Run-Length Encoding
===============================================================================
*/

/// 'All counts RLE'
/// Compressed format: [length, value].
/// Returns the number of items written into the output stream.
template< typename TYPE, class STREAM >
U32 RLE_Compress( const TYPE* __restrict _uncompressed, U32 _count, STREAM &_stream )
{
	mxASSERT( _count > 0 );

	const size_t previous_offset = _stream.Tell();

	// reserve
	TYPE * __restrict compressed_data;
	// in the worst case, RLE can expand the output by two
	const U32 max_compressed_size = _count * sizeof(_uncompressed[0]) * 2;
	/*mxDO*/(_stream.Allocate( max_compressed_size, (char*&)compressed_data ));

	//compressed_data[ 0 ] = 0;	// length will be known and written later
	U32	current_offset = 1;

	U32	currentRunLength = 1;	// repeat count

	const U32 MAX_RUN = (1u << BYTES_TO_BITS(sizeof(TYPE)));	// e.g. 256 (0x100u) for uchar

	TYPE previousValue = _uncompressed[ 0 ];
	for( U32 itemIndex = 1; itemIndex < _count; ++itemIndex )
	{
		TYPE currentValue = _uncompressed[ itemIndex ];
		if( previousValue == currentValue && currentRunLength < MAX_RUN )
		{
			++currentRunLength;
		}
		else
		{
			// flush the current run
			compressed_data[ current_offset - 1 ] = currentRunLength - 1;	// run length is always positive
			compressed_data[ current_offset + 0 ] = previousValue;	// repeated value
			//compressed_data[ current_offset + 1 ] = 0;	// length of the next run will be known later
			current_offset += 2;

			// start a new run
			currentRunLength = 1;
			previousValue = currentValue;
		}
	}

	// Write leftover
	if( currentRunLength )
	{
		compressed_data[ current_offset - 1 ] = currentRunLength - 1;	// write the repeat count
		compressed_data[ current_offset++ ] = previousValue;	// write the repeated value
	}

	// commit
	const U32 itemsWritten = current_offset;

	if( itemsWritten < _count ) {
		_stream.Rewind( previous_offset + itemsWritten );
	} else {
		// RLE was ineffective - the compression didn't occur
	}

	return itemsWritten;
}

template< typename TYPE >
static inline
UINT AsUnsignedInt( const TYPE _value );

template<>
static inline
UINT AsUnsignedInt< char >( const char _value ) { return (unsigned char)_value; }

template<>
static inline
UINT AsUnsignedInt< U8 >( const U8 _value ) { return _value; }

template<>
static inline
UINT AsUnsignedInt< short >( const short _value ) { return (unsigned short)_value; }

template<>
static inline
UINT AsUnsignedInt< int >( const int _value ) { return (unsigned int)_value; }


template< typename TYPE >
void RLE_Decompress( const TYPE* __restrict _compressed, U32 _count, TYPE *__restrict _decompressed )
{
	for( UINT i = 0; i < _count; i += 2 )
	{
		//NOTE: the cast is important, e.g. if the length is a (signed char)0xFF, after adding 1 it will become zero
		const UINT tmp = AsUnsignedInt( _compressed[ i ] );
		const UINT runLength = tmp + 1;
		const TYPE runValue = _compressed[ i + 1 ];

		std::fill( _decompressed, _decompressed + runLength, runValue );
		_decompressed += runLength;
	}
}



/// if repeat count > 2, emit a run

/// Auto-Trigger/Auto-Repeat: emit a run only if repeat count >= N (e.g. N=2, N=3, etc.)
template< typename TYPE, class STREAM >
U32 RLE_Compress_AutoTrigger(
							 const TYPE* __restrict _uncompressed
							 , const U32 _count
							 , STREAM &_stream
							 , const U32 MIN_RUN = 2
							 )
{
	mxASSERT( _count > 0 );

	const size_t previous_offset = _stream.Tell();

	// reserve
	TYPE * __restrict compressed_data;
	// in the worst case, RLE can expand the output by two
	const U32 max_compressed_size = _count * sizeof(_uncompressed[0]) * 2;
	if(mxFAILED(_stream.Allocate( max_compressed_size, (char*&)compressed_data ))) {
		return 0;
	}

	//compressed_data[ 0 ] = 0;	// length will be known and written later
	U32	current_offset = 1;

	U32	currentRunLength = 1;	// repeat count

	const U32 MAX_RUN = (1u << BYTES_TO_BITS(sizeof(TYPE)));	// e.g. 256 (0x100u) for uchar

	TYPE previousValue = _uncompressed[ 0 ];
	for( U32 itemIndex = 1; itemIndex < _count; ++itemIndex )
	{
		TYPE currentValue = _uncompressed[ itemIndex ];
		if( previousValue == currentValue && currentRunLength < MAX_RUN )
		{
			++currentRunLength;
		}
		else
		{
			if( currentRunLength < MIN_RUN )
			{
				// a literal run
				while( currentRunLength-- ) {
					compressed_data[ current_offset++ - 1 ] = previousValue;
				}
			}
			else
			{
				// flush the current run
				compressed_data[ current_offset - 1 ] = currentRunLength - 1;	// run length is always positive
				compressed_data[ current_offset + 0 ] = previousValue;	// repeated value
				//compressed_data[ current_offset + 1 ] = 0;	// length of the next run will be known later
				current_offset += 2;	// written the counter and replicated value
			}

			// start a new run
			currentRunLength = 1;
			previousValue = currentValue;
		}
	}

	// Write leftover
	if( currentRunLength < MIN_RUN )
	{
		// copy literals
		while( currentRunLength-- ) {
			compressed_data[ current_offset++ - 1 ] = previousValue;
		}
		current_offset -= 1;	// we don't write the run length
	}
	else
	{
		compressed_data[ current_offset - 1 ] = currentRunLength - 1;	// write the repeat count
		compressed_data[ current_offset + 0 ] = previousValue;	// write the repeated value
		current_offset += 1;	// wrote the repeated value
	}

	// commit
	const U32 itemsWritten = current_offset;

	if( itemsWritten < _count ) {
		_stream.Rewind( previous_offset + itemsWritten );
	} else {
		// RLE was ineffective - the compression didn't occur
	}

	return itemsWritten;
}



///
template< typename VALUE >	// where VALUE is an unsigned integer
class RLE_Writer: NonCopyable
{
	VALUE *			_write_cursor;
	U32				_current_run_length;	//!< accumulated run length ('repeat count')
	VALUE			_previous_value;
	VALUE * const	_buffer_start;

public:
	RLE_Writer( VALUE *destination_buffer, const VALUE& invalid_value )
		: _write_cursor( destination_buffer )
		, _buffer_start( destination_buffer )
	{
		_current_run_length = 0;
		_previous_value = invalid_value;
	}

	~RLE_Writer()
	{
		mxASSERT2(
			_current_run_length == 0,
			"Not all items have been written! Did you forget to call flush()?"
			);
	}

	static size_t maxOutputSize( const UINT num_items )
	{
		/// can expand output twice in the worst case
		return num_items * 2 * sizeof(VALUE);
	}

	static const U32 MAX_RUN_LENGTH = (1u << BYTES_TO_BITS(sizeof(VALUE)));	// e.g. 256 (0x100u) for uchar

	///
	mxFORCEINLINE
	void put( const VALUE item )
	{
		if( item == _previous_value && _current_run_length < MAX_RUN_LENGTH )
		{
			++_current_run_length;
		}
		else
		{
			// Flush the current run.
			if( _current_run_length )
			{
				*_write_cursor++ = _current_run_length - 1;
				*_write_cursor++ = _previous_value;
			}

			// Start a new run.
			_current_run_length = 1;
			_previous_value = item;
		}
	}

	/// Returns the number of written items.
	U32 flush()
	{
		if( _current_run_length )
		{
			*_write_cursor++ = _current_run_length - 1;
			*_write_cursor++ = _previous_value;
			//
			_current_run_length = 0;
		}

		return _write_cursor - _buffer_start;
	}

	mxFORCEINLINE U32 NumValuesWritten() const
	{
		return _write_cursor - _buffer_start;
	}

	mxFORCEINLINE const char* GetWriteCursor() const
	{
		return (char*) _write_cursor;
	}
};

///
template< typename VALUE >	// where VALUE is an unsigned integer
class RLE_Reader: NonCopyable
{
	U32				_current_run_length;	//!< accumulated run length ('repeat count')
	VALUE			_current_value;
	const VALUE *	_read_cursor;

public:
	RLE_Reader( const VALUE* input )
		: _read_cursor( input )
	{
		_current_run_length = *_read_cursor++ + 1;
		_current_value = *_read_cursor++;
	}

	~RLE_Reader()
	{
		mxASSERT2(_current_run_length == 0, "Not all items have been read!");
	}

	VALUE readSingle()
	{
		if( _current_run_length ) {
			--_current_run_length;
		} else {
			_current_run_length = *_read_cursor++;
			_current_value = *_read_cursor++;
		}
		return _current_value;
	}
};

/*
Ad Hoc Compression Methods: RLE
https://hbfs.wordpress.com/2009/04/14/ad-hoc-compression-methods-rle/

*/