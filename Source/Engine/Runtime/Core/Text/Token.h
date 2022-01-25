/*
=============================================================================
	File:	TbToken.h
	Desc:	
	ToDo:	replace functions with their *2 variants
=============================================================================
*/
#pragma once

#include <Core/Text/NameTable.h>

class RGBAf;

enum ETokenType
{
	TT_Bad,			// unknown token or EoF
	TT_Name,		// arbitrary identifier
	TT_Number,		// any number (int, float)
	TT_String,		// "string" constant
	TT_Literal,		// literal 'character' constant
	TT_Punctuation,	// ...
	TT_Directive,	// e.g.: #line, #pragma
	//TT_Keyword,	// reserved word
};
mxDECLARE_ENUM( ETokenType, U32, TokenTypeT );


enum ETokenFlags
{
	// number sub types
	TF_Integer	= BIT(0),	// integer
	TF_Float	= BIT(1),	// floating point number

	// integer number flags
	TF_Long		= BIT(2),	// long int
	TF_Unsigned	= BIT(3),	// unsigned int
	TF_Decimal	= BIT(4),	// decimal number
	TF_Binary	= BIT(5),	// binary number
	TF_Octal	= BIT(6),	// octal number
	TF_Hex		= BIT(7),	// hexadecimal number

	// floating-point number flags
	TF_Double_Precision	 = BIT(8)	// double
};
mxDECLARE_FLAGS( ETokenFlags, U32, TokenFlags );

/*
-----------------------------------------------------------------------------
	Location contains file-line number info
	which is used for tracking position in a source file.
-----------------------------------------------------------------------------
*/
struct Location
{
	NameID	file;
	int		line : 24;
	int		column : 8;
public:
	Location()
	{
		line = 1;
		column = 1;
	}
};

/*
-----------------------------------------------------------------------------
	TbToken

	The TbToken class is used to represent a single lexed token,
	it is a single lexeme from a source file.

	Tokens are intended to be used by the lexer/preprocess and parser libraries,
	but are not intended to live beyond them
	(for example, they should not live in the ASTs).

// NOTE: TbToken can be used in retail builds for parsing INI files
// and we'd better avoid strings with dynamic memory allocation
-----------------------------------------------------------------------------
*/
struct TbToken
{
	String64	text;	// raw text of this token
	TokenTypeT	type;	// the actual flavor of token this is ( TT_* )
	TokenFlags	flags;
	Location	location;	// position of this token in a source file

public:
	TbToken();
	TbToken( const TbToken& other );
	~TbToken();

	TbToken& operator = ( const TbToken &other );

	bool operator == ( const TbToken& other ) const;
	bool operator != ( const TbToken& other ) const;

	template< UINT N >
	bool operator == ( const char (&s)[N] ) const {
		return strcmp( text.c_str(), s ) == 0;
	}
	template< UINT N >
	bool operator != ( const char (&s)[N] ) const {
		return !(*this == s);
	}

	bool operator == ( const char c ) const {
		return text.c_str()[0] == c && text.c_str()[1] == '\0';
	}
	bool operator != ( const char c ) const {
		return !(*this == c);
	}

	// returns a *read-only* null-terminated string
	mxFORCEINLINE const char* c_str() const	{ return this->text.c_str(); }

public:
	bool GetIntegerValue( int *_value ) const;
	bool GetFloatValue( float *_value ) const;

	int GetIntegerValue() const;
	float GetFloatValue() const;

	bool IsSign() const { return type == TT_Punctuation && Str::EqualC(text,'-'); }
};

template<>
struct THashTrait< TbToken >
{
	mxFORCEINLINE static UINT ComputeHash32( const TbToken& token )
	{
		return FNV32_StringHash( token.text.c_str() );
	}
};
template<>
struct TEqualsTrait< TbToken >
{
	mxFORCEINLINE static bool Equals( const TbToken& a, const TbToken& b )
	{
		return (a == b);
	}
};

typedef TArray< TbToken > TokenList;

