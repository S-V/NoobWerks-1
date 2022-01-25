/*
=============================================================================
	File:	Lexer.cpp
	Desc:	A simple lexicographical text parser (aka scanner, tokenizer).
	ToDo:	optimize (cache class members in local variables)
=============================================================================
*/
#include <Core/Core_PCH.h>
#pragma hdrstop

#include <Core/Text/Lexer.h>



mxSTOLEN("parts stolen from Clang: http://llvm.org/svn/llvm-project/cfe/trunk/lib/Lex/Lexer.cpp");

enum {
	// big enough for storing most string constants
	MAX_TOKEN_LENGTH = 128,

	TAB_WIDTH_IN_COLUMNS = 4,

	SINGLE_QUOTE = '\'',
};

enum CharFlags
{
	// whitespaces
	CharIsHorzWS	= BIT(0),  // ' ', '\t', '\f', '\v'.  Note, no '\0'
	CharIsVertWS	= BIT(1),  // '\r', '\n'

	CharIsLetter	= BIT(2),	// valid letter of the English alphabet (a-z,A-Z)

	// numbers
	CharIsBinDigit	= BIT(3),	// valid digit of a binary number
	CharIsDecDigit	= BIT(4),	// valid digit of a decimal number
	CharIsOctDigit	= BIT(5),	// valid digit of an octal number
	CharIsHexDigit	= BIT(6),	// valid digit of a hexadecimal number

	CharIsUnderscore= BIT(7),	// '_'
	CharIsPunctuation= BIT(8),	// ';', ':', ',', ...
	CharIsPeriod	= BIT(9),	// '.'

	CharIsPathSeparator	= BIT(10),	// '/', '.'

	//CharIsOperator	= BIT(10),	// '=', '<', '!'

	CharIsAnyDigit	= CharIsBinDigit|CharIsDecDigit|CharIsOctDigit|CharIsHexDigit,
};

enum { NUM_CHARS = 256 };

#if 0
first 32 chars are not printable characters:
	/* 000 nul */	0,
	/* 001 soh */	0,
	/* 002 stx */	0,
	/* 003 etx */	0,
	/* 004 eot */	0,
	/* 005 enq */	0,
	/* 006 ack */	0,
	/* 007 bel */	0,
	/* 010 bs  */	0,
	/* 011 ht  */	BLANK,
	/* 012 nl  */	NEWLINE,
	/* 013 vt  */	BLANK,
	/* 014 ff  */	BLANK,
	/* 015 cr  */	0,
	/* 016 so  */	0,
	/* 017 si  */	0,
	/* 020 dle */	0,
	/* 021 dc1 */	0,
	/* 022 dc2 */	0,
	/* 023 dc3 */	0,
	/* 024 dc4 */	0,
	/* 025 nak */	0,
	/* 026 syn */	0,
	/* 027 etb */	0,
	/* 030 can */	0,
	/* 031 em  */	0,
	/* 032 sub */	0,
	/* 033 esc */	0,
	/* 034 fs  */	0,
	/* 035 gs  */	0,
	/* 036 rs  */	0,
	/* 037 us  */	0,
	/* 040 sp  */	BLANK,
#endif

