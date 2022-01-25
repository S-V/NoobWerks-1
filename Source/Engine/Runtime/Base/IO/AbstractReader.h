#pragma once


///
///	Abstract stream reader
///
class AReader: public NonCopyable, public DbgNamedObject< 128 >
{
public:
	///
	virtual ERet Read( void *buffer, size_t size ) = 0;

	/// Return the size of data (length of the file if this is a file), in bytes.
	virtual size_t Length() const = 0;

	virtual size_t Tell() const = 0;

public:
	inline ERet SerializeMemory( void *dest, const size_t size )
	{
		return this->Read( dest, size );
	}
	template< typename TYPE >
	inline ERet Get( TYPE &value )
	{
		return this->Read( &value, sizeof(TYPE) );
	}
	template< typename TYPE >
	inline void SerializeArray( TYPE * a, UINT count )
	{
		if( TypeTrait<TYPE>::IsPlainOldDataType ) {
			if( count ) {
				this->SerializeMemory( a, count * sizeof a[0] );
			}
		} else {
			for( UINT i = 0; i < count; i++ ) {
				*this >> a[i];
			}
		}
	}
protected:
	inline AReader() {}
	virtual ~AReader();
private:
	PREVENT_COPY(AReader);
};






namespace AReader_
{
	ERet Align(
		AReader & reader,
		const U32 alignment
		);

}//namespace AReader_
