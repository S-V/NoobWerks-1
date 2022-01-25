/*
=============================================================================
	File:	TbToken.cpp
	Desc:
=============================================================================
*/
#include <Core/Core_PCH.h>
#pragma hdrstop

// for strtoul()
#include <cstdlib>

#include <Base/Util/Color.h>
#include <Core/Text/Token.h>
#include <Core/Text/TextWriter.h>

mxBEGIN_REFLECT_ENUM( TokenTypeT )
	mxREFLECT_ENUM_ITEM( Bad, ETokenType::TT_Bad ),
	mxREFLECT_ENUM_ITEM( Name, ETokenType::TT_Name ),
	mxREFLECT_ENUM_ITEM( Number, ETokenType::TT_Number ),
	mxREFLECT_ENUM_ITEM( String, ETokenType::TT_String ),
	mxREFLECT_ENUM_ITEM( Literal, ETokenType::TT_Literal ),
	mxREFLECT_ENUM_ITEM( Punctuation, ETokenType::TT_Punctuation ),
	mxREFLECT_ENUM_ITEM( Directive, ETokenType::TT_Directive ),
mxEND_REFLECT_ENUM

#if 0
const char* ETokenKind_To_Chars( TokenTypeT _type )
{
	switch( _type )
	{
	case TC_Bad:	return "Bad";
	case TT_Number:	return "integer";
	case TC_FloatNumber:	return "float";
	case TC_Char:	return "char";
	case TT_String:	return "string";
	case TT_Name:	return "identifier";
	case TC_Punctuation:	return "punctuation";
	case TT_Directive:	return "directive";
		mxNO_SWITCH_DEFAULT;
	}
}
#endif

/*
-----------------------------------------------------------------------------
	Location
-----------------------------------------------------------------------------
*/

/*
-----------------------------------------------------------------------------
	TbToken
-----------------------------------------------------------------------------
*/
TbToken::TbToken()
{
	type = TT_Bad;
	flags.raw_value = 0;
}

TbToken::TbToken( const TbToken& other )
{
	*this = other;
}

TbToken::~TbToken()
{
}

TbToken& TbToken::operator = ( const TbToken &other )
{
	text		= other.text;
	type		= other.type;
	//m_loc		= other.m_loc;
	//m_numberType= other.m_numberType;
	return *this;
}
//
//TokenTypeT TbToken::GetKind() const
//{
//	return m_kind;
//}
//
//const char* TbToken::Raw() const
//{
//	return text.raw();
//}
//
//const UINT TbToken::Length() const
//{
//	return this->GetText().length();
//}
//
////const TokenFlags TbToken::GetFlags() const
////{
////	return m_flags;
////}
//
//const ENumberType TbToken::GetNumberType() const
//{
//	return m_numberType;
//}
//
//const String& TbToken::GetText() const
//{
//	return text;
//}
//
//const Location& TbToken::GetLocation() const
//{
//	return m_loc;
//}

bool TbToken::operator == ( const TbToken& other ) const
{
	return text == other.text
		&& type == other.type
		;
}

bool TbToken::operator != ( const TbToken& other ) const
{
	return !(*this == other);
}

bool TbToken::GetIntegerValue( int *_value ) const
{
	mxASSERT_PTR(_value);
	chkRET_FALSE_IF_NOT(type == TT_Number && (flags & TF_Integer) );

	//if( m_numberType == TF_Decimal )
	{
		const long int longIntValue = ::atol( this->text.c_str() );
		*_value = longIntValue;
	}
	//else if( m_numberType == TF_Hex )
	//{
	//	const unsigned long ulongValue = strtoul( this->raw(), NULL, 16 );
	//	*_value = (int)ulongValue;
	//}

	return true;
}

bool TbToken::GetFloatValue( float *_value ) const
{
	chkRET_FALSE_IF_NIL(_value);
	chkRET_FALSE_IF_NOT(type == TT_Number && (flags & TF_Float) );

	const double floatValue = ::atof( this->text.c_str() );
	*_value = floatValue;

	return true;
}

int TbToken::GetIntegerValue() const
{
	int result = ::atoi( text.raw() );
	return result;
}

