/*
=============================================================================
	File:	TextUtils.h
	Desc:
=============================================================================
*/
#pragma once

#include <inttypes.h>	// PRIx64


// searches the specified string in the given array of strings
// and returns the index of the string
// returns INDEX_NONE if not found
UINT BinaryStringSearch(const char* stringArray[], const UINT arraySize,
					   const char* theString);


void SkipSpaces(const char *& buffer);
bool IsPathSeparator( char c );
UINT StripTrailingSlashes( char* s, UINT len );
UINT IndexOfLastChar( const char* s, const UINT len, const char c );

int ParseInteger(const char *p);
float ParseFloat(const char* p);
double ParseDouble(const char* p);

U32 StringToU32( const char* s, U32 minValue, U32 maxValue );
FLOAT StringToF32( const char* s, FLOAT minValue, FLOAT maxValue );

char* ConvertBaseNToBinary32( U32 x, UINT base, char buf[32] );
void print4BitsInBinary( UINT number, char outbuf_[32] );
void Print8BitsInBinary( UINT number, char outbuf_[32] );

/// Print n as a binary number. Make sure that buffer outbuf_ has sufficient length.
/// Inserts spaces between nibbles.
/// Returns the number of characters written.
///
int printAsBinary( U64 number, char *outbuf_, UINT byte_width, char nibble_separator = ' ' );

template< typename INTEGER >
int printAsBinary( INTEGER number, char *outbuf_, char nibble_separator = ' ' )
{
	return printAsBinary( number, outbuf_, sizeof(number), nibble_separator );
}



template< UINT LENGTH >
static inline
bool StringStartsWith( const char* outbuf_, const char (&substr)[LENGTH] )
{
	return strncmp( outbuf_, substr, LENGTH-1 ) == 0;
}

///
template< typename INTEGER >
void Dev_PrintArrayAsHex(
						 const INTEGER* values
						 , unsigned values_count
						 , unsigned num_columns = 1
						 , bool fixed_width_pad_with_zeros = true
						 )
{
	const char* fmt = "0x%x, ";

	if(sizeof(values[0]) == 4)
	{
		fmt = fixed_width_pad_with_zeros
			? "0x%08x, "
			: "0x%x, "
			;
	}
	else if(sizeof(values[0]) == 8)
	{
		fmt = fixed_width_pad_with_zeros
			? "%#018"PRIx64", "
			: "0x%"PRIx64", "
			;
	}
	else if(sizeof(values[0]) == 2)
	{
		fmt = fixed_width_pad_with_zeros
			? "0x%04x, "
			: "0x%x, "
			;
	}

	const unsigned numRows = values_count / num_columns;

	unsigned valuesPrinted = 0;

	for( unsigned iRow = 0; iRow < numRows; iRow++ )
	{
		for( unsigned iCol = 0; iCol < num_columns; iCol++ )
		{
			DBG_PRINTF( fmt, values[ valuesPrinted++ ] );
			if( valuesPrinted > values_count ) {
				goto L_End;
			}
		}
		DBG_PRINTF( "\n" );
	}
L_End:
	DBG_PRINTF( "\n" );
}
