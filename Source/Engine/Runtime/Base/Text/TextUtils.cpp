/*
=============================================================================
	File:	TextUtils.cpp
	Desc:
=============================================================================
*/

#include <Base/Base_PCH.h>
#pragma hdrstop
#include <Base/Base.h>

#include <Base/Text/TextUtils.h>



mxOPTIMIZE("make iterative, remove tail recursion");
static UINT BinaryStringSearch_internal(
					   const char* stringArray[], const UINT arraySize,
					   const char* substring, INT low, INT high, UINT charIndex )
{
	// NOTE: signed comparison
	if( high < low ) {
		return INDEX_NONE;	// not found
	}

	//const INT mid = low + (high - low) / 2;	// <- avoids overflow
	const INT mid = ((UINT)low + (UINT)high) / 2;	// <- slightly faster than the above version


	const char* outbuf_ = stringArray[ mid ];

	mxLOOP_FOREVER
	{
		const UINT cA = outbuf_[ charIndex ];
		const UINT cB = substring[ charIndex ];

		// check if null terminator has been reached
		if( cA == 0 || cB == 0 )
		{
			break;
		}

#if MX_DEBUG
		const char ccA = cA;
		const char ccB = cB;
		(void)ccA,(void)ccB;
#endif

		// advance to the next char
		++charIndex;

		if( cA > cB )
		{
			// search in lower (left) half
			return BinaryStringSearch_internal(stringArray,arraySize,substring,
				low,mid-1,
				charIndex
			);
		}

		if( cA < cB )
		{
			// search in upper (right) half
			return BinaryStringSearch_internal(stringArray,arraySize,substring,
				mid+1,high,
				charIndex
			);
		}

		mxASSERT( cA == cB );
	}

	// reached the end of both strings,
	// the strings are equal
	if( 0 == outbuf_[charIndex] && 0 == substring[charIndex] )
	{
		// found
		return mid;
	}

	return INDEX_NONE;	// not found
}
//-------------------------------------------------------------------------
UINT BinaryStringSearch(const char* stringArray[], const UINT arraySize,
					   const char* theString)
{
	mxASSERT_PTR(stringArray);
	mxASSERT(arraySize > 1);
	mxASSERT_PTR(theString);

	return BinaryStringSearch_internal(
		stringArray, arraySize,
		theString,
		0, arraySize-1,
		0
	);
}

//-------------------------------------------------------------------------
// Prints x as a binary number, works for any base between 2 and 10.
//
char* ConvertBaseNToBinary32( U32 x, UINT base, char buf[32] )
{
	char *s = buf + 32;
	*--s = 0;
	if( !x ) {
		*--s = '0';
	}
	for( ; x; x /= base ) {
		*--s = '0' + (x % base);
	}
	return s;
}
//-------------------------------------------------------------------------
void print4BitsInBinary( UINT number, char outbuf_[32] )
{
	for( UINT i = 0; i < 4; i++ )
	{
		outbuf_[ i ] = ( number & 0x80 ) ? '1' : '0';
		number = number << 1;
	}
}
//-------------------------------------------------------------------------
void Print8BitsInBinary( UINT number, char outbuf_[32] )
{
	for( UINT i = 0; i < 8; i++ )
	{
		outbuf_[ i ] = ( number & 0x80 ) ? '1' : '0';
		number = number << 1;
	}
}
//-------------------------------------------------------------------------
// Print n as a binary number. Make sure that buffer outbuf_ has sufficient length.
void printAsBinary( U32 number, char* outbuf_ )
{
	U32 n = 1 << 31;
	UINT i = 0;
	while( n )
	{
		outbuf_[ i++ ] = (n & number) ? '1' : '0';
		n >>= 1;
	}
}
//-------------------------------------------------------------------------

int printAsBinary( U64 number, char *outbuf_, UINT byte_width, char nibble_separator )
{
	U64 tested_bit_mask = U64(1) << (BYTES_TO_BITS(byte_width) - 1);	// test MSB first

	int tested_bit_index = 0;
	int written_char_index = 0;

	while( tested_bit_mask )
	{
		++tested_bit_index;

		outbuf_[ written_char_index++ ] = (tested_bit_mask & number) ? '1' : '0';

		// insert spaces between nibbles
		if( tested_bit_index && (tested_bit_index % 4 == 0) ) {
			outbuf_[ written_char_index++ ] = nibble_separator;
		}

		tested_bit_mask >>= 1;
	}

	return written_char_index;
}

//-------------------------------------------------------------------------
void SkipSpaces(const char *& buffer)
{
	while( *buffer && (*buffer < 32) )
		++buffer;
}

bool IsPathSeparator( char c )
{
	return c == '/' || c == '\\';
}

//-------------------------------------------------------------------------
UINT StripTrailingSlashes( char* s, UINT len )
{
	// scan the string backwards and skip all path separators
	while( len > 0 && IsPathSeparator( s[ len - 1 ] ) )
	{
		--len;
	}
	s[ len ] = 0;
	return len;
}