float TbToken::GetFloatValue() const
{
	double result = ::atof( text.raw() );
	return result;
}

void Lex_PrintF(
				const Location& location,
				const char* format, ...
				)
{
	va_list	args;
	va_start( args, format );
	Lex_PrintV( location, format, args );
	va_end( args );
}

void Lex_PrintV(
				const Location& location,
				const char* format, va_list args,
				const char* additional_info
				)
{
	ptPRINT_VARARGS_BODY(
		format, args,
		//NOTE: formatted for visual studio debug output (click -> go to source location)
		ptPRINT("%s(%d,%d): %s%s\n", location.file.c_str(), location.line, location.column, additional_info ? additional_info : "", ptr_)
		);
}

void Lex_ErrorBeep()
{
	mxBeep(200);
}

///*
//-----------------------------------------------------------------------------
//	TokenStringWriter
//-----------------------------------------------------------------------------
//*/
//TokenStringWriter::TokenStringWriter( String &destination )
//	: m_dest( destination )
//{
//
//}
//
//void TokenStringWriter::AddToken( const TbToken& newToken )
//{
//	m_dest.Append( newToken.raw(), newToken.length() );
//}

/*
-----------------------------------------------------------------------------
	TokenStream
-----------------------------------------------------------------------------
*/
static void InsertLineDirective( const TbToken& token, AWriter & stream )
{
	String256	directive;
	Str::Format(directive, "#line %u \"%s\"\n", token.location.line, token.location.file.c_str());
	//Str::Format(directive, "#line %u\n", loc.Line());	//GLSL doesn't support file names in #line
	stream.Write( directive.raw(), directive.length() );
}

static void PutToken( const TbToken& token, AWriter & stream )
{
	const U32 tokenLength = token.text.length();
	if( tokenLength > 0 ) {
		stream.Write( token.text.c_str(), tokenLength );
	}
}

void TokenStream::Dump( AWriter & stream ) const
{
	for( UINT iToken = 0; iToken < m_tokens.num(); iToken++ )
	{
		const TbToken& token = m_tokens[ iToken ];

		mxASSERT(token.text.length() > 0);

		PutToken(token, stream);

		stream.Write(" ",1);
	}
}

void TokenStream::Dump_HumanReadable( AWriter & stream ) const
{
	TextWriter	writer( stream );

	UINT prevLine = 1;
mxTIDY_UP_FILE;
	for( UINT iToken = 0; iToken < m_tokens.num(); iToken++ )
	{
		const TbToken& token = m_tokens[ iToken ];
		mxASSERT(token.text.length() > 0);

		const UINT currLine = token.location.line;
		if( currLine != prevLine )
		{
			writer.NewLine();
			//InsertLineDirective(token, stream);
			prevLine = currLine;
		}

		//char nextTokenChar = '\0';
		//if( iToken < m_tokens.num()-1 )
		//{
		//	const TbToken& nextToken = m_tokens[ iToken + 1 ];
		//	nextTokenChar = nextToken.GetText()[0];
		//}

		//writer.Putf("TbToken: '%s' (%u)\n", token.text.c_str(), token.text.length());
		PutToken(token, writer.GetStream());

		//const UINT tokenLength = token.text.length();
		//if( tokenLength > 0 )
		//{
		//	writer.InsertTabs();
		//	stream.Write( token.text.c_str(), tokenLength );
		//}
		//if( token == '{' )
		//{
		//	writer.IncreaseIndentation();
		//}
		//else if( token == '}' )
		//{
		//	writer.DecreaseIndentation();
		//	//writer.NewLine();
		//}
		////if( token.location.Column() + token.text.length() )
		//else
			writer.Emit(" ",1);

//		if( token.type == TT_Directive )
//		{
//			writer.NewLine();
//		}
//		else if( token == '{' )
//		{
//			writer.NewLine();
//		//	InsertLineDirective(token, stream);
//			writer.IncreaseIndentation();
//			//writer.Putf("");
//		}
//		else if( token == '}' )
//		{
//			writer.DecreaseIndentation();
//			writer.NewLine();
//	//		InsertLineDirective(token, stream);
//			//writer.DecreaseIndentation();
//		}
//		else if( token == ';' )
//		{
//			writer.NewLine();
////			InsertLineDirective(token, stream);
//			//if( nextTokenChar != '{' )
//			//{
//			//	writer.InsertTabs();
//			//}
//		}
//		else
//		{
//			//if( token.type == TC_Punctuation )
//			//{
//			//	Lex_PrintF(token.location, "writing whitespace after '%s'",token.text.c_str());
//			//}
//			writer.Emit(" ",1);
//		}
	}
}