// this table is generated automatically
static const int gs_CharMap[ NUM_CHARS ] =
{
/* 0  */ 0,
/* 1  */ (0),
/* 2  */ (0),
/* 3  */ (0),
/* 4  */ (0),
/* 5  */ (0),
/* 6  */ (0),
/* 7  */ (0),
/* 8  */ (0),
/* 9  */ (0|CharIsHorzWS),
/* 10 */ (0|CharIsVertWS),
/* 11 */ (0|CharIsHorzWS),
/* 12 */ (0|CharIsHorzWS),
/* 13 */ (0|CharIsVertWS),
/* 14 */ (0),
/* 15 */ (0),
/* 16 */ (0),
/* 17 */ (0),
/* 18 */ (0),
/* 19 */ (0),
/* 20 */ (0),
/* 21 */ (0),
/* 22 */ (0),
/* 23 */ (0),
/* 24 */ (0),
/* 25 */ (0),
/* 26 */ (0),
/* 27 */ (0),
/* 28 */ (0),
/* 29 */ (0),
/* 30 */ (0),
/* 31 */ (0),
/* 32 */ (0|CharIsHorzWS),
/* 33 ! */ (0|CharIsPunctuation),
/* 34 " */ (0),
/* 35 # */ (0),
/* 36 $ */ (0|CharIsPunctuation),
/* 37 % */ (0|CharIsPunctuation),
/* 38 & */ (0|CharIsPunctuation),
/* 39 ' */ (0|CharIsPunctuation),
/* 40 ( */ (0|CharIsPunctuation),
/* 41 ) */ (0|CharIsPunctuation),
/* 42 * */ (0),
/* 43 + */ (0|CharIsPunctuation),
/* 44 , */ (0|CharIsPunctuation),
/* 45 - */ (0|CharIsPunctuation),
/* 46 . */ (0|CharIsPeriod|CharIsPunctuation|CharIsPathSeparator),
/* 47 / */ (0|CharIsPathSeparator),
/* 48 0 */ (0|CharIsHexDigit|CharIsOctDigit|CharIsDecDigit|CharIsBinDigit),
/* 49 1 */ (0|CharIsHexDigit|CharIsOctDigit|CharIsDecDigit|CharIsBinDigit),
/* 50 2 */ (0|CharIsHexDigit|CharIsOctDigit|CharIsDecDigit),
/* 51 3 */ (0|CharIsHexDigit|CharIsOctDigit|CharIsDecDigit),
/* 52 4 */ (0|CharIsHexDigit|CharIsOctDigit|CharIsDecDigit),
/* 53 5 */ (0|CharIsHexDigit|CharIsOctDigit|CharIsDecDigit),
/* 54 6 */ (0|CharIsHexDigit|CharIsOctDigit|CharIsDecDigit),
/* 55 7 */ (0|CharIsHexDigit|CharIsOctDigit|CharIsDecDigit),
/* 56 8 */ (0|CharIsHexDigit|CharIsDecDigit),
/* 57 9 */ (0|CharIsHexDigit|CharIsDecDigit),
/* 58 : */ (0|CharIsPunctuation),
/* 59 ; */ (0|CharIsPunctuation),
/* 60 < */ (0|CharIsPunctuation),
/* 61 = */ (0|CharIsPunctuation),
/* 62 > */ (0|CharIsPunctuation),
/* 63 ? */ (0|CharIsPunctuation),
/* 64 @ */ (0),
/* 65 A */ (0|CharIsLetter|CharIsHexDigit),
/* 66 B */ (0|CharIsLetter|CharIsHexDigit),
/* 67 C */ (0|CharIsLetter|CharIsHexDigit),
/* 68 D */ (0|CharIsLetter|CharIsHexDigit),
/* 69 E */ (0|CharIsLetter|CharIsHexDigit),
/* 70 F */ (0|CharIsLetter|CharIsHexDigit),
/* 71 G */ (0|CharIsLetter),
/* 72 H */ (0|CharIsLetter),
/* 73 I */ (0|CharIsLetter),
/* 74 J */ (0|CharIsLetter),
/* 75 K */ (0|CharIsLetter),
/* 76 L */ (0|CharIsLetter),
/* 77 M */ (0|CharIsLetter),
/* 78 N */ (0|CharIsLetter),
/* 79 O */ (0|CharIsLetter),
/* 80 P */ (0|CharIsLetter),
/* 81 Q */ (0|CharIsLetter),
/* 82 R */ (0|CharIsLetter),
/* 83 S */ (0|CharIsLetter),
/* 84 T */ (0|CharIsLetter),
/* 85 U */ (0|CharIsLetter),
/* 86 V */ (0|CharIsLetter),
/* 87 W */ (0|CharIsLetter),
/* 88 X */ (0|CharIsLetter),
/* 89 Y */ (0|CharIsLetter),
/* 90 Z */ (0|CharIsLetter),
/* 91 [ */ (0|CharIsPunctuation),
/* 92 \ */ (0|CharIsPunctuation|CharIsPathSeparator),
/* 93 ] */ (0|CharIsPunctuation),
/* 94 ^ */ (0),
/* 95 _ */ (0|CharIsUnderscore),
/* 96 ` */ (0),
/* 97 a */ (0|CharIsLetter|CharIsHexDigit),
/* 98 b */ (0|CharIsLetter|CharIsHexDigit),
/* 99 c */ (0|CharIsLetter|CharIsHexDigit),
/* 100 d */ (0|CharIsLetter|CharIsHexDigit),
/* 101 e */ (0|CharIsLetter|CharIsHexDigit),
/* 102 f */ (0|CharIsLetter|CharIsHexDigit),
/* 103 g */ (0|CharIsLetter),
/* 104 h */ (0|CharIsLetter),
/* 105 i */ (0|CharIsLetter),
/* 106 j */ (0|CharIsLetter),
/* 107 k */ (0|CharIsLetter),
/* 108 l */ (0|CharIsLetter),
/* 109 m */ (0|CharIsLetter),
/* 110 n */ (0|CharIsLetter),
/* 111 o */ (0|CharIsLetter),
/* 112 p */ (0|CharIsLetter),
/* 113 q */ (0|CharIsLetter),
/* 114 r */ (0|CharIsLetter),
/* 115 s */ (0|CharIsLetter),
/* 116 t */ (0|CharIsLetter),
/* 117 u */ (0|CharIsLetter),
/* 118 v */ (0|CharIsLetter),
/* 119 w */ (0|CharIsLetter),
/* 120 x */ (0|CharIsLetter),
/* 121 y */ (0|CharIsLetter),
/* 122 z */ (0|CharIsLetter),
/* 123 { */ (0|CharIsPunctuation),
/* 124 | */ (0|CharIsPunctuation),
/* 125 } */ (0|CharIsPunctuation),
/* 126 ~ */ (0),
/* 127  */ (0),
/* 128 € */ (0),
/* 129  */ (0),
/* 130 ‚ */ (0),
/* 131 ƒ */ (0),
/* 132 „ */ (0),
/* 133 … */ (0),
/* 134 † */ (0),
/* 135 ‡ */ (0),
/* 136 ˆ */ (0),
/* 137 ‰ */ (0),
/* 138 Š */ (0),
/* 139 ‹ */ (0),
/* 140 Œ */ (0),
/* 141  */ (0),
/* 142 Ž */ (0),
/* 143  */ (0),
/* 144  */ (0),
/* 145 ‘ */ (0),
/* 146 ’ */ (0),
/* 147 “ */ (0),
/* 148 ” */ (0),
/* 149 • */ (0),
/* 150 – */ (0),
/* 151 — */ (0),
/* 152 ˜ */ (0),
/* 153 ™ */ (0),
/* 154 š */ (0),
/* 155 › */ (0),
/* 156 œ */ (0),
/* 157  */ (0),
/* 158 ž */ (0),
/* 159 Ÿ */ (0),
/* 160   */ (0),
/* 161 ¡ */ (0),
/* 162 ¢ */ (0),
/* 163 £ */ (0),
/* 164 ¤ */ (0),
/* 165 ¥ */ (0),
/* 166 ¦ */ (0),
/* 167 § */ (0),
/* 168 ¨ */ (0),
/* 169 © */ (0),
/* 170 ª */ (0),
/* 171 « */ (0),
/* 172 ¬ */ (0),
/* 173 ­ */ (0),
/* 174 ® */ (0),
/* 175 ¯ */ (0),
/* 176 ° */ (0),
/* 177 ± */ (0),
/* 178 ² */ (0),
/* 179 ³ */ (0),
/* 180 ´ */ (0),
/* 181 µ */ (0),
/* 182 ¶ */ (0),
/* 183 · */ (0),
/* 184 ¸ */ (0),
/* 185 ¹ */ (0),
/* 186 º */ (0),
/* 187 » */ (0),
/* 188 ¼ */ (0),
/* 189 ½ */ (0),
/* 190 ¾ */ (0),
/* 191 ¿ */ (0),
/* 192 À */ (0),
/* 193 Á */ (0),
/* 194 Â */ (0),
/* 195 Ã */ (0),
/* 196 Ä */ (0),
/* 197 Å */ (0),
/* 198 Æ */ (0),
/* 199 Ç */ (0),
/* 200 È */ (0),
/* 201 É */ (0),
/* 202 Ê */ (0),
/* 203 Ë */ (0),
/* 204 Ì */ (0),
/* 205 Í */ (0),
/* 206 Î */ (0),
/* 207 Ï */ (0),
/* 208 Ð */ (0),
/* 209 Ñ */ (0),
/* 210 Ò */ (0),
/* 211 Ó */ (0),
/* 212 Ô */ (0),
/* 213 Õ */ (0),
/* 214 Ö */ (0),
/* 215 × */ (0),
/* 216 Ø */ (0),
/* 217 Ù */ (0),
/* 218 Ú */ (0),
/* 219 Û */ (0),
/* 220 Ü */ (0),
/* 221 Ý */ (0),
/* 222 Þ */ (0),
/* 223 ß */ (0),
/* 224 à */ (0),
/* 225 á */ (0),
/* 226 â */ (0),
/* 227 ã */ (0),
/* 228 ä */ (0),
/* 229 å */ (0),
/* 230 æ */ (0),
/* 231 ç */ (0),
/* 232 è */ (0),
/* 233 é */ (0),
/* 234 ê */ (0),
/* 235 ë */ (0),
/* 236 ì */ (0),
/* 237 í */ (0),
/* 238 î */ (0),
/* 239 ï */ (0),
/* 240 ð */ (0),
/* 241 ñ */ (0),
/* 242 ò */ (0),
/* 243 ó */ (0),
/* 244 ô */ (0),
/* 245 õ */ (0),
/* 246 ö */ (0),
/* 247 ÷ */ (0),
/* 248 ø */ (0),
/* 249 ù */ (0),
/* 250 ú */ (0),
/* 251 û */ (0),
/* 252 ü */ (0),
/* 253 ý */ (0),
/* 254 þ */ (0),
/* 255 ÿ */ (0),
};

