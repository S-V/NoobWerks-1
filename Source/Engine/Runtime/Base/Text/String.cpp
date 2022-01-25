/*
=============================================================================
	File:	String.cpp
	Desc:	String class.

	String formatting libraries:
	http://bstring.sourceforge.net/features.html
	https://github.com/cppformat/cppformat
	http://daniel.haxx.se/projects/trio/

	String implementations:
	Facebook's FBString
	https://code.google.com/p/libglim/source/browse/trunk/gstring.hpp
=============================================================================
*/
#include <Base/Base_PCH.h>
#pragma hdrstop
#include <Base/Base.h>
#include <Base/Text/TextUtils.h>
// for PRIXPTR
#include <inttypes.h>

namespace {
	enum { MAX_LENGTH = 4096 };
	enum { GRANULARITY = 16 };

	static AllocatorI *	s_stringAllocator = nil;

	static char* AllocateStringData( size_t size ) {
		void* mem = s_stringAllocator->Allocate( size, STRING_ALIGNMENT );
//		DBGOUT("String: alloc %d -> 0x%"PRIXPTR, size, (uintptr_t)mem);
		mxASSERT2((UINT_PTR(mem) & 3) == 0, "The pointer must be at least 4-byte aligned");
		return (char*)mem;
	}
	static void FreeStringData( void* ptr, size_t size ) {
		if( ptr ) {
//			DBGOUT("String: free 0x%"PRIXPTR, (uintptr_t)ptr);
			(void)size;
			s_stringAllocator->Deallocate( ptr );
		}
	}
}//namespace


namespace MemoryHeaps {
	void setStringAllocator( AllocatorI * allocator )
	{
		mxASSERT(nil == s_stringAllocator);
		s_stringAllocator = allocator;
	}
}//namespace MemoryHeaps


String::String()
{
	this->_initialize();
}

String::String( const String& other )
{
	this->_initialize();
	*this = other;
}

String::~String()
{
	this->clear();
}

void String::initializeWithExternalStorage( char* buffer, int size, bool readonly )
{
	mxASSERT_PTR(buffer);
	mxASSERT2((UINT_PTR(buffer) & 3) == 0, "The pointer must be at least 4-byte aligned");
	mxASSERT(size > 0);

	this->clear();

	int flags = MEMORY_NOT_OWNED;
	if( readonly ) {
		flags |= MEMORY_READONLY;
	}
	this->SetBufferPointer( buffer, flags );

	m_length = 0;
	m_alloced = size;
}

bool String::ownsAllocatedMemory() const
{
	return (UINT_PTR(m_stringAndFlags) & MEMORY_NOT_OWNED) == 0;
}
bool String::isWriteable() const
{
	return (UINT_PTR(m_stringAndFlags) & MEMORY_READONLY) == 0;
}

int String::length() const
{
	return m_length;
}

int String::capacity() const
{
	return m_alloced;
}

void String::empty()
{
	if( m_length && this->isWriteable() ) {
		this->GetBufferPointer()[0] = '\0';
	}
	m_length = 0;
}

void String::clear()
{
	if( m_alloced && this->ownsAllocatedMemory() ) {
		FreeStringData( this->raw(), this->capacity() );
	}
	this->_initialize();
}

ERet String::resize( int _length )
{
	mxDO(this->reserve( _length + 1 ));
	mxDO(this->EnsureIsWritable());

	this->GetBufferPointer()[ _length ] = '\0';
	m_length = _length;

	return ALL_OK;
}

ERet String::reserve( int _capacity )
{
	const int length = this->length();
	const int capacity = this->capacity();	
	if( _capacity > capacity )
	{
		char* oldBuffer = this->GetBufferPointer();
		char* newBuffer = AllocateStringData( _capacity );
		if( !newBuffer ) {
			return ERR_OUT_OF_MEMORY;
		}
		if( length ) {
			memcpy( newBuffer, oldBuffer, length+1 );	// copy trailing zero
		}
		if( this->ownsAllocatedMemory() ) {
			FreeStringData( oldBuffer, capacity );
		}
		this->SetBufferPointer( newBuffer, 0 );
		m_alloced = _capacity;
	}
	return ALL_OK;
}

