/*
=============================================================================
	File:	String.h
	Desc:	String class and string-related functions.
	ToDo:	fix bugs
=============================================================================
*/
#pragma once

#include <Base/Memory/MemoryBase.h>
#include <Base/Object/Reflection/Reflection.h>
#include <Base/Template/Containers/Array/DynamicArray.h>
#include <Base/Text/StringsCommon.h>
#include <Base/Text/StringTools.h>

/// String pointers should be aligned on a 4-byte boundary:
/// 1) we use the lowest bit of the string pointer to store "is memory owned?" flag;
/// 2) aligned strings are more efficient (to parse, etc).
enum { STRING_ALIGNMENT = 4 };

namespace MemoryHeaps {
	void setStringAllocator( AllocatorI * allocator );
}//namespace MemoryHeaps

mxSTOLEN("Havok");
/// This class is used to store a char* c-string.
/// It automatically handles allocations and deallocation of
/// stored c-string. The string memory may be externally or internally
/// allocated, the lowest bit of the pointer being used as a
/// should-deallocate flag. This simplifies destruction of objects
/// which may be allocated in a packfile for instance.
class CharPtr: public TStringBase< char, CharPtr >
{
	/// m_stringAndFlag&~1 is the real pointer, m_stringAndFlag&1 is the deallocate flag.
	const char* m_stringAndFlag;
	enum StringFlags {
		OWNED_FLAG = 0x1,	//!< If we own it, we free it.
	};
	// 4 or 8 bytes
public:
	String& setReference( const char* _chars );
};


/// Dynamic character string.
/// Stores a length and maintains a terminating null byte.
/// If you want an object to point to a string that is seldom or never changed it's better to use CharPtr.
class String: public TStringBase< char, String >
{
	/// should always be null-terminated
	char *	m_stringAndFlags;	//!< two upper bits contain memory ownership status

	U16	m_length;	//!< length not including the last null byte
	U16	m_alloced;	//!< total number of allocated characters (string capacity)

	// 32: 8 bytes
	// 64: 12 bytes

	/// these are stored in 2 lowest bits of the string pointer
	enum StringFlags {
		MEMORY_NOT_OWNED	= 0x1,	//!< Should we deallocate the memory block?
		MEMORY_READONLY		= 0x2,	//!< Can we write to the pointed memory?
	};

public:
	String();
	String( const String& other );
	~String();

	//NOTE: maximum string length will be (size - 1)
	//@todo: pass StringFlags directly?
	void initializeWithExternalStorage( char* buffer, int size, bool readonly = false );

	bool ownsAllocatedMemory() const;

	bool isWriteable() const;

	int length() const;

	/// returns the number of allocated items
	int capacity() const;

	/// resets, but doesn't release memory
	void empty();

	/// releases allocated memory
	void clear();

	/// ensures that the string has enough space _and_ is _writable_
	ERet resize( int _newLength );

	/// ensures string data buffer is large enough; may not allocate memory
	ERet reserve( int _capacity );

	void capLength( int _newLength );

	//=== TArrayBase
	U32 num() const;
	char * raw();
	const char* raw() const;

	ERet Copy( const Chars& _chars );

	/// The string will be tagged as read-only.
	/// NOTE: dangerous - writing to read-only memory results in a crash!
	String& setReference( const Chars& _chars );

	/// you may want to call this before writing to the string (Copy-On-Write, COW)
	ERet EnsureIsWritable();

	ERet EnsureOwnsMemory();

	operator const Chars () const;

	String& operator = ( const Chars& _chars );
	String& operator = ( const String& other );

	// case sensitive comparison
	bool operator == ( const String& other ) const;
	bool operator != ( const String& other ) const;

	// case sensitive comparison
	bool operator == ( const Chars& static_string ) const;
	bool operator != ( const Chars& static_string ) const;

	bool operator < ( const String& _other ) const;

public:	// binary serialization
	ERet SaveToStream( AWriter &_stream ) const;
	ERet LoadFromStream( AReader& _stream );
	friend AWriter& operator << ( AWriter& file, const String& obj );
	friend AReader& operator >> ( AReader& file, String& obj );
	friend mxArchive& operator && ( mxArchive& archive, String& o );

	void DoNotFreeMemory();
	void SetReadOnly();

