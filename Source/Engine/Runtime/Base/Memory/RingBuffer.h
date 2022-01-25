/*
=============================================================================
	File:	RingBuffer.h
	Desc:	Implements the ring buffer (aka Circular Buffer).
=============================================================================
*/
#pragma once

#include <Base/Memory/MemoryBase.h>

/// FIFO (First In, First Out) ring buffer:
/// items are added at the back and removed from the front.
/// This is a good structure to store the last N incoming items.
template< class HEADER >
class TRingBuffer
{
	void *		m_buffer_start;	// Pointer to allocated memory space for buffer.
	void *		m_buffer_end;
	U32			m_capacity;	// The size of allocated buffer memory, in bytes.
	HEADER *	m_write;	// The 'write' pointer, where allocation takes place.
	HEADER *	m_read;		// The 'read' pointer, where blocks are released.
	HEADER *	m_last_allocated;	// The most recently allocated block.
	//       +------------------------+
	// READ  |                        | WRITE
	//       +------------------------+
	void ResetToEmpty() {
		m_read = m_write = (HEADER*) m_buffer_start;
		m_last_allocated = nil;
	}

public:
	TRingBuffer()
	{
		mxZERO_OUT(*this);
	}
	~TRingBuffer()
	{
	}
	ERet Initialize( void * _base, U32 _capacity )
	{
		mxASSERT_PTR(_base);
		mxASSERT(IS_ALIGNED_BY(_base, mxALIGNOF(HEADER)));
		mxASSERT(_capacity);
		m_buffer_start = _base;
		m_buffer_end = mxAddByteOffset( m_buffer_start, _capacity );
		m_capacity = _capacity;
		this->ResetToEmpty();
		return ALL_OK;
	}
	void Shutdown()
	{
		mxZERO_OUT(*this);
	}

	U32 capacity() const {
		return m_capacity;
	}

	bool IsEmpty() const {
		return m_read == m_write;
	}

	HEADER* GetReadPtr() {
		return IsEmpty() ? nil : m_read;
	}

	/// Checks if the allocation will cause the write pointer to move past the buffer end
	/// (i.e. the length of contiguous space [tail..end] is less than _size).
	bool WillWrapAround( U32 _size ) const
	{
		return mxAddByteOffset( m_write, _size ) > m_buffer_end;
	}

	//
	/// (i.e. tests if the 'write' pointer won't overwrite the 'read' pointer).
	bool HasEnoughFreeSpace( U32 _size ) const
	{
		if( m_read == m_write ) {
			// the buffer is empty, and both pointers will be reset to the beginning
			return _size < m_capacity;
		}
		const HEADER* newWritePtr = mxAddByteOffset( m_write, _size );
		if( m_read < m_write && newWritePtr >= m_buffer_end ) {
			return false;
		}
		if( m_read > m_write && newWritePtr >= m_read ) {
			return false;
		}
		return true;
	}

	HEADER* Allocate( U32 _length, void * user_data )
	{
		const U32 lengthWithHeader = HEADER::SIZE + TAlignUp<16>(_length);
		mxASSERT(lengthWithHeader <= m_capacity);

		if( !this->HasEnoughFreeSpace( lengthWithHeader ) )
		{
			if( !HEADER::FreeSomeSpace( &m_read, m_write, user_data ) ) {
				return nil;
			}
			if( !this->HasEnoughFreeSpace( lengthWithHeader ) ) {
				return nil;
			}
		}

		if( m_read == m_write ) {
			this->ResetToEmpty();
		}

		HEADER* newBlock = nil;
		if( WillWrapAround( lengthWithHeader ) )
		{
			newBlock = (HEADER*) m_buffer_start;
			m_write = mxAddByteOffset( newBlock, lengthWithHeader );
			newBlock->__next = mxAddByteOffset( newBlock, lengthWithHeader );
		}
		else
		{
			newBlock = m_write;
			HEADER* newWritePtr = mxAddByteOffset( m_write, lengthWithHeader );
			mxASSERT(newWritePtr != m_buffer_end);
			mxASSERT(newWritePtr != m_read);
			m_write = newWritePtr;
			newBlock->__next = mxAddByteOffset( newBlock, lengthWithHeader );
		}

		newBlock->Initialize();
		newBlock->__size = _length;

		if( m_last_allocated ) {
			m_last_allocated->__next = newBlock;
		}
		m_last_allocated = newBlock;

		return newBlock;
	}
};