void String::capLength( int _newLength )
{
	mxASSERT( this->isWriteable() );
	mxASSERT( _newLength <= m_length );
	if( _newLength < m_length )
	{
		this->GetBufferPointer()[ _newLength ] = '\0';
		m_length = _newLength;
	}
}

U32 String::num() const
{
	return this->length();
}

char * String::raw()
{
	char* buffer = this->GetBufferPointer();
	mxASSERT_PTR(buffer);
	return buffer;
}

const char* String::raw() const
{
	char* buffer = this->GetBufferPointer();
	mxASSERT_PTR(buffer);
	return buffer;
}

ERet String::Copy( const Chars& chars )
{
	if( chars.length )
	{
		if( this->GetBufferPointer() != chars.buffer )
		{
			mxDO(this->resize( chars.length ));
			strncpy( this->GetBufferPointer(), chars.buffer, chars.length );
		}
		else
		{
			// we are both pointing at the same string
			mxASSERT(m_length == chars.length);
		}
	}
	else
	{
		this->empty();
	}
	return ALL_OK;
}

String& String::setReference( const Chars& chars )
{
	this->clear();
	this->initializeWithExternalStorage( const_cast< char* >( chars.buffer ), chars.length+1, true/*read-only*/ );
	m_length = chars.length;
	return *this;
}

ERet String::EnsureIsWritable()
{
	if( !this->isWriteable() ) {
		mxDO(this->EnsureOwnsMemory());
	}
	mxASSERT(this->isWriteable());
	return ALL_OK;
}

ERet String::EnsureOwnsMemory()
{
	const int length = m_length;
	if( m_alloced && !this->ownsAllocatedMemory() )
	{
		char* oldBuffer = this->raw();
		char* newBuffer = AllocateStringData( length + 1 );
		memcpy( newBuffer, oldBuffer, length );
		m_stringAndFlags = newBuffer;
		m_stringAndFlags[length] = '\0';
		m_alloced = length + 1;
	}
	return ALL_OK;
}

String::operator const Chars () const
{
	return Chars( this->GetBufferPointer(), this->length() );
}

String& String::operator = ( const Chars& chars )
{
	this->Copy( chars );
	return *this;
}

String& String::operator = ( const String& other )
{
	this->Copy( other );
	return *this;
}

bool String::operator == ( const String& other ) const
{
	if( this->raw() == other.raw() ) {
		return true;
	}
	return 0 == strcmp( this->SafeGetPtr(), other.SafeGetPtr() );
}

bool String::operator != ( const String& other ) const
{
	return !( *this == other );
}

bool String::operator == ( const Chars& static_string ) const
{
	if( this->length() != static_string.length ) {
		return false;
	}
	return 0 == strncmp( this->SafeGetPtr(), static_string.buffer, static_string.length );
}

bool String::operator != ( const Chars& static_string ) const
{
	return !( *this == static_string );
}


bool String::operator < ( const String& _other ) const
{
	return strcmp( this->c_str(), _other.c_str() ) < 0;
}

void String::DoNotFreeMemory()
{
	m_stringAndFlags = (char*) (UINT_PTR(m_stringAndFlags) | MEMORY_NOT_OWNED);
}
void String::SetReadOnly()
{
	m_stringAndFlags = (char*) (UINT_PTR(m_stringAndFlags) | MEMORY_READONLY);
}

ERet String::SaveToStream( AWriter &_stream ) const
{
	const U32 length = this->length();
	mxDO(_stream.Put( length ));
	if( length > 0 ) {
		mxDO(_stream.Write( this->raw(), length ));
	}
	return ALL_OK;
}

ERet String::LoadFromStream( AReader& _stream )
{
	U32	length;
	mxDO(_stream.Get( length ));
	if( length > 0 ) {
		mxDO(this->resize( length ));
		mxDO(_stream.Read( this->raw(), length ));
	} else {
		this->empty();
	}
	return ALL_OK;
}

