#pragma once


///
///	Abstract stream writer
///
class AWriter: public NonCopyable, public DbgNamedObject< 60 >
{
public:
	virtual ERet Write( const void* buffer, size_t size ) = 0;

public:
	inline ERet SerializeMemory( const void* source, const size_t size )
	{
		return this->Write( source, size );
	}
	template< typename TYPE >
	inline ERet Put( const TYPE& value )
	{
		return this->Write( &value, sizeof(TYPE) );
	}
	template< typename TYPE >
	inline void SerializeArray( TYPE * a, UINT count )
	{
		if( TypeTrait<TYPE>::IsPlainOldDataType ) {
			if( count ) {
				this->SerializeMemory( a, count * sizeof(a[0]) );
			}
		} else {
			for( UINT i = 0; i < count; i++ ) {
				*this << a[i];
			}
		}
	}

	virtual void VPreallocate( U32 sizeHint ) {}

protected:
	inline AWriter() {}
	virtual ~AWriter();
private:
	PREVENT_COPY(AWriter);
};
