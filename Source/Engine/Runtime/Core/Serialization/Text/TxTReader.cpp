/*
=============================================================================
=============================================================================
*/
#include <Base/Base.h>
#include <Base/Util/StaticStringHash.h>
#include <Core/Serialization/Text/TxTCommon.h>
#include <Core/Serialization/Text/TxTPrivate.h>
#include <Core/Serialization/Text/TxTReader.h>

#define xxFAILED( X )	((X) != Ret_OK)

// xxTRY(X) - evaluates the given expression and simply returns an error code if the expression fails.
#define xxTRY( X )\
	do{\
		const EResultCode ec = (X);\
		if(xxFAILED(ec))\
		{\
			ptBREAK;\
			return ec;\
		}\
	}while(0)

namespace SON
{

mxSTOLEN("parts stolen from Clang: http://llvm.org/svn/llvm-project/cfe/trunk/lib/Lex/Lexer.cpp");

enum CharFlags
{
	CharIsEoF		= BIT(0),	// end of file

	// whitespaces
	CharIsHorzWS	= BIT(1),  // ' ', '\t', '\f', '\v'.  Note, no '\0'
	CharIsVertWS	= BIT(2),  // '\r', '\n'

	CharIsLetter	= BIT(3),	// valid letter of the English alphabet (a-z,A-Z)

	// numbers
	CharIsBinDigit	= BIT(4),	// valid digit of a binary number
	CharIsDecDigit	= BIT(5),	// valid digit of a decimal number
	CharIsOctDigit	= BIT(6),	// valid digit of an octal number
	CharIsHexDigit	= BIT(7),	// valid digit of a hexadecimal number

	CharIsUnderscore= BIT(8),	// '_'
	CharIsPunctuation= BIT(9),	// ';', ':', ',', '...'
	CharIsPeriod	= BIT(10),	// '.'

	CharIsSign		= BIT(11),	// '+', '-'