	void* getBufferAddress() { return &m_stringAndFlags; }

private:// Internal functions:
	// this should only be called on a freshly constructed string
	inline void _initialize()
	{
		m_stringAndFlags = nil;
		m_length = 0;
		m_alloced = 0;
	}
	inline void SetBufferPointer( char* _ptr, int _flags )
	{
		m_stringAndFlags = (char*) (UINT_PTR(_ptr) | (_flags & 0x3));
	}
	// UNSAFE, NOT REALLY CONST:
	inline char* GetBufferPointer() const
	{
		return (char*) (UINT_PTR(m_stringAndFlags) & ~0x3);
	}
};

mxSTATIC_ASSERT_ISPOW2(sizeof(String));

mxDECLARE_BUILTIN_TYPE( String,	ETypeKind::Type_String );

mxIMPLEMENT_SERIALIZE_FUNCTION( String, SerializeString );

typedef TArray< String >	StringListT;

// Forward declarations.
U32 mxGetHashCode( const String& _str );

/*
-------------------------------------------------------------------------
	TLocalString< SIZE >

	aka Stack String / Static String / Fixed String

	Dynamic string with a small embedded storage to avoid memory allocations.
	Grows automatically when the local buffer is not big enough.
	(see: small string optimization.)

	NOTE: 'SIZE' is the size of the string in bytes, not its maximum capacity!
-------------------------------------------------------------------------
*/
template< U32 SIZE >
class TLocalString: public String
{
	char	m_storage[ SIZE - sizeof(String) ];	// local storage to avoid dynamic memory allocation

	void _setLocalBuffer() {
		m_storage[0] = '\0';
		String::initializeWithExternalStorage( m_storage, mxCOUNT_OF(m_storage) );
	}
public:
	TLocalString()
	{
		this->_setLocalBuffer();
	}
	TLocalString( const Chars& s )
	{
		this->_setLocalBuffer();
		String::Copy( s );
	}
	TLocalString( const String& other )
	{
		this->_setLocalBuffer();
		String::operator = ( other );
	}
	TLocalString( const TLocalString& other )
	{
		this->_setLocalBuffer();
		String::operator = ( other );
	}

	void clear()
	{
		String::clear();
		this->_setLocalBuffer();
	}

	String& operator = ( const Chars& chars )
	{
		return String::operator = ( chars );
	}
};

typedef TLocalString< 32 >	String32;
typedef TLocalString< 64 >	String64;
typedef TLocalString< 96 >	String96;
typedef TLocalString< 128 >	String128;
typedef TLocalString< 256 >	String256;
typedef TLocalString< 512 >	String512;

mxDECLARE_BUILTIN_TYPE( String32,	ETypeKind::Type_String );
mxDECLARE_BUILTIN_TYPE( String64,	ETypeKind::Type_String );
mxDECLARE_BUILTIN_TYPE( String128,	ETypeKind::Type_String );
mxDECLARE_BUILTIN_TYPE( String256,	ETypeKind::Type_String );
mxDECLARE_BUILTIN_TYPE( String512,	ETypeKind::Type_String );


typedef String256	FilePathStringT;


namespace Str
{
	ERet CopyS( String & _dst, const char* _str, int _len );
	ERet CopyS( String & _dst, const char* _str );
	ERet Copy( String & _dst, const Chars& _chars );

	ERet Append( String & _dst, char _c );
	ERet AppendS( String & _dst, const char* _str, int _len );
	ERet Append( String & _dst, const Chars& _chars );
	ERet AppendS( String & _dst, const char* _str );

	int Cmp( const String& _str, const Chars& _text );
	bool Equal( const String& _str, const Chars& _text );
	bool EqualS( const String& _str, const char* _text );
	bool EqualC( const String& _str, const char _symbol );

	bool StartsWith( const String& _string, const Chars& _text );

	String& mxVARARGS SAppendF( String & _dst, const char* _fmt, ... );
	String& mxVARARGS Format( String & _dst, const char* _fmt, ... );

	String& setReference( String & _string, const char* _buffer );

	String& SetBool( String & _str, bool _value );
	String& SetChar( String & _str, char _value );
	String& SetInt( String & _str, int _value );
	String& SetUInt( String & _str, unsigned int _value );
	String& SetFloat( String & _str, float _value );
	String& SetDouble( String & _str, double _value );

	String& ToUpper( String & _str );

	String& StripTrailing( String & _str, char _c );

	String& StripLeading( String & _string, const char* _text );

	String& ReplaceChar(
		String & _str
		, char _old, char _new
		, int start_index = 0
		);

	String& InsertChar( String & _str, int _where, char _new );

	/// insert the given character into the string after every N characters
	String& Separate( String & _str, int _step, char _separator );

	inline bool IsPathSeparator( char _c )
	{
		return _c == '/' || _c == '\\';
	}