void TokenStream::empty()
{
	m_tokens.RemoveAll();
}

void TokenStream::AddToken( const TbToken& newToken )
{
	mxTODO("compare locations of the last tokens? define operator < (SourceLoc a,b) ?");
	m_tokens.add( newToken );
}

bool TokenStream::DumpToFile( const char* filename, bool bHumanReadable, const char* prefix ) const
{
	FileWriter	file( filename );
	chkRET_FALSE_IF_NOT(file.IsOpen());

	if( prefix != NULL )
	{
		const UINT len = strlen( prefix );
		if( len > 0 ) {
			file.Write( prefix, len );
		}
	}

	if( bHumanReadable ) {
		this->Dump_HumanReadable( file );
	} else {
		this->Dump( file );
	}

	return true;
}

/*
-----------------------------------------------------------------------------
	ATokenReader
-----------------------------------------------------------------------------
*/
void ATokenReader::Debug( const char* format, ... ) const
{
#if MX_DEBUG
	va_list	args;
	va_start( args, format );
	Lex_PrintV( this->GetLocation(), format, args );
	va_end( args );
#endif // MX_DEBUG
}

void ATokenReader::Message( const char* format, ... ) const
{
	va_list	args;
	va_start( args, format );
	Lex_PrintV( this->GetLocation(), format, args );
	va_end( args );
}

void ATokenReader::Warning( const char* format, ... ) const
{
	va_list	args;
	va_start( args, format );
	Lex_PrintV( this->GetLocation(), format, args, "warning: " );
	va_end( args );
	//Lex_ErrorBeep();
}

void ATokenReader::Error( const char* format, ... ) const
{
	va_list	args;
	va_start( args, format );
	Lex_PrintV( this->GetLocation(), format, args, "error: " );
	va_end( args );
	Lex_ErrorBeep();
}

/*
-----------------------------------------------------------------------------
	TokenListReader
-----------------------------------------------------------------------------
*/
TokenListReader::TokenListReader( const TokenStream& source )
	: m_tokenList( source.m_tokens.raw() ), m_numTokens( source.m_tokens.num() )
{
	mxASSERT(m_numTokens > 0);
	m_currToken = 0;
}

TokenListReader::TokenListReader( const TbToken* tokens, UINT numTokens )
	: m_tokenList( tokens ), m_numTokens( numTokens )
{
	mxASSERT(numTokens > 0);
	m_currToken = 0;
}

bool TokenListReader::ReadToken( TbToken &nextToken )
{
	if( m_currToken < m_numTokens )
	{
		nextToken = m_tokenList[ m_currToken++ ];
		//DBGOUT("ReadToken: %s\n", outToken.raw());
		return true;
	}
	return false;
}

bool TokenListReader::PeekToken( TbToken &outToken )
{
	State	oldState;
	this->SaveState(oldState);
	bool bOk = this->ReadToken(outToken);
	this->RestoreState(oldState);
	return bOk;
}

bool TokenListReader::EndOfFile() const
{
	return (m_currToken >= m_numTokens);
}

Location TokenListReader::GetLocation() const
{
	return this->CurrentToken().location;
}

bool TokenListReader::IsValid() const
{
	return (m_numTokens > 0) && !this->EndOfFile();
}

const TbToken& TokenListReader::CurrentToken() const
{
	if( EndOfFile() )
	{
		// return the last token
		return m_tokenList[ m_numTokens-1 ];
	}
	mxASSERT(m_currToken < m_numTokens);
	return m_tokenList[ m_currToken ];
}

void TokenListReader::SaveState( State &state ) const
{
	state.currToken = m_currToken;
}

void TokenListReader::RestoreState( const State& state )
{
	m_currToken = state.currToken;
}