AWriter& operator << ( AWriter& stream, const String& obj )
{
	obj.SaveToStream(stream);
	return stream;
}

AReader& operator >> ( AReader& stream, String& obj )
{
	obj.LoadFromStream(stream);
	return stream;
}

namespace Str
{
	ERet CopyS( String & _dst, const char* _str, int _len )
	{
		mxDO(_dst.resize( _len ));
		char* ptr = _dst.raw();
		strncpy( ptr, _str, _len );
		ptr[ _len ] = '\0';
		return ALL_OK;
	}
	ERet CopyS( String & _dst, const char* _str )
	{
		return CopyS( _dst, _str, strlen(_str) );
	}
	ERet Copy( String & _dst, const Chars& _chars )
	{
		return CopyS( _dst, _chars.buffer, _chars.length );
	}

	ERet Append( String & _dst, char _c )
	{
		return AppendS( _dst, &_c, 1 );
	}
	ERet AppendS( String & _dst, const char* _str, int _len )
	{
		const int oldLen = _dst.length();
		mxDO(_dst.resize( oldLen + _len ));
		char* ptr = _dst.raw();
		memcpy( ptr + oldLen, _str, _len );
		ptr[ oldLen + _len ] = '\0';
		return ALL_OK;
	}
	ERet Append( String & _dst, const Chars& _chars )
	{
		return AppendS( _dst, _chars.buffer, _chars.length );
	}
	ERet AppendS( String & _dst, const char* _str )
	{
		return AppendS( _dst, _str, strlen(_str) );
	}

	int Cmp( const String& _str, const Chars& _text )
	{
		return strcmp( _str.c_str(), _text.buffer );
	}
	bool Equal( const String& _str, const Chars& _text )
	{
		return Cmp( _str, _text ) == 0;
	}
	bool EqualS( const String& _str, const char* _text )
	{
		return Cmp( _str, Chars(_text,_InitSlow) ) == 0;
	}
	bool EqualC( const String& _str, const char _symbol )
	{
		return (_str.length() == 1) && (_str[0] == _symbol);
	}

	bool StartsWith( const String& _string, const Chars& _text )
	{
		const char* s1 = _string.c_str();
		const char* s2 = _text.c_str();
		const int l1 = _string.num();
		const int l2 = _text.num();

		if( l1 >= l2 )
		{
			int n = l2;
			while( n-- )
			{
				if( *s1++ != *s2++ ) {
					return false;
				}
			}
			return true;
		}
		return false;
	}

	String& mxVARARGS SAppendF( String & _dst, const char* _fmt, ... )
	{
		va_list	args;
		va_start( args, _fmt );
		ptPRINT_VARARGS_BODY( _fmt, args, Str::AppendS(_dst, ptr_, len_) );
		va_end( args );
		return _dst;
	}
	String& mxVARARGS Format( String & _dst, const char* _fmt, ... )
	{
		va_list	args;
		va_start( args, _fmt );
		ptPRINT_VARARGS_BODY( _fmt, args, Str::CopyS(_dst, ptr_, len_) );
		va_end( args );
		return _dst;
	}

	String& setReference( String & _string, const char* _buffer )
	{
		const size_t length = strlen(_buffer);
		_string.setReference(Chars(_buffer,length));
		return _string;
	}

	String& SetBool( String & _str, bool _value )
	{
		_str = _value ? Chars("true") : Chars("false");
		return _str;
	}
	String& SetChar( String & _str, char _value )
	{
		_str.resize(1);
		*(_str.raw()) = _value;
		return _str;
	}
	String& SetInt( String & _str, int _value )
	{
		return Format( _str, "%d", _value );
	}
	String& SetUInt( String & _str, unsigned int _value )
	{
		return Format( _str, "%u", _value );
	}
	String& SetFloat( String & _str, float _value )
	{
		return Format( _str, "%f", _value );
	}
	String& SetDouble( String & _str, double _value )
	{
		return Format( _str, "%g", _value );
	}