template< UINT BUFFER_SIZE >
inline
void FormatTextMessageV( char (&buffer)[BUFFER_SIZE], const Location& location, const char* format, va_list args )
{
	char temp[ 4096 ];
	const int count = vsnprintf_s( temp, mxCOUNT_OF(temp), mxCOUNT_OF(temp) - 1, format, args );
	mxUNUSED(count);

	//NOTE: formatted for visual studio debug output (click -> go to source location)
	sprintf(
		buffer, mxCOUNT_OF(buffer),
		"%s(%d,%d): %s\n",
		location.file.c_str(), location.line, location.Column(), temp
		);
}


void Lex_PrintF(
				const Location& location,
				const char* format, ...
				);
void Lex_PrintV(
				const Location& location,
				const char* format, va_list args,
				const char* additional_info = NULL
				);

// emit sound signal
void Lex_ErrorBeep();


#define tbLEXER_ERROR_BRK( LEXER, ... )		(LEXER).Error( __VA_ARGS__ );mxASSERT(false)
#define tbLEXER_WARNING_BRK( LEXER, ... )	(LEXER).Warning( __VA_ARGS__ );mxASSERT(false)


/*
-----------------------------------------------------------------------------
	ATokenWriter
-----------------------------------------------------------------------------
*/
struct ATokenWriter
{
	virtual void AddToken( const TbToken& newToken ) = 0;

protected:
	virtual ~ATokenWriter() {}
};

/*
-----------------------------------------------------------------------------
	TokenStream
-----------------------------------------------------------------------------
*/
struct TokenStream: public ATokenWriter
{
	TArray< TbToken >	m_tokens;

public:
	void empty();

	//=-- ATokenWriter
	virtual void AddToken( const TbToken& newToken ) override;

	bool DumpToFile( const char* filename, bool bHumanReadable = false, const char* prefix = NULL ) const;
	void Dump( AWriter & stream ) const;
	void Dump_HumanReadable( AWriter & stream ) const;

	//void IncrementLineNumbers( UINT incByAmount );
};

/*
-----------------------------------------------------------------------------
	ATokenReader
-----------------------------------------------------------------------------
*/
struct ATokenReader
{
	virtual bool ReadToken( TbToken &newToken ) = 0;
	virtual bool PeekToken( TbToken &nextToken ) = 0;
	virtual bool EndOfFile() const = 0;
	virtual Location GetLocation() const = 0;

	virtual void Debug( const char* format, ... ) const;
	virtual void Message( const char* format, ... ) const;
	virtual void Warning( const char* format, ... ) const;
	virtual void Error( const char* format, ... ) const;

	// for debugging
	virtual bool IsValid() const { return true; }

protected:
	virtual ~ATokenReader() {}
};

/*
-----------------------------------------------------------------------------
	TokenListReader
-----------------------------------------------------------------------------
*/
class TokenListReader: public ATokenReader
{
	const TbToken *	m_tokenList;
	const UINT		m_numTokens;
	UINT			m_currToken;

public:
	typedef ATokenReader Super;

	TokenListReader( const TokenStream& source );
	TokenListReader( const TbToken* tokens, UINT numTokens );

	virtual bool ReadToken( TbToken &nextToken ) override;
	virtual bool PeekToken( TbToken &outToken ) override;
	virtual bool EndOfFile() const override;
	virtual Location GetLocation() const override;

	virtual bool IsValid() const override;

	const TbToken& CurrentToken() const;

	struct State
	{
		UINT currToken;
	};
	void SaveState( State & state ) const;
	void RestoreState( const State& state );

	virtual void Error( const char* format, ... ) const override;
};

bool SkipToken( ATokenReader& lexer );
TokenTypeT PeekTokenType( ATokenReader& lexer );
bool PeekTokenChar( ATokenReader& lexer, const char tokenChar );
bool PeekTokenString( ATokenReader& lexer, const char* tokenString );
bool ExpectTokenType( ATokenReader& lexer, TokenTypeT _type, TbToken &_token );

bool ExpectAnyToken( ATokenReader& lexer, TbToken &_token );
ERet expectAnyToken( TbToken & token_, ATokenReader & lexer );

ERet peekNextToken( TbToken & token_, ATokenReader & lexer );

ERet expectChar( ATokenReader& lexer, char c ) ;
inline bool ExpectString( ATokenReader& lexer, TbToken &token ) {
	return ExpectTokenType( lexer, TT_String, token );
}
inline bool ExpectIdentifier( ATokenReader& lexer, TbToken &token ) {
	return ExpectTokenType( lexer, TT_Name, token );
}