void TokenListReader::Error( const char* format, ... ) const
{
	this->Debug("current token: '%s'", this->CurrentToken().text.c_str());
	char	buffer[4096];
	mxGET_VARARGS_A(buffer, format);
	Super::Error( buffer );
}


bool SkipToken( ATokenReader& lexer )
{
	TbToken	dummyToken;
	return lexer.ReadToken( dummyToken );
}
TokenTypeT PeekTokenType( ATokenReader& lexer )
{
	TbToken	nextToken;
	lexer.PeekToken( nextToken );
	return nextToken.type;
}
bool PeekTokenChar( ATokenReader& lexer, const char tokenChar )
{
	TbToken	nextToken;
	chkRET_FALSE_IF_NOT( lexer.PeekToken( nextToken ) );
	return nextToken == tokenChar;
}
bool PeekTokenString( ATokenReader& lexer, const char* tokenString )
{
	TbToken	nextToken;
	RET_FALSE_IF_NOT( lexer.PeekToken( nextToken ) );
	return Str::EqualS( nextToken.text, tokenString );
}
bool ExpectTokenType( ATokenReader& lexer, TokenTypeT _type, TbToken &_token )
{
	if ( !ExpectAnyToken( lexer, _token ) ) {
		return false;
	}
	if( _token.type != _type )
	{
		tbLEXER_ERROR_BRK(lexer, "expected '%s' but found '%s'",
			TokenTypeT_Type().FindString(_type), _token.text.c_str() );
		return false;
	}
	return true;
}
bool ExpectAnyToken( ATokenReader& lexer, TbToken &_token )
{
	if ( !lexer.ReadToken( _token ) ) {
		tbLEXER_ERROR_BRK(lexer, "couldn't read expected token" );
		return false;
	}
	return true;
}

ERet expectAnyToken( TbToken & token_, ATokenReader & lexer )
{
	if ( !lexer.ReadToken( token_ ) ) {
		tbLEXER_ERROR_BRK(lexer, "couldn't read expected token" );
		return ERR_EOF_REACHED;
	}
	return ALL_OK;
}

ERet peekNextToken( TbToken & token_, ATokenReader & lexer )
{
	return lexer.PeekToken( token_ ) ? ALL_OK : ERR_EOF_REACHED;
}

ERet expectChar( ATokenReader& lexer, char c )
{
	TbToken	token;
	if( !lexer.ReadToken( token ) ) {
		tbLEXER_ERROR_BRK(lexer, "couldn't read expected '%c' : end of file reached", c);
		return ERR_EOF_REACHED;
	}

	if( token == c ) {
		return ALL_OK;
	}

	//tbLEXER_ERROR_BRK(*this, "expected a character constant '%c', but read '%s'", c, token.text.c_str());
	tbLEXER_ERROR_BRK(lexer, "expected '%c', but read '%s'", c, token.text.c_str());

	return ERR_UNEXPECTED_TOKEN;
}
#if 0
bool ExpectString( Token &token )
{
	if( !this->ReadToken( token ) )
	{
		tbLEXER_ERROR_BRK(*this, "couldn't read expected string");
		return false;
	}
	if( token.type != TT_String )
	{
		tbLEXER_ERROR_BRK(*this, "expected a string literal, but read '%s'", token.text.c_str());
		return false;
	}
	return true;
}

bool ExpectInt( Token &token )
{
	if( !this->ReadToken( token ) )
	{
		tbLEXER_ERROR_BRK(*this, "couldn't read expected integer number");
		return false;
	}

	if ( token.type == TC_Punctuation && token == "-" ) {
		this->ExpectTokenType( TT_Number, &token );
		return -((signed int) token.GetIntegerValue());
	}

	if( token.type != TT_Number )
	{
		tbLEXER_ERROR_BRK(*this, "expected an integer number, but read '%s'", token.text.c_str());
		return false;
	}
	return true;
}

bool ExpectFloat( Token &token )
{
	if( !this->ReadToken( token ) )
	{
		tbLEXER_ERROR_BRK(*this, "couldn't read expected floating-point number");
		return false;
	}
	if( token.type != TC_FloatNumber )
	{
		tbLEXER_ERROR_BRK(*this, "expected an floating-point number, but read '%s'", token.text.c_str());
		return false;
	}
	return true;
}