#if 0
// http://www.asawicki.info/news_1468_circular_buffer_of_raw_binary_data_in_c.html

#include <algorithm> // for std::min

class CircularBuffer
{
public:
  CircularBuffer(size_t capacity);
  ~CircularBuffer();

  size_t size() const { return size_; }
  size_t capacity() const { return capacity_; }
  // Return number of bytes written.
  size_t write(const char *data, size_t bytes);
  // Return number of bytes read.
  size_t read(char *data, size_t bytes);

private:
  size_t beg_index_, end_index_, size_, capacity_;
  char *data_;
};

CircularBuffer::CircularBuffer(size_t capacity)
  : beg_index_(0)
  , end_index_(0)
  , size_(0)
  , capacity_(capacity)
{
  data_ = new char[capacity];
}

CircularBuffer::~CircularBuffer()
{
  delete [] data_;
}

size_t CircularBuffer::write(const char *data, size_t bytes)
{
  if (bytes == 0) return 0;

  size_t capacity = capacity_;
  size_t bytes_to_write = std::min(bytes, capacity - size_);

  // Write in a single step
  if (bytes_to_write <= capacity - end_index_)
  {
    memcpy(data_ + end_index_, data, bytes_to_write);
    end_index_ += bytes_to_write;
    if (end_index_ == capacity) end_index_ = 0;
  }
  // Write in two steps
  else
  {
    size_t size_1 = capacity - end_index_;
    memcpy(data_ + end_index_, data, size_1);
    size_t size_2 = bytes_to_write - size_1;
    memcpy(data_, data + size_1, size_2);
    end_index_ = size_2;
  }

  size_ += bytes_to_write;
  return bytes_to_write;
}

size_t CircularBuffer::read(char *data, size_t bytes)
{
  if (bytes == 0) return 0;

  size_t capacity = capacity_;
  size_t bytes_to_read = std::min(bytes, size_);

  // Read in a single step
  if (bytes_to_read <= capacity - beg_index_)
  {
    memcpy(data, data_ + beg_index_, bytes_to_read);
    beg_index_ += bytes_to_read;
    if (beg_index_ == capacity) beg_index_ = 0;
  }
  // Read in two steps
  else
  {
    size_t size_1 = capacity - beg_index_;
    memcpy(data, data_ + beg_index_, size_1);
    size_t size_2 = bytes_to_read - size_1;
    memcpy(data + size_1, data_, size_2);
    beg_index_ = size_2;
  }

  size_ -= bytes_to_read;
  return bytes_to_read;
}
#endif

#if 0
class RingBuffer
{
	BYTE *	m_base;		// Pointer to allocated memory space for buffer.
	U32		m_capacity;	// The size of allocated buffer memory, in bytes.
	U32		m_head;	// The head position, 'read' pointer (offset from the start of the buffer).
	U32		m_tail;	// The tail position, 'write' pointer (offset from the start of the buffer).
public:
	RingBuffer()
	{
	}
	~RingBuffer()
	{
	}
	/// Allocates memory and initializes the buffer.
	ERet Initialize( void * _base, U32 _capacity )
	{
		mxASSERT_PTR(_base);
		mxASSERT(_capacity);
		m_base = _base;
		m_capacity = _capacity;
		// Reset head and tail positions
		m_head = 0;
		m_tail = 0;
		return ALL_OK;
	}
	/// Deallocates all memory and destroys the buffer.
	void Shutdown()
	{
		//
	}
	U32 ReadableSpace() const {
		return m_tail - m_head;
	}

#if 0
// Returns the amount of data (in bytes) available for reading from the buffer.
int GetMaxReadSize();
// Returns the amount of space (in bytes) available for writing into the buffer.
int GetMaxWriteSize();
#endif
	/// Returns the number of bytes written (which could be from 0 to _size).
	U32 Write( const void* _data, U32 _size )
	{
		//
	}
	/// Returns the number of bytes read (which could be from 0 to _size).
	U32 Read( void *_data, U32 _size )
	{
		//
	}
};
#endif
//----------------------------------------------------------//
//				End Of File.									//
//----------------------------------------------------------//