	//CharIsOperator	= BIT(11),	// '==', '>=', '<', etc.

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
mxPREALIGN(16) static const UINT16 gs_charTable[ NUM_CHARS ] =
{
/* 0 - EOF */ CharIsEoF,
/* 1  */ (0),
/* 2  */ (0),
/* 3  */ (0),
/* 4  */ (0),
/* 5  */ (0),
/* 6  */ (0),
/* 7  */ (0),
/* 8  */ (0),
/* 9 	 */ (0|CharIsHorzWS),
/* 10 
 */ (0|CharIsVertWS),
/* 11  */ (0|CharIsHorzWS),
/* 12  */ (0|CharIsHorzWS),
/* 13 
 */ (0|CharIsVertWS),
/* 14  */ (0),
/* 15  */ (0),
/* 16  */ (0),
/* 17  */ (0),
/* 18  */ (0),
/* 19  */ (0),
/* 20  */ (0),
/* 21  */ (0),
/* 22  */ (0),
/* 23  */ (0),
/* 24  */ (0),
/* 25  */ (0),
/* 26  */ (0),
/* 27  */ (0),
/* 28  */ (0),
/* 29  */ (0),
/* 30  */ (0),
/* 31  */ (0),
/* 32   */ (0|CharIsHorzWS),
/* 33 ! */ (0),
/* 34 " */ (0),
/* 35 # */ (0),
/* 36 $ */ (0|CharIsPunctuation),
/* 37 % */ (0),
/* 38 & */ (0),
/* 39 ' */ (0),
/* 40 ( */ (0|CharIsPunctuation),
/* 41 ) */ (0|CharIsPunctuation),
/* 42 * */ (0),
/* 43 + */ (CharIsSign),
/* 44 , */ (0|CharIsPunctuation),
/* 45 - */ (CharIsSign),
/* 46 . */ (0|CharIsPeriod|CharIsPunctuation),
/* 47 / */ (0),
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
/* 60 < */ (0),
/* 61 = */ (0),
/* 62 > */ (0),
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
/* 92 \ */ (0|CharIsPunctuation),
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
/* 124 | */ (0),
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

/// IsIdentifierHead - Return true if this is the first character of an
/// identifier, which is [a-zA-Z_].
static inline bool IsIdentifierHead( CharType c ) {
	return (gs_charTable[c] & (CharIsLetter|CharIsUnderscore)) != 0;
}
/// IsIdentifierBody - Return true if this is the body character of an
/// identifier, which is [a-zA-Z0-9_].
static inline bool IsIdentifierBody( CharType c ) {
	return (gs_charTable[c] & (CharIsLetter|CharIsDecDigit|CharIsUnderscore)) != 0;
}

/// IsHorizontalWhitespace - Return true if this character is horizontal
/// whitespace: ' ', '\\t', '\\f', '\\v'.  Note that this returns false for
/// '\\0'.
static inline bool IsHorizontalWhitespace( CharType c ) {
	return (gs_charTable[c] & CharIsHorzWS) != 0;
}

/// IsVerticalWhitespace - Return true if this character is vertical
/// whitespace: '\\n', '\\r'.  Note that this returns false for '\\0'.
static inline bool IsVerticalWhitespace( CharType c ) {
	return (gs_charTable[c] & CharIsVertWS) != 0;
}

/// IsWhitespace - Return true if this character is horizontal or vertical
/// whitespace: ' ', '\\t', '\\f', '\\v', '\\n', '\\r'.  Note that this returns
/// false for '\\0'.
static inline bool IsWhitespace( CharType c ) {
	return (gs_charTable[c] & (CharIsHorzWS|CharIsVertWS)) != 0;
}

/// IsNumberBody - Return true if this is the body character of a
/// preprocessing number, which is [a-zA-Z0-9_.].
static inline bool IsNumberBody( CharType c ) {
	return (gs_charTable[c] & (CharIsLetter|CharIsDecDigit|CharIsUnderscore|CharIsPeriod)) != 0;
}

static inline bool IsNumberStart( CharType c ) {
	return (gs_charTable[c] & (CharIsSign|CharIsAnyDigit|CharIsPeriod)) != 0;
}

static inline bool IsDecimalDigit( CharType c ) {
	return (gs_charTable[c] & (CharIsDecDigit)) != 0;
}

static inline bool IsHexadecimalDigit( CharType c ) {
	return (gs_charTable[c] & (CharIsHexDigit)) != 0;
}

static inline bool IsPunctuation( CharType c ) {
	return (gs_charTable[c] & (CharIsPunctuation)) != 0;
}

// Useful references:
// http://krashan.ppa.pl/articles/stringtofloat/
//
// Simple and fast atof (ascii to float) function.
//
// - Executes about 5x faster than standard MSCRT library atof().
// - An attractive alternative if the number of calls is in the millions.
// - Assumes input is a proper integer, fraction, or scientific format.
// - Matches library atof() to 15 digits (except at extreme exponents).
// - Follows atof() precedent of essentially no error checking.
//
// 09-May-2009 Tom Van Baak (tvb) www.LeapSecond.com
//

//#define white_space(c) ((c) == ' ' || (c) == '\t')
//#define valid_digit(c) ((c) >= '0' && (c) <= '9')
#define valid_digit(c) (IsDecimalDigit(c))

static double faster_atof(char *p, char*& end)
{
    int frac;
    double sign, value, scale;

    //// Skip leading white space, if any.
    //while (white_space(*p) ) {
    //    p += 1;
    //}

    // Get sign, if any.
    sign = 1.0;
    if (*p == '-') {
        sign = -1.0;
        p += 1;
    } else if (*p == '+') {
        p += 1;
    }

    // Get digits before decimal point or exponent, if any.
    for (value = 0.0; valid_digit(*p); p += 1) {
        value = value * 10.0 + (*p - '0');
    }

    // Get digits after decimal point, if any.
    if (*p == '.') {
        double pow10 = 10.0;
        p += 1;
        while (valid_digit(*p)) {
            value += (*p - '0') / pow10;
            pow10 *= 10.0;
            p += 1;
        }
    }

    // Handle exponent, if any.
    frac = 0;
    scale = 1.0;
    if ((*p == 'e') || (*p == 'E')) {
        unsigned int expon;

        // Get sign of exponent, if any.
        p += 1;
        if (*p == '-') {
            frac = 1;
            p += 1;

        } else if (*p == '+') {
            p += 1;
        }

        // Get digits of exponent, if any.
        for (expon = 0; valid_digit(*p); p += 1) {
            expon = expon * 10 + (*p - '0');
        }
        if (expon > 308) expon = 308;

        // Calculate scaling factor.
        while (expon >= 50) { scale *= 1E50; expon -= 50; }
        while (expon >=  8) { scale *= 1E8;  expon -=  8; }
        while (expon >   0) { scale *= 10.0; expon -=  1; }
    }

	end = p;

    // Return signed and scaled floating point result.
    return sign * (frac ? (value / scale) : (value * scale));
}

#define xxRET_NIL_IF_ERROR(_PARSER)		{if(_PARSER.error != Ret_OK) return nil;}

static inline CharType CurrentChar( const Parser& _parser )
{
	return _parser.buffer[ _parser.position ];
}
static inline bool AtEOF( const Parser& _parser )
{
	return _parser.position >= _parser.length || CurrentChar(_parser) == '\0';
}

static void ShowParsingError( const Parser& _parser, const char* message, ... )
{
	va_list	args;
	va_start( args, message );
	ptPRINT_VARARGS_BODY(
		message, args,
		//NOTE: formatted for visual studio debug output (click -> go to source location)
		ptWARN("%s(%d,%d): %s\n", _parser.file, _parser.line, _parser.column, ptr_)
	);
	va_end( args );

	mxBeep(100);
}

static void DebugPrintNode( const Node& _node )
{
	const ETypeTag tag = _node.tag.type;
	switch( tag )
	{
	case TypeTag_Nil :		ptPRINT("Nil\n");break;

	case TypeTag_List :		ptPRINT("( size=%u, kids=%#010x, next=%#010x )\n", _node.value.l.size, _node.value.l.kids, _node.next);break;
	case TypeTag_Array :	ptPRINT("[ size=%u, kids=%#010x, next=%#010x ]\n", _node.value.a.size, _node.value.a.kids, _node.next);break;
	case TypeTag_Object :	ptPRINT("{ size=%u, kids=%#010x, next=%#010x }\n", _node.value.o.size, _node.value.o.kids, _node.next);break;

	case TypeTag_String :	ptPRINT("'%s' (next=%#010x)\n", _node.value.s.start, _node.next);break;
	case TypeTag_Number :	ptPRINT("%f (next=%#010x)\n", _node.value.n.f, _node.next);break;
	//case TypeTag_Boolean :	ptPRINT("Boolean\n");break;
	//case TypeTag_HashKey :	ptPRINT("HashKey\n");break;
	//case TypeTag_DateTime :ptPRINT("DateTime\n");break;
	mxDEFAULT_UNREACHABLE(;);
	}
}

//static void DebugPrintKey( const Node& _value, const Parser& _parser )
//{
//	if( _value.name ) {
//		ptPRINT("%s = ", _value.name);
//	}
//}

static void DebugPrintState( const Parser& _parser )
{
	//ptPRINT("=== BEGIN Dumping nodes:\n");
	//const UINT count = _parser.nodes->num();
	//const Node* items = _parser.nodes->raw();
	//for( UINT i = 0; i < count; i++ )
	//{
	//	const Node& node = items[ i ];
	//	ptPRINT("[%u]: ", i);	DebugPrintKey(node, _parser);	DebugPrintNode(node);
	//}
	//ptPRINT("=== END Dumping nodes:\n");
}

static inline void IncrementLineCounter( Parser & _parser )
{
	_parser.line++;
	_parser.column = 0;
}

static inline CharType ReadChar( Parser & _parser )
{
	CharType c = _parser.buffer[ _parser.position ];
	_parser.position++;
	_parser.column++;
	return c;
}

static inline void SkipRestOfLine( Parser & _parser )
{
	while( !AtEOF(_parser) && CurrentChar(_parser) != '\n' )
	{
		ReadChar(_parser);
	}
}

static inline void InsertNullCharacter( Parser & _parser, U32 _offset )
{
	mxASSERT(_offset < _parser.length);
	_parser.buffer[ _offset ] = '\0';
}

// Reads spaces, tabs, C-like comments etc.
// When a newline character is found the line counter is increased.
// Returns true if the lexer should continue scanning the text buffer.
//
static inline bool SkipWhitespaces( Parser & _parser )
{
	// skip white spaces
	for(;;)
	{
L_Loop:
		if( AtEOF(_parser) ) {
			return false;
		}
		const CharType c = CurrentChar(_parser);
		// check one-line comment
		if( c == ';' ) {
			ReadChar(_parser);	// skip ';'
			SkipRestOfLine(_parser);
			continue;
		}
		// check multi-line comments
		if( c == '|' )
		{
			ReadChar(_parser);
			for(;;) {
				if( AtEOF(_parser) ) {
					return false;
				}
				if( CurrentChar(_parser) == '|' ) {
					ReadChar(_parser);
					goto L_Loop;	// end of / ... / comment
				}
				if( CurrentChar(_parser) == '\n' ) {
					IncrementLineCounter(_parser);
				}
				ReadChar(_parser);
			}
		}

#if txtPARSER_ALLOW_C_STYLE_COMMENTS
		// check if it's a start of a comment
		if( c == '/' )
		{
			ReadChar(_parser);	// skip '/'
			// multi-line comments /* ... */
			// these can span multiple lines, but do not nest
			if( CurrentChar(_parser) == '*' ) {
				ReadChar(_parser);
				for(;;) {
					if( AtEOF(_parser) ) {
						return false;
					}
					if( CurrentChar(_parser) == '*' ) {
						ReadChar(_parser);
						if( CurrentChar(_parser) == '/' ) {
							ReadChar(_parser);
							goto L_Loop;	// end of /* */ comment
						}
					}
					ReadChar(_parser);
				}
			}
		}
#endif // txtPARSER_ALLOW_C_STYLE_COMMENTS

		if( !IsWhitespace( c ) ) {
			break;
		}
		if( c == '\n' ) {
			IncrementLineCounter(_parser);
		}
		//if( c == '\t' ) {
		//	_parser.column++;
		//}
		ReadChar(_parser);	// eat whitespace
	}
	return true;
}

static inline EResultCode ReadExpectedCharacter( Parser & _parser, CharType _character )
{
	if( SkipWhitespaces( _parser ) )
	{
		if( CurrentChar( _parser ) == _character ) {
			ReadChar( _parser );
			return Ret_OK;
		}
	}
	ShowParsingError(_parser, "expected '%c', but got '%c'", _character, CurrentChar(_parser) );
	return Ret_UNEXPECTED_CHARACTER;
}

// represents a List/Array/Object scope
struct Scope
{
	Node *	node;	// pointer to the corresponding node
	Node *	tail;	// pointer to last child of the node
};

static inline void AppendChild( Scope & _parent, Node* _child )
{
	mxASSERT(!ETypeTag_Is_Leaf(_parent.node->tag.type));
	if( _parent.tail ) {
		mxASSERT(_parent.tail->next == nil);
		_parent.tail->next = _child;
	} else {
		_parent.node->value.l.kids = _child;
	}
	_parent.tail = _child;
	_parent.node->value.l.size++;
}

static U32 ParseListBody( Scope & _parent, Parser & _parser, NodeAllocator & _allocator );
static U32 ParseArrayBody( Scope & _parent, Parser & _parser, NodeAllocator & _allocator );
static U32 ParseObjectBody( Scope & _parent, Parser & _parser, NodeAllocator & _allocator );

static Node* ParseList( Parser & _parser, NodeAllocator & _allocator )
{
	ReadExpectedCharacter( _parser, '(' );	// consume '(' (40)

	Node* listValue = _allocator.AllocateNode();

	Scope	listScope;
	listScope.node = listValue;
	listScope.tail = nil;
	
	listValue->value.u = 0;
	listValue->next = nil;
	listValue->tag.type = TypeTag_List;
	listValue->name = nil;

	ParseListBody( listScope, _parser, _allocator );

	ReadExpectedCharacter( _parser, ')' );	// (41)

	return listValue;
}

static Node* ParseArray( Parser & _parser, NodeAllocator & _allocator )
{
	ReadExpectedCharacter( _parser, '[' );	// consume '[' (91)

	Node* arrayValue = _allocator.AllocateNode();
	arrayValue->value.u = 0;
	arrayValue->next = nil;
	arrayValue->tag.type = TypeTag_Array;
	arrayValue->name = nil;

	Scope	arrayScope;
	arrayScope.node = arrayValue;
	arrayScope.tail = 0;

	ParseArrayBody( arrayScope, _parser, _allocator );

	ReadExpectedCharacter( _parser, ']' );	// (93)

	return arrayValue;
}

static Node* ParseObject( Parser & _parser, NodeAllocator & _allocator )
{
	ReadExpectedCharacter( _parser, '{' );	// consume '{' (123)

	Node* objectValue = _allocator.AllocateNode();
	objectValue->value.u = 0;
	objectValue->next = nil;
	objectValue->tag.type = TypeTag_Object;
	objectValue->name = nil;

	Scope	objectScope;
	objectScope.node = objectValue;
	objectScope.tail = 0;

	ParseObjectBody( objectScope, _parser, _allocator );

	ReadExpectedCharacter( _parser, '}' );	// (125)

	return objectValue;
}

// Reads a numeric constant.
static Node* ParseNumber( Parser & _parser, NodeAllocator & _allocator )
{
	char *	start = _parser.buffer + _parser.position;
	char *	end;
	double	f = faster_atof(start, end);

	const U32 charsRead = U32(end - start);
	if( !charsRead ) {
		ShowParsingError(_parser, "malformed number");
		return nil;
	}

	_parser.position += charsRead;
	_parser.column += charsRead;

	Node* valueNode = _allocator.AllocateNode();
	valueNode->value.n.f = f;
	valueNode->next = nil;
	valueNode->tag.type = TypeTag_Number;
	valueNode->name = nil;

	return valueNode;
}

static Node* ParseString( Parser & _parser, NodeAllocator & _allocator )
{
	ReadChar( _parser );	// consume '\''

	U32	stringOffset = _parser.position;
	char *	stringBody = _parser.buffer + stringOffset;
	while( !AtEOF(_parser) && CurrentChar(_parser) != '\'' )
	{
		ReadChar( _parser );
	}

	const U32 nullPosition = smallest(_parser.position, _parser.length-1);

	ReadExpectedCharacter( _parser, '\'' );

	InsertNullCharacter(_parser, nullPosition);

	Node* valueNode = _allocator.AllocateNode();
	valueNode->value.s.start = stringBody;
	valueNode->value.s.length = nullPosition - stringOffset;
	valueNode->next = nil;
	valueNode->tag.type = TypeTag_String;
	valueNode->name = nil;

	return valueNode;
}

static Node* ParseNil( Parser & _parser, NodeAllocator & _allocator )
{
	ReadChar( _parser );	// consume 'n'
	if( ReadChar(_parser) == 'i' && ReadChar(_parser) == 'l' )
	{
		Node* valueNode = _allocator.AllocateNode();
		valueNode->value.u = 0;
		valueNode->next = nil;
		valueNode->tag.type = TypeTag_Nil;
		valueNode->name = nil;
		return valueNode;
	}
	ShowParsingError(_parser, "expected 'nil'");
	return nil;
}

// <value> := <object> | <array> | <string> | <number> | NIL
static Node* ParseValue( Parser & _parser, NodeAllocator & _allocator )
{
	if( !SkipWhitespaces( _parser ) ) {
		return nil;
	}

	const CharType c = CurrentChar( _parser );

	// if there is a number
	if( IsNumberStart( c ) ) {
		return ParseNumber(_parser, _allocator);
	}
	// if there is a leading quote - must be a string
	else if( c == '\'' ) {
		return ParseString(_parser, _allocator);
	}
	else if( c == '{' ) {
		return ParseObject(_parser, _allocator);
	}
	else if( c == '(' ) {
		return ParseList(_parser, _allocator);
	}
	else if( c == '[' ) {
		return ParseArray(_parser, _allocator);
	}
	else if( c == 'n' ) {
		return ParseNil(_parser, _allocator);
	}

	ShowParsingError(_parser, "unknown character: '%c'", c);
	return nil;
}

// Reads an identifier and returns its length.
static U32 ReadIdentifier( Parser & _parser )
{
	const U32 start = _parser.position;
	while( !AtEOF(_parser) && IsIdentifierBody(CurrentChar( _parser )) )
	{
		ReadChar( _parser );
	}
	return _parser.position - start;
}

// <assignment> := <key> '=' <value>
static Node* ParseAssignment( Parser & _parser, NodeAllocator & _allocator )
{
	//mxASSERT(_parent->tag.type == TypeTag_Object);

	CharType c = CurrentChar( _parser );

	// hashed identifier: #name
	bool bHashed = false;
	if( c == '#' ) {
		c = ReadChar(_parser);
		bHashed = true;
	}

	if( IsIdentifierHead( c ) )
	{
		const U32 start = _parser.position;
		const U32 length = ReadIdentifier( _parser );
		ReadExpectedCharacter( _parser, '=' );
		InsertNullCharacter(_parser, start + length);
		Node* valueNode = ParseValue( _parser, _allocator );
		if( !valueNode ) {
			return nil;
		}
		//DBGOUT("read value '%s' of type '%s'\n", _parser.buffer+start, ETypeTag_To_Chars(valueNode->tag));
		valueNode->name = _parser.buffer + start;
		if( bHashed ) {
			valueNode->tag.hash = GetDynamicStringHash( valueNode->name );
		}
		return valueNode;
	}
	ShowParsingError(_parser, "unexpected character: '%c'", c);
	return nil;
}

// <list_body> = { <value> }
static U32 ParseListBody( Scope & _parent, Parser & _parser, NodeAllocator & _allocator )
{
	Node* listNode = _parent.node;
	mxASSERT(listNode->tag.type == TypeTag_List);
	U32 childCount = 0;
	for(;;)
	{
		if( !SkipWhitespaces( _parser ) ) {
			break;
		}
		if( CurrentChar(_parser) == ')' ) {
			break;
		}
		Node* valueNode = ParseValue( _parser, _allocator );
		if( !valueNode ) {
			break;
		}

#if txtPARSER_PRESERVE_ORDER
		AppendChild( _parent, valueNode );
#else
		PrependChild( listNode, valueNode );
#endif

		childCount++;
	}
	return childCount;
}

// <array_body> = { <value> }
static U32 ParseArrayBody( Scope & _parent, Parser & _parser, NodeAllocator & _allocator )
{
	Node *	arrayNode = _parent.node;
	mxASSERT(arrayNode->tag.type == TypeTag_Array);
	U32	childCount = 0;
	void *	previousPtr = nil;
	bool bLinearizeChildren = false;
	for(;;)
	{
		if( !SkipWhitespaces( _parser ) ) {
			break;
		}
		if( CurrentChar(_parser) == ']' ) {
			break;
		}
		Node* valueNode = ParseValue( _parser, _allocator );
		if( !valueNode ) {
			break;
		}
		
		if( previousPtr ) {
			if( (size_t)valueNode - (size_t)previousPtr != sizeof(Node) ) {
				bLinearizeChildren = true;
			}
		}
		previousPtr = valueNode;

#if txtPARSER_PRESERVE_ORDER
		AppendChild( _parent, valueNode );
#else
		PrependChild( arrayNode, valueNode );
#endif

		childCount++;
	}

	// array items must be stored contiguously in memory to allow O(1) access
	if( bLinearizeChildren )
	{
		mxASSERT(childCount > 1);

		Node* kids = _allocator.AllocateArray( childCount );

		// copy linked list to array
		{
			Node* src = arrayNode->value.a.kids;
			Node* prev = nil;
			Node* copy = kids;
			while( src )
			{
				if( prev ) {
					prev->next = copy;
				}
				prev = copy;
				*copy = *src;
				copy++;
				Node* next = src->next;
				_allocator.ReleaseNode( src );
				src = next;
			}
		}

		arrayNode->value.a.kids = kids;
	}

	arrayNode->value.a.size = childCount;

	return childCount;
}

// <object_body> = list of <assignment>
static U32 ParseObjectBody( Scope & _parent, Parser & _parser, NodeAllocator & _allocator )
{
	Node* objectNode = _parent.node;
	mxASSERT(objectNode->tag.type == TypeTag_Object);
	U32 childCount = 0;
	for(;;)
	{
		if( !SkipWhitespaces( _parser ) ) {
			break;
		}
		if( CurrentChar(_parser) == '}' ) {
			break;
		}
		Node* childValue = ParseAssignment( _parser, _allocator );
		if( !childValue ) {
			break;
		}

#if txtPARSER_PRESERVE_ORDER
		AppendChild( _parent, childValue );
#else
		PrependChild( objectNode, childValue );
#endif

		childCount++;
	}
	return childCount;
}

Parser::Parser()
{
	this->buffer	= nil;
	this->length	= 0;
	this->position	= 0;
	this->line		= 1;
	this->column	= 0;
	this->file		= "?";
	this->errorCode	= 0;
}

Parser::~Parser()
{
}

Node* ParseBuffer(
				  Parser& _parser,
				  NodeAllocator & nodeAllocator
						)
{
	mxASSERT_PTR(_parser.buffer);
	mxASSERT(_parser.length > 1);

	SkipWhitespaces( _parser );
	if( CurrentChar( _parser ) == '{' )
	{
		return ParseObject( _parser, nodeAllocator );
	}
	else if( CurrentChar( _parser ) == '[' )
	{
		return ParseArray( _parser, nodeAllocator );
	}
	else
	{
		Node* objectValue = nodeAllocator.AllocateNode();
		{
			objectValue->value.u = 0;
			objectValue->next = nil;
			objectValue->tag.type = TypeTag_Object;
			objectValue->name = nil;
		}

		Scope	root;
		root.node = objectValue;
		root.tail = nil;

		ParseObjectBody( root, _parser, nodeAllocator );

		return objectValue;
	}
}

}//namespace SON

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