	String& ToUpper( String & _str )
	{
		int	len = _str.length();
		char *	ptr = _str.raw();		
		for( int i = 0; i < len; i++ ) {
			ptr[i] = toupper( ptr[i] );
		}
		return _str;
	}

	String& StripTrailing( String & _str, char _c )
	{
		char *	p = _str.raw();
		int		i = _str.length();
		while( (i > 0) && (_str[i-1] == _c) )
		{
			p[i-1] = '\0';
			i--;
		}
		_str.capLength(i);
		return _str;
	}

	mxSTOLEN("idLib, Doom3 BFG")
	// strip string from front as many times as the string occurs
	String& StripLeading( String & _string, const char* _text )
	{
		const int l = strlen( _text );
		char *	data = _string.raw();
		int		length = _string.length();

		if( l > 0 ) {
			while( !strncmp( data, _text, l ) ) {
				memmove( data, data + l, length - l + 1 );
				length -= l;
			}
		}
		_string.capLength(length);
		return _string;
	}

	String& ReplaceChar(
		String & _str
		, char _old, char _new
		, int start_index /*= 0*/
		)
	{
		char *	p = _str.raw() + start_index;
		while( *p ) {
			if( *p == _old ) {
				*p = _new;
			}
			p++;
		}
		return _str;
	}

	String& InsertChar( String & _str, int _where, char _new )
	{
		const int length = _str.length();
		_where = Clamp( _where, 0, length );
		_str.resize( length + 1 );
		char *	data = _str.raw();
		// shift right
		for( int i = length; i >= _where; i-- ) {
			data[i+1] = data[i];
		}
		data[_where] = _new;
		return _str;
	}

	String& Separate( String & _str, int _step, char _separator )
	{
		mxASSERT(_step > 0);
		const int length = _str.length();
		int numInserted = 0;
		for( int i = 0; i < length; i++ )
		{
			if( i % _step == 0 ) {
				InsertChar( _str, i + numInserted++, _separator );
			}
		}
		return _str;
	}

	// removes the path from the filename
	String& StripPath( String & _str )
	{
		String256	tmp;
		Str::Copy( tmp, _str );
		int		i = tmp.length();
		char *	p = tmp.raw();
		while( ( i > 0 ) && ( p[i-1] != '/' ) && ( p[i-1] != '\\' ) ) {
			i--;
		}
		Str::CopyS( _str, p + i, _str.length() - i );
		return _str;
	}

	/// removes the filename from a path (assumes the the string ends with a path separator)
	String& StripFileName( String & _str )
	{
		// scan the string backwards and find the position of the last path separator
		int		i = _str.length() - 1;
		char *	p = _str.raw();
		while( (i > 0) && !IsPathSeparator( p[i] ) ) {
			i--;
		}
		if( i < 0 ) {
			i = 0;
		}
		_str.capLength( i );
		return _str;
	}

	// removes any file extension
	String& StripFileExtension( String & _str )
	{
		// find the last dot. If it's followed by a dot or a slash,
		// then it's part of a directory specifier, e.g. "../testbed/.Build/unnamed_file".

		// scan backwards for '.'

		int length = _str.length() - 1;
		char* data = _str.raw();
		int i = length - 1;
		while( i >= 0 )
		{
			if( data[i] == '.' || IsPathSeparator(data[i]) ) {
				break;
			}
			i--;
		}

		if( i > 0 && !IsPathSeparator(data[i]) )
		{
			// must the the last dot
			data[i] = '\0';
			_str.resize( i );
		}

		return _str;
	}

	String& StripLastFolders( String & _path, int count )
	{
		int len = _path.length();
		if( len > 0 )
		{
			// skip trailing slash
			if( IsPathSeparator( _path[len-1] ) ) {
				len--;
			}

			int folders_removed = 0;

			int i = len - 1;
			while( i > 0 )
			{
				if( IsPathSeparator( _path[i] ) )
				{
					_path.capLength( i + 1 );
					++folders_removed;
				}
				if( folders_removed >= count ) {
					break;
				}
				i--;
			}
		}
		return _path;
	}