bool ExpectConstant( Token &token )
{
	if( !this->ReadToken( token ) )
	{
		tbLEXER_ERROR_BRK(*this, "couldn't read expected constant");
		return false;
	}
	if( !TokenIsConstant( token ) )
	{
		tbLEXER_ERROR_BRK(*this, "expected a constant, but read '%s'", token.text.c_str());
		return false;
	}
	return true;
}
#endif

bool Expect( ATokenReader& lexer, const char* tokenString )
{
	TbToken	token;
	if( !lexer.ReadToken( token ) )
	{
		tbLEXER_ERROR_BRK(lexer, "couldn't read expected '%s'", tokenString);
		return false;
	}
	if( !Str::Equal( token.text, Chars(tokenString,_InitSlow) ) )
	{
		tbLEXER_ERROR_BRK(lexer, "expected '%s', but read '%s'", tokenString, token.text.c_str());
		return false;
	}
	return true;
}

ERet expectTokenString( ATokenReader& lexer, const char* _text )
{
	TbToken	token;
	if( !lexer.ReadToken( token ) )
	{
		tbLEXER_ERROR_BRK(lexer, "couldn't read expected '%s'", _text);
		return ERR_EOF_REACHED;
	}
	if( !Str::EqualS( token.text, _text ) )
	{
		tbLEXER_ERROR_BRK(lexer, "expected '%s', but read '%s'", _text, token.text.c_str());
		return ERR_UNEXPECTED_TOKEN;
	}
	return ALL_OK;
}
ERet expectTokenChar( ATokenReader& lexer, const char _char )
{
	TbToken	token;
	if( !lexer.ReadToken( token ) )
	{
		tbLEXER_ERROR_BRK(lexer, "couldn't read expected '%c'", _char);
		return ERR_EOF_REACHED;
	}
	if( !Str::EqualC( token.text, _char ) )
	{
		tbLEXER_ERROR_BRK(lexer, "expected '%c', but read '%s'", _char, token.text.c_str());
		return ERR_UNEXPECTED_TOKEN;
	}
	return ALL_OK;
}

ERet SkipBracedSection( ATokenReader& lexer )
{
	mxDO(expectTokenChar( lexer, '{' ));

	return SkipUntilClosingBrace( lexer );
}

ERet SkipUntilClosingBrace( ATokenReader& lexer )
{
	int depth = 1;

	while( depth > 0 )
	{
		TbToken	token;
		if( !lexer.ReadToken( token ) ) {
			return ERR_EOF_REACHED;
		}
		if( token == '{' ) {
			depth++;
		}
		if( token == '}' ) {
			depth--;
		}
	}
	return ALL_OK;
}

ERet ParseFlags( ATokenReader& lexer, const MetaFlags& _type, MetaFlags::ValueT &_flags )
{
	mxDO(expectTokenChar( lexer, '{' ));

	MetaFlags::ValueT result = 0;

	int depth = 1;

	while( depth > 0 )
	{
		TbToken	token;
		if( !lexer.ReadToken( token ) ) {
			return ERR_EOF_REACHED;
		}
		if( token == '{' ) {
			depth++;
		}
		else if( token == '}' ) {
			depth--;
		}
		else {
			MetaFlags::ValueT mask = 0;
			mxDO(_type.FindValueSafe( token.text.c_str(), &mask ));
			result |= mask;
		}
	}
	_flags = result;
	return ALL_OK;
}

ERet ExpectUInt32(
				  ATokenReader& lexer,
				  TbToken &_token,
				  U32 &_value,
				  U32 _max /*= MAX_UINT32*/
				  )
{
	if ( !ExpectAnyToken( lexer, _token ) ) {
		return ERR_EOF_REACHED;
	}
	else if( _token.type != TT_Number ) {
		lexer.Error("expected an unsigned integer number, but got '%s'", _token.text.c_str());
		return ERR_UNEXPECTED_TOKEN;
	}
	else {
		_value = (U32) _token.GetIntegerValue();
	}

	if( _value > _max )
	{
		lexer.Error("expected an unsigned integer number less than or equal to %u, but got %u", _max, _value);
		return ERR_UNEXPECTED_TOKEN;
	}

	return ALL_OK;
}