//-------------------------------------------------------------------------
UINT IndexOfLastChar( const char* s, const UINT len, const char c )
{
	UINT newLen = len;
	while(newLen > 0 && s[newLen] != c )
	{
		--newLen;
	}

	return newLen < len ? newLen : INDEX_NONE;
}
//-------------------------------------------------------------------------
int ParseInteger(const char *p)
{
	int s = p[0]=='-' ? -1 : 1;
	p += (p[0]=='-') | (p[0]=='+');
	int r = 0;
	for(; p[0]>='0' && p[0]<='9'; ++p)
		r = r*10 + (int)(p[0] - '0');
	return r*s;
}
//-------------------------------------------------------------------------
float ParseFloat(const char* p)
{
	return c_cast(float)ParseDouble(p);
}
//-------------------------------------------------------------------------
double ParseDouble(const char* p)
{
	float s = p[0]=='-' ? -1.0f : 1.0f;
	p += (p[0]=='-') | (p[0]=='+');
	double r = 0;
	for(; p[0]>='0' && p[0]<='9'; ++p)
		r = r*10.0 + (double)(p[0] - '0');
	if(p[0]=='.' || p[0]==',')
	{
		double k = 0.1;
		for(++p; p[0]>='0' && p[0]<='9'; ++p, k *= 0.1)
			r += k*(double)(p[0] - '0');
	}
	if(p[0]=='e' || p[0]=='E')
		r *= pow(10.0, (double)ParseInteger(p + 1));
	return r*s;
}
//-------------------------------------------------------------------------
U32 StringToU32( const char* s, U32 minValue, U32 maxValue )
{
	U32 v = ParseInteger(s);
	mxASSERT(v >= minValue);
	mxASSERT(v <= maxValue);
	return v;
}
//-------------------------------------------------------------------------
FLOAT StringToF32( const char* s, FLOAT minValue, FLOAT maxValue )
{
	FLOAT v = ParseDouble(s);
	mxASSERT(v >= minValue);
	mxASSERT(v <= maxValue);
	return v;
}
//-------------------------------------------------------------------------
/**
 * C++ version 0.4 char* style "itoa":
 * Written by Lukás Chmela
 * Released under GPLv3.
 */
char* my_itoa(int value, char* result, int base) {
	// check that the base if valid
	if (base < 2 || base > 36) { *result = '\0'; return result; }

	char* ptr = result, *ptr1 = result, tmp_char;
	int tmp_value;

	do {
		tmp_value = value;
		value /= base;
		*ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz" [35 + (tmp_value - value * base)];
	} while ( value );

	// Apply negative sign
	if (tmp_value < 0) *ptr++ = '-';
	*ptr-- = '\0';
	while(ptr1 < ptr) {
		tmp_char = *ptr;
		*ptr--= *ptr1;
		*ptr1++ = tmp_char;
	}
	return result;
}
//-------------------------------------------------------------------------
int naive_char_2_int(const char *p) {
	int x = 0;
	bool neg = false;
	if (*p == '-') {
		neg = true;
		++p;
	}
	while (*p >= '0' && *p <= '9') {
		x = (x*10) + (*p - '0');
		++p;
	}
	if (neg) {
		x = -x;
	}
	return x;
}
//-------------------------------------------------------------------------
// fast solution for strings up to about 32 characters
// See:http://www.azillionmonkeys.com/qed/asmexample.html
mxNO_INLINE
int xstrlen (const char *s)
{
#define hasNulByte(x) ((x - 0x01010101) & ~x & 0x80808080)
#define SW (sizeof (int) / sizeof (char))

	const char *p;
	int d;

	p = s - 1;
	do {
		p++;
		if ((((int) p) & (SW - 1)) == 0) {
			do {
				d  = *((int *) p);
				p += SW;
			} while (!hasNulByte (d));
			p -= SW;
		}
	} while (*p != 0);
	return p - s;

#undef hasNulByte
#undef SW
}
//-------------------------------------------------------------------------
// Four bytes are examined at once.
// The program reads a double word from memory,
// extracts each of its bytes by ANDing with a mask,
// and compares the bytes with zero.
// That is what Agner Fog calls "vector operations in general purpose registers".
/*
Warning: this function will crash if an non-readable memory page is located right after the end of the string. The simplest way to prevent this is to allocate 3 additional bytes at the end of string. (Хвостики в массивах!;-))
The dwords may be unaligned, but x86 architecture allows access to unaligned data. For small strings, the alignment will take more time than the penalty of unaligned reads.
The code is not portable: you will have to add another 4 conditions if you use a 64-bit processor. For big-endian architectures, the order of conditions should be reversed.
*/
mxNO_INLINE
size_t swar_strlen(const char *s) {
	size_t len = 0;
#if 1
	for(;;) {
		unsigned x = *(unsigned*)s;
		if((x & 0xFF) == 0) return len;
		if((x & 0xFF00) == 0) return len + 1;
		if((x & 0xFF0000) == 0) return len + 2;
		if((x & 0xFF000000) == 0) return len + 3;
		s += 4, len += 4;
	}
#else
	// this version is slightly faster on a long string because of aligned memory access.
	const char *p = s;
	switch ( (intptr_t)p & 3 ) {
		case 3:
			if (*p++ == 0)
				return p - s;
		case 2:
			if (*p++ == 0)
				return p - s;
		case 1:
			if (*p++ == 0)
				return p - s;
	}

	for(;;) {
		unsigned x = *(unsigned*)p;
		if((x & 0xFF) == 0) return p - s;
		if((x & 0xFF00) == 0) return p - s + 1;
		if((x & 0xFF0000) == 0) return p - s + 2;
		if((x & 0xFF000000) == 0) return p - s + 3;
		p += 4;
	}
#endif
}
//-------------------------------------------------------------------------




//----------------------------------------------------------//
//				End Of File.									//
//----------------------------------------------------------//