#if 0
	Here is the code for building the char table:

namespace CharUtil
{

static inline bool IsWhitespace( char c ) {
	return 0x20 == c || '\t' == c || '\n' == c || '\r' == c;
}

static inline bool IsDigit( char c ) {
	return c >= '0' && c <= '9';
}

static inline bool IsBinDigit( char c ) {
	return c == '0' || c == '1';
}

static inline bool IsOctDigit( char c ) {
	return c >= '0' && c <= '7';
}

static inline bool IsHexDigit( char c ) {
	return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
}

static inline bool IsLetter( char c ) {
	return
		//( c == '_' ) ||
		( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' )
		;
}

}//namespace CharUtil

	static
	void InitCharTable()
	{
		mxZERO_OUT( gs_CharMap );
		
		{
			gs_CharMap[ ' ' ] |= CharIsHorzWS;
			gs_CharMap[ '\t' ] |= CharIsHorzWS;
			gs_CharMap[ '\f' ] |= CharIsHorzWS;
			gs_CharMap[ '\v' ] |= CharIsHorzWS;
		}
		{
			gs_CharMap[ '\r' ] |= CharIsVertWS;
			gs_CharMap[ '\n' ] |= CharIsVertWS;
		}
		{
			gs_CharMap[ '_' ] = CharIsUnderscore;

			gs_CharMap[ '(' ] = CharIsPunctuation;
			gs_CharMap[ ')' ] = CharIsPunctuation;
			gs_CharMap[ '{' ] = CharIsPunctuation;
			gs_CharMap[ '}' ] = CharIsPunctuation;
			gs_CharMap[ '[' ] = CharIsPunctuation;
			gs_CharMap[ ']' ] = CharIsPunctuation;
			gs_CharMap[ ':' ] = CharIsPunctuation;
			gs_CharMap[ ';' ] = CharIsPunctuation;
			gs_CharMap[ ',' ] = CharIsPunctuation;
			gs_CharMap[ '.' ] = CharIsPunctuation;
			gs_CharMap[ '?' ] = CharIsPunctuation;
			gs_CharMap[ '$' ] = CharIsPunctuation;
			gs_CharMap[ '/' ] = CharIsPathSeparator;
			gs_CharMap[ '\\' ] = CharIsPunctuation|CharIsPathSeparator;

			gs_CharMap[ '+' ] = CharIsPunctuation;
			gs_CharMap[ '-' ] = CharIsPunctuation;

			gs_CharMap[ '=' ] = CharIsPunctuation;

			gs_CharMap[ '&' ] = CharIsPunctuation;
			gs_CharMap[ '|' ] = CharIsPunctuation;

			gs_CharMap[ '.' ] = CharIsPunctuation|CharIsPeriod|CharIsPathSeparator;
		}

		for( int iChar = 0; iChar < NUM_CHARS; iChar++ )
		{
			const unsigned char c = iChar;

			if( CharUtil::IsLetter( c ) )
			{
				gs_CharMap[ iChar ] |= CharIsLetter;
			}

			if( CharUtil::IsBinDigit( c ) )
			{
				gs_CharMap[ iChar ] |= CharIsBinDigit;
			}
			if( CharUtil::IsDigit( c ) )
			{
				gs_CharMap[ iChar ] |= CharIsDecDigit;
			}
			if( CharUtil::IsOctDigit( c ) )
			{
				gs_CharMap[ iChar ] |= CharIsOctDigit;
			}
			if( CharUtil::IsHexDigit( c ) )
			{
				gs_CharMap[ iChar ] |= CharIsHexDigit;
			}
		}
	}