ERet ExpectFloat(
				 ATokenReader& lexer,
				 TbToken &_token,
				 float &_value,
				 float _min, float _max
				 )
{
	if ( !ExpectAnyToken( lexer, _token ) ) {
		return ERR_EOF_REACHED;
	}
	if ( _token.IsSign() ) {
		if( !ExpectTokenType( lexer, TT_Number, _token ) ) {
			return ERR_EOF_REACHED;
		}
		_value = -_token.GetFloatValue();
	}
	else if( _token.type != TT_Number ) {
		lexer.Error("expected a floating point number, but got '%s'", _token.text.c_str());
		return ERR_UNEXPECTED_TOKEN;
	}
	else {
		_value = _token.GetFloatValue();
	}
	if( _value < _min || _value > _max )
	{
		lexer.Error("expected a floating point number in range [%f, %f], but got %f", _min, _max, _value);
		return ERR_UNEXPECTED_TOKEN;
	}
	return ALL_OK;
}

ERet ExpectFloat(
				 ATokenReader& lexer,
				 float &_value,
				 float _min, float _max
				 )
{
	TbToken	token;
	return ExpectFloat( lexer, token, _value, _min, _max );
}

bool expectBool( ATokenReader& lexer, bool &value )
{
	TbToken	token;
	chkRET_FALSE_IF_NOT(ExpectAnyToken(lexer, token));

	String64	tokenValue( token.text );
	Str::ToUpper( tokenValue );

	if( Str::Equal(tokenValue, Chars("TRUE")) || token == '1' ) {
		value = true;
		return true;
	}
	if( Str::Equal(tokenValue, Chars("FALSE")) || token == '0' ) {
		value = false;
		return true;
	}

	lexer.Error("expected a boolean value, but got %s", token.text.c_str());
	return false;
}

bool expectFloat( ATokenReader& lexer, float & value, float _min, float _max )
{
	TbToken	token;
	return expectFloat( lexer, token, value, _min, _max );
}

bool expectFloat( ATokenReader& lexer, TbToken &_token, float &_value, float _min, float _max )
{
	if ( !ExpectAnyToken( lexer, _token ) ) {
		return false;
	}
	if ( _token.IsSign() ) {
		if( !ExpectTokenType( lexer, TT_Number, _token ) ) {
			return false;
		}
		_value = -_token.GetFloatValue();
	}
	else if( _token.type != TT_Number ) {
		lexer.Error("expected a floating point number, but got '%s'", _token.text.c_str());
		return false;
	}
	else {
		_value = _token.GetFloatValue();
	}
	if( _value < _min || _value > _max )
	{
		lexer.Error("expected a floating point number in range [%f, %f], but got %f", _min, _max, _value);
		return false;
	}
	return true;
}

float expectFloat( ATokenReader& lexer, float _min, float _max )
{
	float value = 0;
	if( !expectFloat( lexer, value, _min, _max ) ) {
		return 0;
	}
	return value;
}

ERet expectFloat2(
				  float *value_
				  , ATokenReader & lexer
				  , float min_value
				  , float max_value
				  )
{
	TbToken	token;

	if ( !ExpectAnyToken( lexer, token ) ) {
		lexer.Error("expected a floating point number, but reached EOF");
		return ERR_EOF_REACHED;
	}

	float	value;

	if ( token.IsSign() ) {
		if( !ExpectTokenType( lexer, TT_Number, token ) ) {
			lexer.Error("expected a floating point number, but got '%s'", token.text.c_str());
			return ERR_UNEXPECTED_TOKEN;
		}
		value = -token.GetFloatValue();
	}
	else if( token.type != TT_Number ) {
		lexer.Error("expected a floating point number, but got '%s'", token.text.c_str());
		return ERR_UNEXPECTED_TOKEN;
	}
	else {
		value = token.GetFloatValue();
	}

	if( value < min_value || value > max_value )
	{
		lexer.Error(
			"expected a floating point number in range [%f, %f], but got %f",
			min_value, max_value, value
			);
		return ERR_INVALID_PARAMETER;
	}

	*value_ = value;

	return ALL_OK;
}