	/// removes the path from the filename
	String& StripPath( String & _str );

	/// removes the filename from a path (assumes the the string ends with a path separator)
	String& StripFileName( String & _str );

	/// removes any file extension
	String& StripFileExtension( String & _path );

	/// strips off the N last directories from the given path
	String& StripLastFolders( String & _str, int count = 1 );

	ERet setFileExtension(
		String & s,
		const char* new_extension
		);

	String& AppendSlash( String & _str );

	/// changes all '\' to '/'.
	String& FixBackSlashes( String & _str );

	/// Converts a path to the uniform form (Unix-style) in place
	String& NormalizePath( String & _str );

	/// removes all trailing '\' and '/'.
	String& StripTrailingSlashes( String & _str );

	/// returns NULL if the given filename doesn't have an extension
	const char* FindExtensionS( const char* filename );
	bool HasExtensionS( const char* filename, const char* extension );
	void ExtractExtension( String & _extension, const char* filename );
	const char* GetFileName( const char* filepath );

	template< class STRING >
	STRING ComposeFilePath( const char* _filepath, const char* _extension )
	{
		STRING	result;
		Str::CopyS(result, _filepath);
		Str::SetFileExtension(result, _extension);
		return result;
	}
	ERet ComposeFilePath( String& result, const char* _path, const char* _filename );
	ERet ComposeFilePath( String& result, const char* _path, const char* _filebase, const char* _extension );

	//
	ERet loadFileToString(
		DynamicArray< char > &dst_string_
		, AReader & file_stream
		);

	ERet loadFileToString(
		DynamicArray< char > &dst_string_
		, const char* filename
		);

}//namespace Str

template< class TYPE >	// where TYPE has member 'name' of type 'String'
int FindIndexByName( const TArray< TYPE >& items, const String& name )
{
	for( int i = 0; i < items.num(); i++ )
	{
		const TYPE& item = items[ i ];
		if( item.name == name ) {
			return i;
		}
	}
	return -1;
}
template< class TYPE >	// where TYPE has member 'name' of type 'String'
int FindIndexByName( const TBuffer< TYPE >& items, const String& name )
{
	for( int i = 0; i < items.num(); i++ )
	{
		const TYPE& item = items[ i ];
		if( item.name == name ) {
			return i;
		}
	}
	return -1;
}

/// loads the given file to the string and appends a null terminator if needed
ERet Util_LoadFileToString2( const char* _filePath, char **_fileData, UINT *_fileSize, AllocatorI & _heap );
ERet Util_DeleteString2( const void* fileData, size_t size, AllocatorI & _heap );



void GetTimeOfDayString( String &OutTimeOfDay, int hour, int minute, int second, char separator = '-' );

void GetDateString( String &OutCurrDate, int year, int month, int day, char separator = '-' );

void GetDateTimeOfDayString(
	String &OutDateTime,
	const CalendarTime& dateTime = CalendarTime::GetCurrentLocalTime(),
	char separator = '-' );

mxOBSOLETE
template< class STRING >
void GetCurrentDateString( STRING &OutCurrDate )
{
	//mxASSERT( OutCurrDate.IsEmpty() );
	CalendarTime	currDateTime( CalendarTime::GetCurrentLocalTime() );
	GetDateString( OutCurrDate, currDateTime.year, currDateTime.month, currDateTime.day );
}
mxOBSOLETE
template< class STRING >
STRING GetCurrentTimeString( char separator = '-' )
{
	//mxASSERT( OutCurrDate.IsEmpty() );
	CalendarTime	currDateTime( CalendarTime::GetCurrentLocalTime() );
	STRING result;
	GetTimeOfDayString( result, currDateTime.hour, currDateTime.minute, currDateTime.second, separator );
	return result;
}
mxOBSOLETE
template< class STRING >
void GetCurrentDateTimeString( STRING &OutCurrDate )
{
	//mxASSERT( OutCurrDate.IsEmpty() );
	CalendarTime	currDateTime( CalendarTime::GetCurrentLocalTime() );
	GetDateTimeOfDayString( OutCurrDate, currDateTime );
}

inline
const char* GetNumberExtension( UINT num )
{
	return	( num == 1 ) ? "st" :
			( num == 2 ) ? "nd" :
			( num == 3 ) ? "rd" :
			"th";
}

inline
const char* GetWordEnding( UINT num )
{
	return ( num < 10 ) ? "s" : "";
}

//----------------------------------------------------------//
//				End Of File.									//
//----------------------------------------------------------//