	ERet setFileExtension(
		String & s,
		const char* new_extension
		)
	{
		StripFileExtension(s);
		if ( *new_extension != '.' ) {
			mxDO(Str::Append( s, '.' ));
		}
		mxDO(Str::AppendS( s, new_extension ));
		return ALL_OK;
	}

	// changes all '\' to '/'.
	String& FixBackSlashes( String & _str )
	{
		return ReplaceChar( _str, '\\', '/' );
	}

	String& AppendSlash( String & _str )
	{
		if( _str.length() > 0 && !IsPathSeparator(_str.getLast()) )
		{
			Append( _str, '/' );
		}
		return _str;
	}

	// Converts a path to the uniform form (Unix-style) in place
	String& NormalizePath( String & _str )
	{
		if( _str.nonEmpty() )
		{
			FixBackSlashes( _str );
			if( _str.getLast() != '/' ) {
				Append( _str, '/' );
			}
		}
		return _str;
	}

	String& StripTrailingSlashes( String & _str )
	{
		UINT newLength = ::StripTrailingSlashes( _str.raw(), _str.length() );
		_str.capLength( newLength );
		return _str;
	}

	const char* FindExtensionS( const char* filename )
	{
		int length = strlen(filename);
		for(int i = length-1; i >= 0; i-- )
		{
			if( filename[i] == '.' ) {
				return filename + i + 1;
			}
		}
		return nil;
	}
	bool HasExtensionS( const char* filename, const char* extension )
	{
		// skip '.' in the given extension string
		while( extension && *extension == '.' ) {
			extension++;
		}
		// back up until a '.' or the start
		const char* fileExt = FindExtensionS(filename);
		if( fileExt && extension )
		{
			return stricmp(fileExt, extension) == 0;
		}
		return false;
	}
	void ExtractExtension( String & _extension, const char* filename )
	{
		const char* extension = FindExtensionS(filename);
		if( extension ) {
			Str::CopyS( _extension, extension );
		} else {
			_extension.empty();
		}
	}
	mxSTOLEN("Valve's Source Engine");
	//-------------------------------------------------------------------------
	// Purpose: Returns a pointer to the beginning of the unqualified file name 
	//			(no path information)
	// Input:	in - file name (may be unqualified, relative or absolute path)
	// Output:	pointer to unqualified file name
	//-------------------------------------------------------------------------
	const char* GetFileName( const char* filepath )
	{
		// back up until the character after the first path separator we find,
		// or the beginning of the string
		const char * out = filepath + strlen( filepath ) - 1;
		while ( ( out > filepath ) && ( !IsPathSeparator( *( out-1 ) ) ) )
			out--;
		return out;
	}

	static
	ERet ensureEndsWithPathSeparator( const String& s )
	{
		if( s.length() > 0 && s[s.length() - 1] == '/' ) {
			return ALL_OK;
		}
		return ERR_INVALID_PARAMETER;
	}

	ERet ComposeFilePath( String& result, const char* _path, const char* _filename )
	{
		mxDO(Str::CopyS(result, _path));
		Str::NormalizePath(result);
		mxDO(Str::AppendS(result, _filename));
		return ALL_OK;
	}
	ERet ComposeFilePath( String& result, const char* _path, const char* _filebase, const char* _extension )
	{
		mxDO(Str::CopyS(result, _path));
		Str::NormalizePath(result);
		mxDO(Str::AppendS(result, GetFileName(_filebase)));
		mxDO(Str::setFileExtension(result, _extension));
		return ALL_OK;
	}

	ERet loadFileToString(
		DynamicArray< char > &dst_string_
		, AReader & file_stream
		)
	{
		mxDO(dst_string_.setNum( file_stream.Length() ));
		mxDO(file_stream.Read( dst_string_.raw(), dst_string_.rawSize() ));
		return ALL_OK;
	}

	ERet loadFileToString(
		DynamicArray< char > &dst_string_
		, const char* filename
		)
	{
		FileReader	file_reader;
		mxDO(file_reader.Open( filename ));

		return loadFileToString( dst_string_, file_reader );
	}

}//namespace Str