bool expectUInt( ATokenReader& lexer, UINT & value, UINT _min, UINT _max )
{
	TbToken	token;
	if( !ExpectAnyToken( lexer, token ) ) {
		return false;
	}

	int integerValue;
	const bool bOk = token.GetIntegerValue( &integerValue );

	if( !bOk || integerValue < _min || integerValue > _max )
	{
		lexer.Error("expected an unsigned integer number in range [%u, %u], but got %s", _min, _max, token.text.c_str());
		return false;
	}

	value = (UINT)integerValue;

	return true;
}

bool expectU8( ATokenReader& lexer, U8 &value, UINT _min, UINT _max )
{
	UINT temp;
	chkRET_FALSE_IF_NOT(expectUInt(lexer, temp, _min, _max ));
	value = temp;
	return true;
}

bool expectInt( ATokenReader& lexer, int & value, int _min, int _max )
{
	TbToken	token;
	return expectInt( lexer, token, value, _min, _max );
}

bool expectInt( ATokenReader& lexer, TbToken & _token, int & _value, int _min, int _max )
{
	if ( !ExpectAnyToken( lexer, _token ) ) {
		return false;
	}
	if ( _token.IsSign() ) {
		if( !ExpectTokenType( lexer, TT_Number, _token ) ) {
			return false;
		}
		_value = -_token.GetIntegerValue();
	}
	else if( _token.type != TT_Number ) {
		lexer.Error("expected an integer number, but got '%s'", _token.text.c_str());
		return false;
	}
	else {
		_value = _token.GetIntegerValue();
	}
	if( _value < _min || _value > _max )
	{
		lexer.Error("expected an integer number in range [%i, %i], but got %i", _min, _max, _value);
		return false;
	}
	return true;
}

int expectInt( ATokenReader& lexer, int _min, int _max )
{
	int value = 0;
	if( !expectInt( lexer, value, _min, _max ) ) {
		return 0;
	}
	return value;
}

ERet expectInt32(
				 ATokenReader& lexer
				 , TbToken & _token
				 , int &_value
				 , int _min
				 , int _max
				 )
{
	if ( !ExpectAnyToken( lexer, _token ) )
	{
		return ERR_EOF_REACHED;
	}

	if ( _token.IsSign() )
	{
		if( !ExpectTokenType( lexer, TT_Number, _token ) )
		{
			return ERR_UNEXPECTED_TOKEN;
		}
		_value = -_token.GetIntegerValue();
	}
	else if( _token.type != TT_Number )
	{
		lexer.Error("expected an integer number, but got '%s'", _token.text.c_str());
		return ERR_UNEXPECTED_TOKEN;
	}
	else
	{
		_value = _token.GetIntegerValue();
	}

	if( _value < _min || _value > _max )
	{
		lexer.Error("expected an integer number in range [%i, %i], but got %i", _min, _max, _value);
		return ERR_UNEXPECTED_TOKEN;
	}

	return ALL_OK;
}


ERet expectName( ATokenReader& lexer, String &tokenString )
{
	TbToken	token;
	chkRET_X_IF_NOT(ExpectIdentifier( lexer, token ), ERR_UNEXPECTED_TOKEN);

	mxASSERT(token.text.length() > 0);

	tokenString.Copy( token.text );

	return ALL_OK;
}

ERet expectString( ATokenReader& lexer, String &string_literal_ )
{
	TbToken	token;
	chkRET_X_IF_NOT(ExpectString( lexer, token ), ERR_UNEXPECTED_TOKEN);

	mxDO(string_literal_.Copy( token.text ));

	return ALL_OK;
}

bool expectRGBA( ATokenReader& lexer, float * value )
{
	chkRET_FALSE_IF_NOT(Expect(lexer, "RGBA"));
	chkRET_FALSE_IF_NOT(expectChar(lexer, '(') != ALL_OK);

	chkRET_FALSE_IF_NOT(expectFloat( lexer, value[0], 0.0f, 1.0f ));
	chkRET_FALSE_IF_NOT(expectFloat( lexer, value[1], 0.0f, 1.0f ));
	chkRET_FALSE_IF_NOT(expectFloat( lexer, value[2], 0.0f, 1.0f ));
	chkRET_FALSE_IF_NOT(expectFloat( lexer, value[3], 0.0f, 1.0f ));

	chkRET_FALSE_IF_NOT(expectChar(lexer, ')') != ALL_OK);

	return true;
}