// expect a certain token, reads the token when available
bool Expect( ATokenReader& lexer, const char* tokenString );

ERet expectTokenString( ATokenReader& lexer, const char* _text );
ERet expectTokenChar( ATokenReader& lexer, const char _char );
ERet SkipBracedSection( ATokenReader& lexer );

ERet SkipUntilClosingBrace( ATokenReader& lexer );

/// '{' flag1 flag2 ... '}'
ERet ParseFlags( ATokenReader& lexer,
				const MetaFlags& _type, MetaFlags::ValueT &_flags );

bool expectBool( ATokenReader& lexer, bool &value );

bool expectU8( ATokenReader& lexer, U8 &value, UINT _min = 0, UINT _max = MAX_UINT8 );

bool expectUInt( ATokenReader& lexer, UINT & value, UINT _min = 0, UINT _max = MAX_UINT32 );

bool expectInt( ATokenReader& lexer, TbToken & _token, int & value, int _min = MIN_INT32, int _max = MAX_INT32 );
bool expectInt( ATokenReader& lexer, int & _value, int _min = MIN_INT32, int _max = MAX_INT32 );
int expectInt( ATokenReader& lexer, int _min = MIN_INT32, int _max = MAX_INT32 );

ERet expectInt32(
				 ATokenReader& lexer,
				 TbToken &_token,
				 int &_value,
				 int _min = MIN_INT32, int _max = MAX_INT32
				 );

ERet ExpectUInt32(
				  ATokenReader& lexer,
				  TbToken &_token,
				  U32 &_value,
				  U32 _max = MAX_UINT32
				  );

ERet ExpectFloat(
				 ATokenReader& lexer,
				 TbToken &_token,
				 float &_value,
				 float _min = -1e6f, float _max = +1e6f
				 );
ERet ExpectFloat(
				 ATokenReader& lexer,
				 float &_value,
				 float _min = -1e6f, float _max = +1e6f
				 );

bool expectFloat( ATokenReader& lexer, TbToken &token, float & value, float _min = -1e6f, float _max = +1e6f );
bool expectFloat( ATokenReader& lexer, float &value, float _min = -1e6f, float _max = +1e6f );
float expectFloat( ATokenReader& lexer, float _min = -1e6f, float _max = +1e6f );

ERet expectFloat2(
				  float *value_
				  , ATokenReader & lexer
				  , float min_value = -1e6f
				  , float max_value = +1e6f
				  );

//expectIdentifier
ERet expectName( ATokenReader& lexer, String &tokenString );

ERet expectString( ATokenReader& lexer, String &string_literal_ );

bool expectRGBA( ATokenReader& lexer, float * value );
bool expectRGBA( ATokenReader& lexer, RGBAf & value );

ERet readOptionalChar( ATokenReader& lexer, char tokenChar );
ERet readOptionalSemicolon( ATokenReader& lexer );

ERet parseVec2D( ATokenReader & lexer, float *xy_ );
ERet parseVec3D( ATokenReader & lexer, float *xyz_ );
ERet parseVec4D( ATokenReader & lexer, float *xyzw_ );

// parse matrices with floats
int Parse1DMatrix( ATokenReader& lexer, int x, float *m );
int Parse2DMatrix( ATokenReader& lexer, int y, int x, float *m );
int Parse3DMatrix( ATokenReader& lexer, int z, int y, int x, float *m );

ERet Parse1DMatrix2( ATokenReader& lexer, int _size_X, float *_m );


ERet parseEnumValue(
					ATokenReader& lexer,
					const MetaEnum& metaEnum, MetaEnum::ValueT *enumValue_
					);

template< typename ENUM_TYPE, typename STORAGE >
ERet parseEnum( ATokenReader& lexer,
				TEnum< ENUM_TYPE, STORAGE > *enum_ )
{
	const TbMetaType& metaType = TypeDeducer< TEnum< ENUM_TYPE, STORAGE > >::GetType();
	const MetaEnum& metaEnum = metaType.UpCast<MetaEnum>();

	MetaEnum::ValueT	enumValue;
	mxDO(parseEnumValue( lexer, metaEnum, &enumValue ));

	*enum_ = static_cast< ENUM_TYPE >( enumValue );

	return ALL_OK;
}