// loads the given file to the string and appends a null terminator if needed
ERet Util_LoadFileToString2( const char* _filePath, char **_fileData, UINT *_fileSize, AllocatorI & _heap )
{
	chkRET_X_IF_NIL( _filePath, ERR_NULL_POINTER_PASSED );
	chkRET_X_IF_NIL( _fileData, ERR_NULL_POINTER_PASSED );
	chkRET_X_IF_NIL( _fileSize, ERR_NULL_POINTER_PASSED );

	*_fileData = nil;
	*_fileSize = 0;

	FileReader	file;
	mxTRY(file.Open( _filePath, FileRead_NoErrors ));

	const size_t realSize = file.Length();
	mxASSERT(_fileSize > 0);

	char* data = (char*) _heap.Allocate( realSize + 1, STRING_ALIGNMENT );
	mxENSURE(data, ERR_OUT_OF_MEMORY, "Failed to allocate %llu bytes", realSize);

	mxTRY(file.Read( data, realSize ));
	data[ realSize ] = '\0';

	*_fileData = data;
	*_fileSize = realSize;

	return ALL_OK;
}

ERet Util_DeleteString2( const void* fileData, size_t size, AllocatorI & _heap )
{
	if( fileData != nil )
	{
		_heap.Deallocate( (void*)fileData );
	}
	return ALL_OK;
}

void GetTimeOfDayString( String &OutTimeOfDay, int hour, int minute, int second, char separator /*= '-'*/ )
{
	mxASSERT( hour >= 0 && hour <= 23 );
	mxASSERT( minute >= 0 && minute <= 59 );
	mxASSERT( second >= 0 && second <= 59 );
	//mxASSERT( OutTimeOfDay.IsEmpty() );

	String32	szHour;
	Str::SetInt( szHour, hour );
	
	String32	szMinute;
	Str::SetInt( szMinute, minute );

	String32	szSecond;
	Str::SetInt( szSecond, second );

	if( hour < 10 ) {
		Str::Append( OutTimeOfDay, '0' );
	}
	Str::Append( OutTimeOfDay, szHour );
	Str::Append( OutTimeOfDay, separator );

	if( minute < 10 ) {
		Str::Append( OutTimeOfDay, '0' );
	}
	Str::Append( OutTimeOfDay, szMinute );
	Str::Append( OutTimeOfDay, separator );

	if( second < 10 ) {
		Str::Append( OutTimeOfDay, '0' );
	}
	Str::Append( OutTimeOfDay, szSecond );
}

void GetDateString( String &OutCurrDate, int year, int month, int day, char separator /*= '-'*/ )
{
	mxASSERT( year >= 0 && year <= 9999 );
	mxASSERT( month >= 1 && month <= 12 );
	mxASSERT( day >= 1 && day <= 31 );
	//mxASSERT( OutCurrDate.IsEmpty() );

	String32	szYear;
	Str::SetInt( szYear, year );

	String32	szMonth;
	Str::SetInt( szMonth, month );

	String32	szDay;
	Str::SetInt( szDay, day );

	Str::Append( OutCurrDate, szYear );
	Str::Append( OutCurrDate, separator );

	if( month < 10 ) {
		Str::Append( OutCurrDate, '0' );
	}
	Str::Append( OutCurrDate, szMonth );
	Str::Append( OutCurrDate, separator );

	if( day < 10 ) {
		Str::Append( OutCurrDate, '0' );
	}
	Str::Append( OutCurrDate, szDay );
}

void GetDateTimeOfDayString(
	String &OutDateTime,
	const CalendarTime& dateTime /*= CalendarTime::GetCurrentLocalTime()*/,
	char separator /*= '-'*/ )
{
	GetTimeOfDayString( OutDateTime, dateTime.hour, dateTime.minute, dateTime.second, separator );
	Str::Append( OutDateTime, '_' );
	GetDateString( OutDateTime, dateTime.year, dateTime.month, dateTime.day, separator );
}

//----------------------------------------------------------//
//				End Of File.									//
//----------------------------------------------------------//