bool expectRGBA( ATokenReader& lexer, RGBAf & value )
{
	return expectRGBA( lexer, value.raw() );
}

ERet readOptionalChar( ATokenReader& lexer, char tokenChar )
{
	TbToken	nextToken;

	if( lexer.PeekToken( nextToken ) )
	{
		if( nextToken == tokenChar )
		{
			SkipToken( lexer );
		}
		return ALL_OK;
	}

	// end of file reached
	return ERR_EOF_REACHED;
}

ERet readOptionalSemicolon( ATokenReader& lexer )
{
	return readOptionalChar( lexer, ';' );
}

ERet parseVec2D( ATokenReader & lexer, float *xy_ )
{
	mxDO(expectTokenChar( lexer, '(' ));
	xy_[ 0 ] = expectFloat( lexer );
	xy_[ 1 ] = expectFloat( lexer );
	mxDO(expectTokenChar( lexer, ')' ));
	return ALL_OK;
}

ERet parseVec3D( ATokenReader & lexer, float *xyz_ )
{
	mxDO(expectTokenChar( lexer, '(' ));
	xyz_[ 0 ] = expectFloat( lexer );
	xyz_[ 1 ] = expectFloat( lexer );
	xyz_[ 2 ] = expectFloat( lexer );
	mxDO(expectTokenChar( lexer, ')' ));
	return ALL_OK;
}

ERet parseVec4D( ATokenReader & lexer, float *xyzw_ )
{
	mxDO(expectTokenChar( lexer, '(' ));
	xyzw_[ 0 ] = expectFloat( lexer );
	xyzw_[ 1 ] = expectFloat( lexer );
	xyzw_[ 2 ] = expectFloat( lexer );
	xyzw_[ 3 ] = expectFloat( lexer );
	mxDO(expectTokenChar( lexer, ')' ));
	return ALL_OK;
}

int Parse1DMatrix( ATokenReader& lexer, int x, float *m ) {
	int i;

	if ( !Expect(lexer,  "(" ) ) {
		return false;
	}

	for ( i = 0; i < x; i++ ) {
		m[i] = expectFloat( lexer );
	}

	if ( !Expect(lexer,  ")" ) ) {
		return false;
	}
	return true;
}

ERet Parse1DMatrix2( ATokenReader& lexer, int _size_X, float *_m )
{
	mxDO(expectTokenChar( lexer, '(' ));

	for( int i = 0; i < _size_X; i++ ) {
		mxDO(ExpectFloat( lexer, _m[i] ));
	}

	mxDO(expectTokenChar( lexer, ')' ));
	return ALL_OK;
}

int Parse2DMatrix( ATokenReader& lexer, int y, int x, float *m ) {
	int i;

	if ( !Expect(lexer,  "(" ) ) {
		return false;
	}

	for ( i = 0; i < y; i++ ) {
		if ( !Parse1DMatrix( lexer, x, m + i * x ) ) {
			return false;
		}
	}

	if ( !Expect(lexer,  ")" ) ) {
		return false;
	}
	return true;
}

int Parse3DMatrix( ATokenReader& lexer, int z, int y, int x, float *m ) {
	int i;

	if ( !Expect(lexer,  "(" ) ) {
		return false;
	}

	for ( i = 0 ; i < z; i++ ) {
		if ( !Parse2DMatrix( lexer, y, x, m + i * x*y ) ) {
			return false;
		}
	}

	if ( !Expect(lexer,  ")" ) ) {
		return false;
	}
	return true;
}


ERet parseEnumValue(
					ATokenReader& lexer,
					const MetaEnum& metaEnum, MetaEnum::ValueT *enumValue_
					)
{
	TbToken	token;
	chkRET_X_IF_NOT(ExpectIdentifier( lexer, token ), ERR_UNEXPECTED_TOKEN);

	mxDO(metaEnum.FindValueSafe( token.text.c_str(), enumValue_ ));

	return ALL_OK;
}

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