	int main()
	{
		InitCharTable();

		FileWriter	file("R:/charMap.cpp");

		mxTextWriter	wr( file );

		wr.Putf("/* 0 - EOF */ 0,\n");

		for( int iChar = 1; iChar < NUM_CHARS; iChar++ )
		{
			const int charFlags = gs_CharMap[ iChar ];

			String	charInfo;
			charInfo.Format( "/* %u %c */ (0", iChar, (char)iChar );

	#define ADD_FLAG( CHAR_FLAG )\
			if( charFlags & (CHAR_FLAG) )\
				charInfo += "|"#CHAR_FLAG;

			ADD_FLAG(CharIsHorzWS);
			ADD_FLAG(CharIsVertWS);

			ADD_FLAG(CharIsLetter);

			ADD_FLAG(CharIsHexDigit);
			ADD_FLAG(CharIsOctDigit);
			ADD_FLAG(CharIsDecDigit);
			ADD_FLAG(CharIsBinDigit);

			ADD_FLAG(CharIsUnderscore);
			ADD_FLAG(CharIsPeriod);
			ADD_FLAG(CharIsPunctuation);

			ADD_FLAG(CharIsPathSeparator);

			charInfo += "),\n";

			file.Write(charInfo.raw(), charInfo.Length());
		}
		return 0;
	}
#endif


/// IsIdentifierHead - Return true if this is the first character of an
/// identifier, which is [a-zA-Z_].
static inline bool IsIdentifierHead(unsigned char c) {
  return (gs_CharMap[c] & (CharIsLetter|CharIsUnderscore)) ? true : false;
}
/// IsIdentifierBody - Return true if this is the body character of an
/// identifier, which is [a-zA-Z0-9_].
static inline bool IsIdentifierBody(unsigned char c) {
  return (gs_CharMap[c] & (CharIsLetter|CharIsDecDigit|CharIsUnderscore)) ? true : false;
}

/// IsHorizontalWhitespace - Return true if this character is horizontal
/// whitespace: ' ', '\\t', '\\f', '\\v'.  Note that this returns false for
/// '\\0'.
static inline bool IsHorizontalWhitespace(unsigned char c) {
  return (gs_CharMap[c] & CharIsHorzWS) ? true : false;
}

/// IsVerticalWhitespace - Return true if this character is vertical
/// whitespace: '\\n', '\\r'.  Note that this returns false for '\\0'.
static inline bool IsVerticalWhitespace(unsigned char c) {
  return (gs_CharMap[c] & CharIsVertWS) ? true : false;
}

/// IsWhitespace - Return true if this character is horizontal or vertical
/// whitespace: ' ', '\\t', '\\f', '\\v', '\\n', '\\r'.  Note that this returns
/// false for '\\0'.
static inline bool IsWhitespace(unsigned char c) {
  return (gs_CharMap[c] & (CharIsHorzWS|CharIsVertWS)) ? true : false;
}

/// IsNumberBody - Return true if this is the body character of a
/// preprocessing number, which is [a-zA-Z0-9_.].
static inline bool IsNumberBody(unsigned char c) {
  return (gs_CharMap[c] & (CharIsLetter|CharIsDecDigit|CharIsUnderscore|CharIsPeriod)) ?
    true : false;
}

///// isRawStringDelimBody - Return true if this is the body character of a
///// raw string delimiter.
//static inline bool isRawStringDelimBody(unsigned char c) {
//  return (gs_CharMap[c] &
//          (CharIsLetter|CharIsDecDigit|CharIsUnderscore|CharIsPeriod|CHAR_RAWDEL)) ?
//    true : false;
//}

// Allow external clients to make use of gs_CharMap.
static inline bool IsIdentifierBodyChar(unsigned char c, const LexerOptions &LangOpts) {
	return IsIdentifierBody(c) || (c == '$' && (LangOpts.m_flags & LexerOptions::DollarsInIdentifiers));
}

static inline bool IsFirstIdentifierChar( unsigned char c, const LexerOptions& options )
{
	return IsIdentifierHead(c) || (c == '$' && (options.m_flags & LexerOptions::DollarsInIdentifiers));
}

static inline bool IsNumberStart( unsigned char c )
{
	return (gs_CharMap[c] & (CharIsAnyDigit|CharIsPeriod)) ? true : false;
}

static inline bool IsDecimalDigit( unsigned char c )
{
	return (gs_CharMap[c] & (CharIsDecDigit)) ? true : false;
}

static inline bool IsHexadecimalDigit( unsigned char c )
{
	return (gs_CharMap[c] & (CharIsHexDigit)) ? true : false;
}

static inline bool IsPunctuation( unsigned char c )
{
	return (gs_CharMap[c] & (CharIsPunctuation)) ? true : false;
}

static inline bool isPathSeparator( unsigned char c ) {
	return (gs_CharMap[c] & (CharIsPathSeparator)) ? true : false;
}

static inline bool IsEndOfFile( unsigned char c )
{
	return c == '\0';
}

LexerOptions::LexerOptions()
{
	m_flags.raw_value = SkipComments|DollarsInIdentifiers;
}

LexerState::LexerState()
{
	this->clear();
}

void LexerState::clear()
{
	readPtr = NULL;
	line = 1;
	column = 1;
}

struct SPunctuation
{
	const char *	p;	// punctuation character(s)
	int			length;
	//int n;	// punctuation id
};

template< int LENGTH >
static inline int TCalcStringLength( const char (&_str)[LENGTH] )
{
	return LENGTH-1;
};

static const SPunctuation gs_punctuationTable[] =
{
#define DECLARE_PUNCTUATION_TOKEN( ID, TEXT )	{ TEXT, TCalcStringLength(TEXT) },
	#include "Tokens.inl"
#undef DECLARE_PUNCTUATION_TOKEN
};

void Lexer::Initialize()
{
	m_source = NULL;
	m_length = 0;

	m_curr = NULL;

	m_currentLine = 1;
	m_currentColumn = 1;
}

void Lexer::Shutdown()
{
}

Lexer::Lexer()
{
	this->Initialize();
}

Lexer::Lexer( const void* source, int length, const char* file )
{
	this->Initialize();

	mxASSERT_PTR(source);
	mxASSERT(length > 0);
	//mxASSERT2(source[length] == '\0', "text buffer must be null-terminated!");

	m_source = (char*) source;
	m_length = length;

	m_curr = (char*) source;

	m_currentLine = 1;
	m_currentColumn = 1;

	m_fileName = NameID(file);
}

Lexer::~Lexer()
{
	this->Shutdown();
}

void Lexer::SetFileName( const char* fileName )
{
	m_fileName = NameID(fileName);
}

void Lexer::SetOptions( const LexerOptions& options )
{
	m_options = options;
}

bool Lexer::ReadToken( TbToken &newToken )
{
	return this->ReadNextTokenInternal( newToken );
}

bool Lexer::PeekToken( TbToken &nextToken )
{
	// gives more precise token line number info
	RET_FALSE_IF_NOT( this->SkipWhiteSpaces() );

	LexerState currState;
	this->SaveState( currState );

	const bool bOk = this->ReadNextTokenInternal( nextToken );

	this->RestoreState( currState );

	return bOk;
}

bool Lexer::ReadNextTokenInternal( TbToken &token )
{
	//this->SaveState(m_prevState);

	// clear the token stuff
	token.location = Location();
	token.type = TT_Bad;
	token.text.clear();

	// skip white spaces and comments
	RET_FALSE_IF_NOT( this->SkipWhiteSpaces() );

	const char currentChar = this->CurrChar();

	// Identifiers and keywords
	if ( IsFirstIdentifierChar( currentChar, m_options ) )
	{
		return this->ReadName( token );
	}

	// if there is a number
	else if( IsNumberStart( currentChar ) )
	{
		return this->ReadNumber( token );
	}

	// if there is a leading quote
	else if( currentChar == '\"' )
	{
		return this->ReadString( token );
	}

	// if there is a leading quote
	else if( currentChar == SINGLE_QUOTE )
	{
		// a character or an integer constant
		return this->readString_SingleQuotes( token );
	}

	// Punctuation & Operators
	else if( IsPunctuation( currentChar ) )
	{
		return ParsePunctuation( token );
	}

	//

	Str::SetChar( token.text, currentChar );

	if( m_options.m_flags & LexerOptions::IgnoreUnknownCharacterErrors )
	{
		SkipChar();
		return true;
	}

	tbLEXER_ERROR_BRK(*this, "unknown token: '%s'", token.text.c_str());
	return false;
}

// NOTE: doesn't call ReadToken(), because it may lead to recursive calls and stack overflow
bool Lexer::ExpectNextCharInternal( char c )
{
	// skip white spaces and comments
	if( !this->SkipWhiteSpaces() )
	{
		tbLEXER_ERROR_BRK(*this, "couldn't read expected '%c' : end of file reached", c);
		return false;
	}
	const char currChar = CurrChar();
	mxASSERT( !IsEndOfFile( currChar ) );

	if( currChar != c )
	{
		tbLEXER_ERROR_BRK(*this, "expected '%c', but got '%c'", c, currChar);
		return false;
	}

	SkipChar();	// eat the expected character

	return true;
}

/*
================
reads a token from the current line, continues reading on the next
line only if a backslash '\' is found
================
*/
bool Lexer::ReadLine( TbToken & token )
{
	const int lastLineNum = GetCurrentLine();

	chkRET_FALSE_IF_NOT(ReadToken( token ));

	return lastLineNum == GetCurrentLine();
}

bool Lexer::SkipRestOfLine()
{
	const int lastLineNum = GetCurrentLine();

	TbToken	token;
	while( PeekToken( token ) )
	{
		if( GetCurrentLine() > lastLineNum )
		{
			break;
		}
		SkipToken(*this);
	}

	return true;
}

/*
=================
  parse the rest of the line
=================
*/
bool Lexer::ReadRestOfLine( String &text )
{
	text.empty();

	const char * const startPos = m_curr;

	const char * endPos = strchr( startPos, '\n' );
	chkRET_FALSE_IF_NIL(endPos);

	const int length = endPos - startPos;
	chkRET_FALSE_IF_NOT(length > 0);

	text.Copy(Chars(startPos, length));

	return true;
}

/*
The next token should be an open brace.
Parses until a matching close brace is found.
Internal brace depths are properly skipped.
Returns the number of text lines read.
*/
int ReadBracedSection( const char* start, String &text, const char* &end )
{
	int lines = 0;
	int depth = 0;
	const char* p = start;

	while( *p )
	{
		char c = *p++;
		if( c == '{' ) {
			depth++;
		}
		if( c == '}' ) {
			depth--;
			if( depth <= 0 ) {
				break;
			}
		}
		if( c == '\n' ) {
			lines++;
		}
	}

	end = p;

	const int length = end - start;

	Str::AppendS( text, start, length );

	return lines;
}

/*
=================
The next token should be an open brace.
Parses until a matching close brace is found.
Internal brace depths are properly skipped.
=================
*/
bool Lexer::ParseBracedSection( String &text, bool skipBraces )
{
	chkRET_FALSE_IF_NOT(ExpectNextCharInternal( '{' ));
	if( !skipBraces ) {
		Str::Append( text, '{' );
	}

	const char * const startPos = m_curr;

	const char * endPos = NULL;

	int depth = 1;

	while( depth > 0 )
	{
		endPos = m_curr;

		TbToken	token;
		if( !ReadToken( token ) ) {
			break;
		}

		if( token.type == '{' ) {
			depth++;
		}
		if( token.type == '}' ) {
			depth--;
		}
	}

	chkRET_FALSE_IF_NIL(endPos);

	const int length = endPos - startPos;

	Str::AppendS( text, startPos, length );

	if( !skipBraces ) {
		Str::Append( text, '}' );
	}

	return true;
}

bool Lexer::ParseBracedSection( ATokenWriter &destination )
{
	chkRET_FALSE_IF_NOT(ExpectNextCharInternal( '{' ));

	int depth = 1;

	while( depth > 0 )
	{
		TbToken	token;
		if( !ReadToken( token ) )
		{
			break;
		}

		destination.AddToken( token );

		if( token == '{' ) {
			depth++;
		}
		if( token == '}' ) {
			depth--;
		}
	}
	return true;
}

char Lexer::CurrChar() const
{
	return *m_curr;
}

char Lexer::PeekChar() const
{
	return *(m_curr + 1);
}

char Lexer::PrevChar() const
{
	return *(m_curr - 1);
}

char Lexer::ReadChar()
{
	//mxASSERT( *m_curr != '\n' && *m_curr != '\t' );
	++m_currentColumn;
	++m_curr;
	return *m_curr;
}

void Lexer::SkipChar()
{
	++m_currentColumn;
	++m_curr;
}

bool Lexer::EndOfFile() const
{
	return ((m_curr - m_source) >= m_length)
		|| IsEndOfFile( CurrChar() )
		;
}

void Lexer::IncrementLineCounter()
{
	++m_currentLine;
	m_currentColumn = 0;
}

void Lexer::SaveState( LexerState &state ) const
{
	state.readPtr	= m_curr;
	state.line		= m_currentLine;
	state.column	= m_currentColumn;
}

void Lexer::RestoreState( const LexerState& state )
{
	m_curr			= state.readPtr;
	m_currentLine	= state.line;
	m_currentColumn	= state.column;
}

int Lexer::GetCurrentLine() const
{
	return m_currentLine;
}

int Lexer::GetCurrentColumn() const
{
	return m_currentColumn;
}

const char* Lexer::GetFileName() const
{
	return m_fileName.raw();
}

void Lexer::GetLocation( Location & out ) const
{
	out.file	= m_fileName;
	out.line	= m_currentLine;
	out.column	= m_currentColumn;
}

Location Lexer::GetLocation() const
{
	Location	result;
	this->GetLocation(result);
	return result;
}

const char* Lexer::CurrentPtr() const
{
	return m_curr;
}

void Lexer::SetLineNumber( int newLineNum )
{
	//chkRET_FALSE_IF_NOT(newLineNum > 0);
	m_currentLine = newLineNum;
}

// Reads an identifier or a keyword
// into the buffer and returns false 
// if started reading at the end of file.
//
// TODO:
// Checks for buffer overruns,
// ignore last chars in long identifiers,
// etc..
bool Lexer::ReadName( TbToken &token )
{
	char	charBuffer[MAX_TOKEN_LENGTH];
	mxZERO_OUT(charBuffer);

	const char* startPos = m_curr;
	char* writePtr = charBuffer;

	const int lastLine = m_currentLine;
	const int lastColumn = m_currentColumn;

	while(
		IsIdentifierBodyChar( CurrChar(), m_options )
		|| ( isPathSeparator( CurrChar() ) && ( m_options.m_flags & LexerOptions::AllowPathNames ) )
		)
	{
		if( m_curr - startPos > mxCOUNT_OF(charBuffer)-1 )
		{
			tbLEXER_ERROR_BRK( *this, "identifier '%s' is too long (max allowed length is %u characters)", charBuffer, int(mxCOUNT_OF(charBuffer)-1) );
			return false;
		}
		*writePtr++ = CurrChar();
		ReadChar();
	}

	const int tokenLength = m_curr - startPos;

	token.location.file	= m_fileName;
	token.location.line		= lastLine;
	token.location.column	= lastColumn;

	token.type = TT_Name;

	Str::CopyS( token.text, charBuffer, tokenLength );

	return true;
}

bool Lexer::ReadString( TbToken &token )
{
	const char* startPos = m_curr;

	const int lastLine = m_currentLine;
	const int lastColumn = m_currentColumn;

	chkRET_FALSE_IF_NOT( ExpectNextCharInternal('\"') );

	while ( CurrChar() != '\"' && !EndOfFile() )
	{
		if( CurrChar() == '\n' )
		{
			tbLEXER_ERROR_BRK(*this, "new line in string constant");
			return false;
		}

		Str::Append( token.text, CurrChar() );
		ReadChar();
	}

	chkRET_FALSE_IF_NOT( ExpectNextCharInternal('\"') );		// read the closing '\"'

	const int tokenLength = m_curr - startPos;
	mxUNUSED(tokenLength);

	token.location.file	= m_fileName;
	token.location.line		= lastLine;
	token.location.column	= lastColumn;

	token.type = TT_String;

	return true;
}

bool Lexer::readString_SingleQuotes( TbToken &token )
{
	const char* startPos = m_curr;

	const int lastLine = m_currentLine;
	const int lastColumn = m_currentColumn;

	chkRET_FALSE_IF_NOT( ExpectNextCharInternal(SINGLE_QUOTE) );

	while ( CurrChar() != SINGLE_QUOTE && !EndOfFile() )
	{
		if( CurrChar() == '\n' )
		{
			tbLEXER_ERROR_BRK(*this, "new line in string constant");
			return false;
		}

		Str::Append( token.text, CurrChar() );
		ReadChar();
	}

	chkRET_FALSE_IF_NOT( ExpectNextCharInternal(SINGLE_QUOTE) );		// read the closing '\"'

	const int tokenLength = m_curr - startPos;
	mxUNUSED(tokenLength);

	token.location.file	= m_fileName;
	token.location.line		= lastLine;
	token.location.column	= lastColumn;

	token.type = TT_String;

	return true;
}

// Reads a numeric constant
// and returns its type ( L_INT32, L_U32, ... ).
// Integers can be decimal, octal or hex.
// Handles the suffixes U, UL, LU, L, etc.
//
bool Lexer::ReadNumber( TbToken &token )
{
	// a float number starting with '.'
	if( CurrChar() == '.' )
	{
		return ReadFloat( token );
	}

	if( CurrChar() == '0' && IsNumberBody(PeekChar()) && PeekChar() != '.' )
	{
		// check for a hexadecimal number (e.g. 0xCAFEBABE)
		if( PeekChar() == 'x' || PeekChar() == 'X' || PeekChar() == 'h' || PeekChar() == 'H' )
		{
			return ReadHexadecimal( token );
		}

		// check for a binary number (e.g. 0b10100111)
		if( PeekChar() == 'b' || PeekChar() == 'B' )
		{
			return ReadBinaryInteger( token );
		}

		// its an octal number
		return ReadOctalInteger( token );
	}

	// decimal integer or floating point number or ip address

	int numDots = 0;

	const char* p = m_curr;

	while( !IsEndOfFile( *p ) )
	{
		++p;
		if( *p == '.' ) {
			numDots++;
			continue;
		}
		if( !IsDecimalDigit( *p ) ) {
			break;
		}
	}

	if( numDots == 0 )
	{
		return ReadInteger( token );
	}
	else if( numDots == 1 )
	{
		return ReadFloat( token );
	}
	else
	{
		tbLEXER_ERROR_BRK(*this, "more than one dot (%u) in a number", numDots);
	}

	return false;
}

bool Lexer::ReadInteger( TbToken &token )
{
	char	charBuffer[MAX_TOKEN_LENGTH];
	mxZERO_OUT(charBuffer);

	const char* startPos = m_curr;
	char* writePtr = charBuffer;

	const int lastLine = m_currentLine;
	const int lastColumn = m_currentColumn;

	// read number body

	while( IsDecimalDigit( CurrChar() ) )
	{
		if( m_curr - startPos > mxCOUNT_OF(charBuffer)-1 )
		{
			tbLEXER_ERROR_BRK(*this, "identifier '%s' is too long (max allowed length is %u characters)", charBuffer, int(mxCOUNT_OF(charBuffer)-1) );
			return false;
		}
		*writePtr++ = CurrChar();
		ReadChar();
	}

	// read integer-suffix if any

	bool bIsLong = false;
	bool bIsUnsigned = false;

	// can have both suffixes (e.g. 100UL)
	// http://cpp.comsci.us/etymology/literals.html
	//

	// unsigned-suffix
	if( CurrChar() == 'u' || CurrChar() == 'U' )
	{
		SkipChar();
		bIsUnsigned = true;
	}
	// long-suffix
	if( CurrChar() == 'l' || CurrChar() == 'L' )
	{
		SkipChar();
		bIsLong = true;
	}

	token.type = TT_Number;
	token.flags = TF_Integer;
/*
	if( bIsLong )
	{
		if( bIsUnsigned )
		{
			token.type = L_U64;
		}
		else
		{
			token.type = L_INT64;
		}
	}
	else
	{
		if( bIsUnsigned )
		{
			token.type = L_U32;
		}
		else
		{
			token.type = L_INT32;
		}
	}
*/
	const int tokenLength = m_curr - startPos;

	token.location.file	= m_fileName;
	token.location.line		= lastLine;
	token.location.column	= lastColumn;

	Str::CopyS( token.text, charBuffer, tokenLength );

	return true;
}

bool Lexer::ReadFloat( TbToken &token )
{
	char	charBuffer[MAX_TOKEN_LENGTH];
	mxZERO_OUT(charBuffer);

	const char* startPos = m_curr;
	char* writePtr = charBuffer;

	const int lastLine = m_currentLine;
	const int lastColumn = m_currentColumn;

	// read number body

	while( IsDecimalDigit( CurrChar() ) || CurrChar() == '.' )
	{
		if( m_curr - startPos > mxCOUNT_OF(charBuffer)-1 )
		{
			tbLEXER_ERROR_BRK(*this, "identifier '%s' is too long (max allowed length is %u characters)", charBuffer, int(mxCOUNT_OF(charBuffer)-1) );
			return false;
		}
		*writePtr++ = CurrChar();
		ReadChar();
	}

	token.type = TT_Number;
	token.flags = TF_Float;

	// read float-suffix if any

	bool bDoublePrecision = true;

	if( CurrChar() == 'f' || CurrChar() == 'F' )
	{
		*writePtr++ = CurrChar();
		SkipChar();
		bDoublePrecision = false;
	}

	const int tokenLength = m_curr - startPos;

	token.location.file	= m_fileName;
	token.location.line		= lastLine;
	token.location.column	= lastColumn;

	Str::CopyS( token.text, charBuffer, tokenLength );

	return true;
}

bool Lexer::ReadHexadecimal( TbToken &token )
{
	const char hexPrefix = ReadChar();
	mxASSERT( hexPrefix == 'x' || hexPrefix == 'X' || hexPrefix == 'h' || hexPrefix == 'H' );

	// advance to the number body
	ReadChar();

	char	charBuffer[MAX_TOKEN_LENGTH];
	mxZERO_OUT(charBuffer);

	const char* startPos = m_curr;
	char* writePtr = charBuffer;

	const int lastLine = m_currentLine;
	const int lastColumn = m_currentColumn;

	// read number body

	while( IsHexadecimalDigit( CurrChar() ) )
	{
		if( m_curr - startPos > mxCOUNT_OF(charBuffer)-1 )
		{
			tbLEXER_ERROR_BRK(*this, "identifier '%s' is too long (max allowed length is %u characters)", charBuffer, int(mxCOUNT_OF(charBuffer)-1) );
			return false;
		}
		*writePtr++ = CurrChar();
		ReadChar();
	}

	const int tokenLength = m_curr - startPos;

	token.location.file	= m_fileName;
	token.location.line		= lastLine;
	token.location.column	= lastColumn;

	token.type = TT_Number;
	token.flags = TF_Hex;

	Str::CopyS( token.text, charBuffer, tokenLength );

	return true;
}

bool Lexer::ReadOctalInteger( TbToken &token )
{
	UNDONE;
	//token.m_numberType = TF_Octal;
	return true;
}

bool Lexer::ReadBinaryInteger( TbToken &token )
{
	UNDONE;
	//token.m_numberType = TF_Binary;
	return true;
}

// Punctuation & Operators
//
bool Lexer::ParsePunctuation( TbToken &token )
{
	const int lastLine = m_currentLine;
	const int lastColumn = m_currentColumn;

	for( int i = 0 ; i < mxCOUNT_OF(gs_punctuationTable); i++ )
	{
		const SPunctuation & s = gs_punctuationTable[i];
		if( strncmp( s.p, m_curr, s.length ) == 0 )
		{
			for( int k = 0; k < s.length; k++ )
			{
				Str::Append( token.text, *m_curr );
				ReadChar();
			}

			token.location.file	= m_fileName;
			token.location.line		= lastLine;
			token.location.column	= lastColumn;

			token.type = TT_Punctuation;

			return true;
		}
	}

UNDONE;
	return false;
}

// Reads spaces, tabs, C-like comments etc.
// When a newline character is found the line counter is increased.
// returns true if the lexer should continue scanning the text buffer.
//
bool Lexer::SkipWhiteSpaces()
{

L_Start_Again:

	// skip white spaces
	while( CurrChar() <= ' ' )
	{
		//if( EndOfFile() ) {
		//	return false;
		//}
		if( CurrChar() == '\n' )
		{
			IncrementLineCounter();
		}
		if( CurrChar() == '\t' )
		{
			m_currentColumn += TAB_WIDTH_IN_COLUMNS-1;
		}

		SkipChar();

		if( EndOfFile() ) {
			return false;
		}
	}


	// check if it's a start of a comment
	if( CurrChar() == '/' )
	{
		if( m_options.m_flags & LexerOptions::SkipComments )
		{
			// single-line comments //
			// line comments terminate at the end of the line
			if( PeekChar() == '/' )
			{
				if( m_options.m_flags & LexerOptions::SkipSingleLineComments )
				{
					SkipChar();	// skip the second '/'
					do
					{
						ReadChar();
						if( EndOfFile() ) {
							return false;
						}
					}
					while ( CurrChar() != '\n' );

					IncrementLineCounter();

					ReadChar();
					if( EndOfFile() ) {
						return false;
					}

					goto L_Start_Again;
				}

			}// if next char is '/'

			// multi-line comments /* ... */
			// these can span multiple lines, but do not nest
			else if ( PeekChar() == '*' )
			{
				if( m_options.m_flags & LexerOptions::SkipMultiLineComments )
				{
					SkipChar();	// skip '*'
					while( true )
					{
						ReadChar();

						if( EndOfFile() ) {
							return false;
						}

						if( CurrChar() == '\n' )
						{
							IncrementLineCounter();
						}

						if ( CurrChar() == '*' )
						{
							if ( PeekChar() == '/' )
							{
								if ( PeekChar() == '*' ) {
									tbLEXER_WARNING_BRK( *this, "nested comment" );
								}

								ReadChar();	// readPtr char is '*' and next char is '/', so eat '*'
								ReadChar();	// and eat '/'

								break;	// end of /* */ comment
							}
						}

					}//while( true )

					ReadChar();	// advance to the next char
					if( EndOfFile() ) {
						return false;
					}

					goto L_Start_Again;
				}

			}// multi-line comments /* ... */

	#if 0
			// D-style comments /+ ... +/
			// these can span multiple lines and can nest
			else if ( PeekChar() == '+' ) 
			{
				SkipChar();

				int nestLevel = 1;

				while( nestLevel > 0 )
				{
					SkipChar();	// skip '+'

					if( EndOfFile() ) {
						if( nestLevel > 0 ) {
							tbLEXER_WARNING_BRK( "unterminated /+ +/ comment; end of file reached." );
						}
						return false;
					}

					if ( '\n' == CurrChar() ) {
						IncrementLineCounter();
					}
					else if ( '/' == CurrChar() )
					{
						if ( '+' == PrevChar() ) {
							--nestLevel;	// '+/'
						}
					}
					else if( '+' == CurrChar() )
					{
						if ( '/' == PrevChar() ) {
							++nestLevel;	// '/+'
						}
					}
				}

				ReadChar();
				if( EndOfFile() ) {
					return false;
				}

				goto L_Start_Again;

			}// D-style comments /+ ... +/
	#endif

		}//if skipping comments is enabled

	}// if this char was '/'


	// Check single-line comments starting with ';'

	if( m_options.m_flags & LexerOptions::SkipSemicolonComments )
	{
		// line comments terminate at the end of the line
		if( CurrChar() == ';' )
		{
			do
			{
				SkipChar();

				if( EndOfFile() ) {
					return false;
				}
			}
			while ( CurrChar() != '\n' );

			IncrementLineCounter();

			SkipChar();	// read '\n'

			if( EndOfFile() ) {
				return false;
			}

			goto L_Start_Again;

		}// if next char is '/'
	}

	// Check multi-line comments starting with '|': | ... |

	if( m_options.m_flags & LexerOptions::SkipSingleVerticalLineComments )
	{
		if( CurrChar() == '|' )
		{
			do
			{
				SkipChar();

				if( EndOfFile() ) {
					return false;
				}

				if ( '\n' == CurrChar() ) {
					IncrementLineCounter();
				}
			}
			while ( CurrChar() != '|' );

			SkipChar();	// skip terminating '|'

			goto L_Start_Again;

		}// if next char is '|'
	}

	return true;
}

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
