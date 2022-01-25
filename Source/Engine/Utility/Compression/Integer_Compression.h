#pragma once

#include <algorithm>
#include <Base/Template/Containers/Blob.h>
#include <Base/Template/MovingAverage.h>

/**
 * For signed numbers, we use an encoding called zig-zag encoding
 * that maps signed numbers to unsigned numbers so they can be
 * efficiently encoded using the normal variable-int encoder.
 */

/// Folds signed to unsigned; the actual formula is: is_negative(v)? -(2v+1) : 2v.
static mxFORCEINLINE
U32 EncodeZigZag32(I32 n)
{
	return (n << 1) ^ (n >> 31);
}

static mxFORCEINLINE
I32 DecodeZigZag32(U32 n)
{
	return (n >> 1) ^ -(I32)(n & 1);
}

static mxFORCEINLINE
U64 EncodeZigZag64(I64 n)
{
	return (n << 1) ^ (n >> 63);
}

static mxFORCEINLINE
I64 DecodeZigZag64(U64 n)
{
	return (n >> 1) ^ -(I64)(n & 1);
}


/// Vbyte (1972?)
/// - 1-5 bytes for a 32 bit unsigned integer
/// - Uses one bit out of every byte to describe continuation – are there more bytes left?
namespace VByte
{
	static inline int
	Write_U32(U8 *p, U32 value)
	{
		if (value < (1u << 7)) {
			*p = value & 0x7Fu;
			return 1;
		}
		if (value < (1u << 14)) {
			*p = (value & 0x7Fu) | (1u << 7);
			++p;
			*p = value >> 7;
			return 2;
		}
		if (value < (1u << 21)) {
			*p = (value & 0x7Fu) | (1u << 7);
			++p;
			*p = ((value >> 7) & 0x7Fu) | (1u << 7);
			++p;
			*p = value >> 14;
			return 3;
		}
		if (value < (1u << 28)) {
			*p = (value & 0x7Fu) | (1u << 7);
			++p;
			*p = ((value >> 7) & 0x7Fu) | (1u << 7);
			++p;
			*p = ((value >> 14) & 0x7Fu) | (1u << 7);
			++p;
			*p = value >> 21;
			return 4;
		}
		else {
			*p = (value & 0x7Fu) | (1u << 7);
			++p;
			*p = ((value >> 7) & 0x7Fu) | (1u << 7);
			++p;
			*p = ((value >> 14) & 0x7Fu) | (1u << 7);
			++p;
			*p = ((value >> 21) & 0x7Fu) | (1u << 7);
			++p;
			*p = value >> 28;
			return 5;
		}
	}

	static inline int
	Read_U32(const U8 *in, U32 *out)
	{
		*out = in[0] & 0x7Fu;
		if (in[0] < 128)
			return 1;
		*out = ((in[1] & 0x7Fu) << 7) | *out;
		if (in[1] < 128)
			return 2;
		*out = ((in[2] & 0x7Fu) << 14) | *out;
		if (in[2] < 128)
			return 3;
		*out = ((in[3] & 0x7Fu) << 21) | *out;
		if (in[3] < 128)
			return 4;
		*out = ((in[4] & 0x7Fu) << 28) | *out;
		return 5;
	}
}//namespace VByte

mxSTOLEN("simple9, Copyright (c) 2014-2015, Conor Stokes");
/// simple9, a word-aligned integer compression algorithm as described in
/// Vo Ngoc Anh and Alistair Moffat. Inverted index compression using
/// word-aligned binary codes. Information Retrieval, 8(1):151вЂ“166, 2005.
namespace Simple9
{

#define U32_BITS (sizeof(U32) * CHAR_BIT)

/// The lower 4 bits store the type of data (selector)
#define SIMPLE9_SELECTOR_BITS 4
#define SIMPLE9_SELECTOR_MASK 0x0000000F

/// 28 bits
#define SIMPLE9_CODE_BITS (U32_BITS - SIMPLE9_SELECTOR_BITS)

/// The maximum integer value that can be packed using Simple-9
#define SIMPLE9_MAX_VALUE ((1UL << SIMPLE9_CODE_BITS) - 1)


class StreamWriter
{
	U32	m_buffer[28];
	U32	m_bufferedItems;	// [0..28]
	NwBlobWriter &	m_writer;

public:
	StreamWriter( NwBlobWriter &_writer )
		: m_writer( _writer )
	{
		m_bufferedItems = 0;
	}
	~StreamWriter()
	{
		mxASSERT2(m_bufferedItems == 0, "Not all items have been written!");
	}

	void Write_U32( const U32 _newValue )
	{
		mxASSERT(_newValue <= SIMPLE9_MAX_VALUE);
		m_buffer[ m_bufferedItems++ ] = _newValue;

		if( m_bufferedItems == 28 ) {
			this->TryFlush();
		}
	}

	/// Finish working, flushing any pending data.
	void Finish();

private:
	void TryFlush();
};


class StreamReader
{
	U32	m_buffer[28];	//!< this is a stack, i.e. elements are appended and removed at the end
	U32	m_bufferedItems;	// [0..28]
	const U32 *	m_get;

public:
	StreamReader( const U32* input )
		: m_get( input )
	{
		m_bufferedItems = 0;
	}
	~StreamReader()
	{
		mxASSERT2(m_bufferedItems == 0, "Not all items have been read!");
	}
	U32 Read_U32();
};

}//namespace Simple9

#if 0

static inline char* VByte_Encode32( char *sptr, U32 v )
{
  unsigned char *ptr = (unsigned char *)sptr;
  if(v < (1u <<  7)) { *(ptr++) = v; }
  else if(v < (1u << 14)) { *(ptr++) = v | 0x80u; *(ptr++) = (v>>7); }
  else if(v < (1u << 21)) { *(ptr++) = v | 0x80u; *(ptr++) = (v>>7) | 0x80u; *(ptr++) = (v>>14); }
  else if(v < (1u << 28)) { *(ptr++) = v | 0x80u; *(ptr++) = (v>>7) | 0x80u; *(ptr++) = (v>>14) | 0x80u; *(ptr++) = (v>>21); }
  else                    { *(ptr++) = v | 0x80u; *(ptr++) = (v>>7) | 0x80u; *(ptr++) = (v>>14) | 0x80u; *(ptr++) = (v>>21) | 0x80u; *(ptr++) = (v>>28); }
  return((char *)ptr);
}

PITHY_STATIC_INLINE const char *pithy_Parse32WithLimit(const char *p, const char *l, size_t *resultOut) {
  const unsigned char *ptr = (const unsigned char *)p, *limit = (const unsigned char *)l;
  size_t   resultShift = 0ul, result = 0ul;
  U32 encodedByte = 0u;
  
  for(resultShift = 0ul; resultShift <= 28ul; resultShift += 7ul) { if(ptr >= limit) { return(NULL); } encodedByte = *(ptr++); result |= (encodedByte & 127u) << resultShift; if(encodedByte < ((resultShift == 28ul) ? 16u : 128u)) { goto done; } }
  return(NULL);
done:
  *resultOut = result;
  return((const char *)ptr);
}
#endif

#if 0
/** 
 * C++ utilities for variable-length integer encoding
 * Compile with -std=c++11 or higher
 * 
 * Version 1.1: Use size_t for size argument, improve function signatures
 * 
 * License: CC0 1.0 Universal
 * Originally published on http://techoverflow.net
 * Copyright (c) 2015 Uli Koehler
 */
/**
 * Encodes an unsigned variable-length integer using the MSB algorithm.
 * This function assumes that the value is stored as little endian.
 * @param value The input value. Any standard integer type is allowed.
 * @param output A pointer to a piece of reserved memory. Must have a minimum size dependent on the input size (32 bit = 5 bytes, 64 bit = 10 bytes).
 * @return The number of bytes used in the output memory.
 */
template<typename int_t = U64_t>
size_t encodeVarint(int_t value, uint8_t* output) {
    size_t outputSize = 0;
    //While more than 7 bits of data are left, occupy the last output byte
    // and set the next byte flag
    while (value > 127) {
        //|128: Set the next byte flag
        output[outputSize] = ((uint8_t)(value & 127)) | 128;
        //Remove the seven bits we just wrote
        value >>= 7;
        outputSize++;
    }
    output[outputSize++] = ((uint8_t)value) & 127;
    return outputSize;
}

/**
 * Decodes an unsigned variable-length integer using the MSB algorithm.
 * @param value A variable-length encoded integer of arbitrary size.
 * @param inputSize How many bytes are 
 */
template<typename int_t = U64_t>
int_t decodeVarint(uint8_t* input, size_t inputSize) {
    int_t ret = 0;
    for (size_t i = 0; i < inputSize; i++) {
        ret |= (input[i] & 127) << (7 * i);
        //If the next-byte flag is set
        if(!(input[i] & 128)) {
            break;
        }
    }
    return ret;
}
#endif
/*--

This file is a part of bsc and/or libbsc, a program and a library for
lossless, block-sorting data compression.

   Copyright (c) 2009-2012 Ilya Grebnov <ilya.grebnov@gmail.com>

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

Please see the file LICENSE for full copyright information and file AUTHORS
for full list of contributors.

See also the bsc and libbsc web site:
  http://libbsc.com/ for more information.

--*/
class RangeCoder
{
    union ari
    {
        struct u
        {
            unsigned int low32;
            unsigned int carry;
        } u;
        unsigned long long low;
    } ari;

    unsigned int ari_code;
    unsigned int ari_ffnum;
    unsigned int ari_cache;
    unsigned int ari_range;

    const unsigned short * __restrict ari_input;
          unsigned short * __restrict ari_output;
          unsigned short * __restrict ari_outputEOB;
          unsigned short * __restrict ari_outputStart;

    mxINLINE void OutputShort(unsigned short s)
    {
        *ari_output++ = s;
    };

    mxINLINE unsigned short InputShort()
    {
        return *ari_input++;
    };

    mxINLINE void ShiftLow()
    {
        if (ari.u.low32 < 0xffff0000U || ari.u.carry)
        {
            OutputShort(ari_cache + ari.u.carry);
            if (ari_ffnum)
            {
                unsigned short s = ari.u.carry - 1;
                do { OutputShort(s); } while (--ari_ffnum);
            }
            ari_cache = ari.u.low32 >> 16; ari.u.carry = 0;
        } else ari_ffnum++;
        ari.u.low32 <<= 16;
    }

public:

    mxINLINE bool CheckEOB()
    {
        return ari_output >= ari_outputEOB;
    }

    mxINLINE void InitEncoder(unsigned char * output, int outputSize)
    {
        ari_outputStart = (unsigned short *)output;
        ari_output      = (unsigned short *)output;
        ari_outputEOB   = (unsigned short *)(output + outputSize - 16);
        ari.low         = 0;
        ari_ffnum       = 0;
        ari_cache       = 0;
        ari_range       = 0xffffffff;
    };

    mxINLINE int FinishEncoder()
    {
        ShiftLow(); ShiftLow(); ShiftLow();
        return (int)(ari_output - ari_outputStart) * sizeof(ari_output[0]);
    }

    mxINLINE void EncodeBit0(int probability)
    {
        ari_range = (ari_range >> 12) * probability;
        if (ari_range < 0x10000)
        {
            ari_range <<= 16; ShiftLow();
        }
    }

    mxINLINE void EncodeBit1(int probability)
    {
        unsigned int range = (ari_range >> 12) * probability;
        ari.low += range; ari_range -= range;
        if (ari_range < 0x10000)
        {
            ari_range <<= 16; ShiftLow();
        }
    }

    mxINLINE void EncodeBit(unsigned int bit)
    {
        if (bit) EncodeBit1(2048); else EncodeBit0(2048);
    };

    mxINLINE void EncodeByte(unsigned int byte)
    {
        for (int bit = 7; bit >= 0; --bit)
        {
            EncodeBit(byte & (1 << bit));
        }
    };

    mxINLINE void EncodeWord(unsigned int word)
    {
        for (int bit = 31; bit >= 0; --bit)
        {
            EncodeBit(word & (1 << bit));
        }
    };

    mxINLINE void InitDecoder(const unsigned char * input)
    {
        ari_input = (unsigned short *)input;
        ari_code  = 0;
        ari_range = 0xffffffff;
        ari_code  = (ari_code << 16) | InputShort();
        ari_code  = (ari_code << 16) | InputShort();
        ari_code  = (ari_code << 16) | InputShort();
    };

    mxINLINE int DecodeBit(int probability)
    {
        unsigned int range = (ari_range >> 12) * probability;
        if (ari_code >= range)
        {
            ari_code -= range; ari_range -= range;
            if (ari_range < 0x10000)
            {
                ari_range <<= 16; ari_code = (ari_code << 16) | InputShort();
            }
            return 1;
        }
        ari_range = range;
        if (ari_range < 0x10000)
        {
            ari_range <<= 16; ari_code = (ari_code << 16) | InputShort();
        }
        return 0;
    }

    mxINLINE unsigned int DecodeBit()
    {
        return DecodeBit(2048);
    }

    mxINLINE unsigned int DecodeByte()
    {
        unsigned int byte = 0;
        for (int bit = 7; bit >= 0; --bit)
        {
            byte += byte + DecodeBit();
        }
        return byte;
    }

    mxINLINE unsigned int DecodeWord()
    {
        unsigned int word = 0;
        for (int bit = 31; bit >= 0; --bit)
        {
            word += word + DecodeBit();
        }
        return word;
    }

};


static mxFORCEINLINE
DWORD CLZ( DWORD _value )
{
	DWORD leadingZero = 0;
	_BitScanReverse( &leadingZero, _value );
	return 31 - leadingZero;
}

static mxFORCEINLINE
UINT RequiredBits( U32 _value )
{
	return _value ? 32 - CLZ(_value) : 0;
}

struct Histograms32
{
	U32		totalValues;
	U32		maxValue;
	/// number of values less than the ceil(value) power of two
	/// (e.g. [0] - number of values less than 1 == 2^0)
	U32		histograms[ 32 ];

public:
	Histograms32()
	{
		mxZERO_OUT(*this);
	}
	void AddValue( const U32 _newValue )
	{
		for( UINT j = 0; j < mxCOUNT_OF(histograms); j++ )
		{
			const U32 upperValue = ( 1ul << j );
			if( _newValue < upperValue )
			{
				histograms[ j ]++;
				break;
			}
		}
		totalValues++;
		maxValue = Max(maxValue, _newValue);
	}
	void Print( LogStream & stream )
	{
		for( UINT j = 0; j < mxCOUNT_OF(histograms); j++ )
		{
			const U32 lowerBound = ( j > 0 ) ? ( 1ul << ( j - 1 ) ) : 0;
			const U32 upperBound = ( 1ul << j );

			const U32 numValuesInRange = histograms[ j ];

			if( numValuesInRange )
			{
				const float percentage = (float(numValuesInRange) / float(totalValues)) * 100.0f;
				//stream << numValuesInRange << "(" << percentage << "%) values in [ " << lowerBound << " .. " << upperBound << " )\n";
				stream.PrintF("%u (%.3f%%) values in [%u .. %u)\n", numValuesInRange, percentage, lowerBound, upperBound );
			}
		}
	}
};

/// A utility for analyzing distributions (for choosing an optimal compression method).
struct SequenceAnalyzer
{
	struct Results
	{
		float	averageRunLength;	//!< simple average
		//U32		meanRunLength;	//!< about half of all L are less and about half are greater or equal to the mean
		Histograms32	values;	//!< distribution of values
		Histograms32	runs;	//!< distribution of run-lengths
		U32	byteFrequencies[ 256 ];
	};

	void Add_U32( const U32 _newValue )
	{
		{
			U32 currentRunLength = m_currentRunLength;
			const U32 previousValue = m_currentValue;
			if( previousValue == _newValue )
			{
				++currentRunLength;
			}
			else
			{
				// flush the current run
				m_results.averageRunLength = m_averageRunLength.calculate( currentRunLength );
				m_results.runs.AddValue( currentRunLength - 1 );

				// start a new run
				currentRunLength = 1;
				m_currentValue = _newValue;
			}
			m_currentRunLength = currentRunLength;
		}

		m_results.values.AddValue( _newValue );

		m_results.byteFrequencies[ (_newValue & 0xFF) ]++;
		m_results.byteFrequencies[ (_newValue >> 8) & 0xFF ]++;
		m_results.byteFrequencies[ (_newValue >> 16) & 0xFF ]++;
		m_results.byteFrequencies[ (_newValue >> 24) & 0xFF ]++;
	}

	SequenceAnalyzer()
	{
		m_currentValue = 0;
		m_currentRunLength = 1;
		mxZERO_OUT(m_results);
	}

	void Print( const char* _name = "integers" )
	{
		LogStream	stream(LL_Info);
		stream.PrintF( "=== SequenceAnalyzer reporting: %u %s ===\n", m_results.values.totalValues, _name );
		stream.PrintF( "-- Maximum Value: %u (%u bits)\n", m_results.values.maxValue, RequiredBits(m_results.values.maxValue) );
		stream << "-- Values:\n";
		m_results.values.Print( stream );
		stream << "-- Average Run Length: " << m_results.averageRunLength << "\n";
		stream << "-- Maximum Run Length: " << m_results.runs.maxValue << "\n";
		stream << "-- Runs:\n";
		m_results.runs.Print( stream );

		if(1)
		{
			stream << "-- Byte frequencies:\n";
			TStaticArray< U8, 256 >	charsByFreq;
			U32	totalCount = 0;
			for( int iByte = 0; iByte < 256; iByte++ ) {
				charsByFreq[ iByte ] = iByte;
				totalCount += m_results.byteFrequencies[ iByte ];
			}
			class CmpByteFreq {
				const U32* m_byteFrequencies;
			public:
				CmpByteFreq( const U32* byteFrequencies )
					: m_byteFrequencies( byteFrequencies )
				{}
				bool operator () ( U8 a, U8 b ) {
					return m_byteFrequencies[a] > m_byteFrequencies[b];
				}
			};
			std::sort( charsByFreq.begin(), charsByFreq.end(), CmpByteFreq(m_results.byteFrequencies) );

			double averageBitsPerSymbol = 0.;
			for( int iByte = 0; iByte < 256; iByte++ )
			{
				const UINT byteIndex = charsByFreq[ iByte ];
				const UINT frequency = m_results.byteFrequencies[ byteIndex ];
				if( frequency )
				{
					const double probability = double(frequency) / totalCount;
					const double bitsRequired = -log( probability );
					averageBitsPerSymbol += bitsRequired;
					stream.PrintF( "Byte[%d] occurs %u times (%.2f%%, %.3lf bits)\n", byteIndex, frequency, probability * 100.0f, bitsRequired );
				}
			}

			averageBitsPerSymbol = averageBitsPerSymbol / 256.0;
			stream.PrintF( "Average bits per symbol: %.3lf\n", averageBitsPerSymbol );
		}
	}

private:
	U32	m_currentValue;
	U32	m_currentRunLength;	//!< accumulated run length ('repeat count'), should always be > 0
	TSimpleMovingAverage< float >	m_averageRunLength;
	Results	m_results;
};

#if 0
/// Please, see "Elias-Fano: quasi-succinct compression of sorted integers in C#".
template< class STREAM >
U32 Compress_Sorted_Integers_Elias_Fano(
										const U32* __restrict input,
										U32 _count,
										STREAM &_writer
										)
{
int compressedBufferPointer2 = 0;	// out
unsigned char	compressedBuffer[4096];

	// Elias-Fano Coding
	// compress sorted integers: Given n and u we have a monotone sequence 0 ? x0, x1, x2, ... , xn-1 ? u
	// at most 2 + log(u / n) bits per element
	// Quasi-succinct: less than half a bit away from succinct bound!
	// https://en.wikipedia.org/wiki/Unary_coding
	// http://vigna.di.unimi.it/ftp/papers/QuasiSuccinctIndices.pdf
	// http://shonan.nii.ac.jp/seminar/029/wp-content/uploads/sites/12/2013/07/Sebastiano_Shonan.pdf
	// http://www.di.unipi.it/~ottavian/files/elias_fano_sigir14.pdf
	// http://hpc.isti.cnr.it/hpcworkshop2014/PartitionedEliasFanoIndexes.pdf

	const U32 largestblockID = (U32)input[ _count - 1 ];	// U - the largest value
	const double averageDelta = (double)largestblockID / (double)_count;
	const double averageDeltaLog2 = logf(averageDelta);	// log2(U/N)
	const UINT numLowBits = Max( (int)floorf(averageDeltaLog2), 0 );	// L = max(0, log2(U/N))

	// Masks out higher bits of x leaving only n lower ones
	const U32 lowBitsMask = (1u << numLowBits) - 1u;	// mask to extract L low bits from a number

	// The (monotonically increasing) sequence (of N numbers) is compressed
	// into two bit arrays which store in concatenated form:
	// 1) the lower L = max(0, log2(u/n)) bits of each integer;
	// 2) the upper bits of each delta/gap as unary codes.
	// (NOTE: The first array stores fixed-size items, the second one store variable-sized integers.)

	int compressedBufferPointer1 = 0;

	// +6 : for docid number, lowerBitsLength and ceiling
	compressedBufferPointer2 = (numLowBits * _count) / 8 + 6; 

	// store _count for decompression: LSB first
	compressedBuffer[compressedBufferPointer1++] = (unsigned char)(_count & 255);
	compressedBuffer[compressedBufferPointer1++] = (unsigned char)((_count >> 8) & 255);
	compressedBuffer[compressedBufferPointer1++] = (unsigned char)((_count >> 16) & 255);
	compressedBuffer[compressedBufferPointer1++] = (unsigned char)((_count >> 24) & 255);

	// store lowerBitsLength for decompression
	compressedBuffer[compressedBufferPointer1++] = (unsigned char)numLowBits;



	U32 buffer1 = 0;
	int bitsAccumulated1 = 0;
	U32 buffer2 = 0;
	int bitsAccumulated2 = 0;

	U32 lastDocID = 0;

	for( U32 i = 0; i < _count; i++ )
	{
		const U32 docID = input[ i ];

		// docID strictly monotone/increasing numbers, docIdDelta strictly positive, no zero allowed
		U32 docIdDelta = (docID - lastDocID - 1);
		lastDocID = docID;

		// low bits
		// Store the lower l= log(u / n) bits explicitly
		// binary packing/bit packing

		buffer1 <<= numLowBits;
		buffer1 |= (docIdDelta & lowBitsMask);
		bitsAccumulated1 += numLowBits;

		// flush buffer to compressedBuffer
		while (bitsAccumulated1 > 7)
		{
			bitsAccumulated1 -= 8;
			compressedBuffer[compressedBufferPointer1++] = (unsigned char)(buffer1 >> bitsAccumulated1);
		}

		// high bits
		// Store high bits as a sequence of unary coded gaps
		// 0=1, 1=01, 2=001, 3=0001, ...
		// https://en.wikipedia.org/wiki/Unary_coding

		// length of unary code 
		U32 unaryCodeLength = (U32)(docIdDelta >> numLowBits) + 1; 
		buffer2 <<= (int)unaryCodeLength;

		// set most right bit 
		buffer2 |= 1;
		bitsAccumulated2 += (int)unaryCodeLength;

		// flush buffer to compressedBuffer
		while (bitsAccumulated2 > 7)
		{
			bitsAccumulated2 -= 8; 
			compressedBuffer[compressedBufferPointer2++] = (unsigned char)(buffer2 >> bitsAccumulated2);  
		}
	}

	// final flush buffer
	if (bitsAccumulated1 > 0)
	{
		compressedBuffer[compressedBufferPointer1++] = (unsigned char)(buffer1 << (8 - bitsAccumulated1));
	}

	if (bitsAccumulated2 > 0)
	{
		compressedBuffer[compressedBufferPointer2++] = (unsigned char)(buffer2 << (8 - bitsAccumulated2));
	}

	//Console.WriteLine("\rCompression:   " + sw.ElapsedMilliseconds.ToString("N0") + " ms  " + _count.ToString("N0") +" DocID  delta: " + averageDelta.ToString("N2") + "  low bits: " + numLowBits.ToString() + "   bits/DocID: " + ((double)compressedBufferPointer2 * (double)8 / (double)_count).ToString("N2")+" (" + (2+averageDeltaLog2).ToString("N2")+")  uncompressed: " + ((U32)_count*4).ToString("N0") + "  compressed: " + compressedBufferPointer2.ToString("N0") +"  ratio: "+ ((double)_count * 4/ compressedBufferPointer2).ToString("N2")) ;

	const float ratio = float(_count*sizeof(input[0])) / compressedBufferPointer2;

	return 0;
}
#endif

inline float CalculateCompressionRatio( int _uncompressed, int _compressed )
{
	return ( _uncompressed - _compressed ) * 100.0f / _uncompressed;
}

/*
===============================================================================
	Variable bit length I/O

	The encoder has to construct each code from individual bits and pieces,
	has to accumulate and append several such codes in a short buffer,
	wait until n bytes of the buffer are full of code bits (where n must be at least 1),
	write the n bytes onto the output, shift the buffer n bytes,
	and keep track of the location of the last bit placed in the buffer.
===============================================================================
*/

#if 0
class BitWriter
{
	//NOTE: need to use a 64-bit bit buffer for writing 32-bit words, because U32(1 << 32) is undefined.
	//(the bit buffer is filled with values from lowest to highest bits)
	U64	m_BitBuffer;	//!< The next bits to be written to output stream; flushed to output when filled
	U32	m_BitsFilled;	//!< The number of valid, lower bits already filled in the bit buffer and pending output

	///// the capacity of the bit buffer, in bits
	//enum { MAX_BITS_IN_BUFFER = CHAR_BIT * sizeof(U64) };

	U32 *	m_start;
	U32 *	m_put;

public:
	BitWriter( U32 * _output )
		: m_start( _output ), m_put( _output )
	{
		m_BitBuffer = 0;
		m_BitsFilled = 0;
	}

	void Write_U32( const U32 _newValue )
	{
		*m_put++ = _newValue;
	}

	/// Writes N lower bits of X
	void WriteBits( U32 _value, U32 _numBits )
	{
		mxASSERT(_numBits <= 32);

		const U32 MAX_BITS_IN_BUFFER = CHAR_BIT * sizeof(m_BitBuffer) / 2;	// 64-bit buffer for handling 32-bit overflow

		//const U32 valueMask = U64(_value) << m_BitsFilled;
		const U64 tmp0 = U64(_value);
		const U64 tmp1 = tmp0 << m_BitsFilled;

		m_BitBuffer |= U64(_value) << m_BitsFilled;	// write Value into BitBuffer at numBitsFilled starting from LSB
		m_BitsFilled += _numBits;
		if( m_BitsFilled > MAX_BITS_IN_BUFFER )	// the bit buffer is overflown
		{
			*m_put++ = (U32)m_BitBuffer;	// flush the accumulated bit buffer to the output
			m_BitsFilled -= MAX_BITS_IN_BUFFER;	// numBitsFilled := the number of remaining, carryover/leftover bits
			// the bit buffer was flushed, overwrite it with the remaining bits
			m_BitBuffer = _value >> ( _numBits - m_BitsFilled );	// place the remaining Value bits into BitBuffer
		}
	}

	void WriteZeroes( U32 _numBits )
	{
		this->WriteBits( 0, _numBits );
	}

	/// Finish working, flushing any pending data.
	U32 Finish()
	{
		this->Flush();
		return m_put - m_start;
	}

	void Flush()
	{
		if( m_BitsFilled )
		{
			*m_put++ = m_BitBuffer;
			m_BitsFilled = 0;
		}
	}
};

#endif

#if 0
#define SET_UINT16_RAW(x,val)	(*(U16*)(x) = (U16)(val))
#define SET_U32_RAW(x,val)	(*(U32*)(x) = (U32)(val))
#define SET_UINT16(x,val)	(*(U16*)(x) = (U16)(val))
#define SET_U32(x,val)	(*(U32*)(x) = (U32)(val))

class BitWriter
{
private:
	BYTE* out;
	U16* pntr[2];	// the uint16's to write the data in mask to when there are enough bits
	U32 mask;		// The next bits to be read/written in the bitstream
	U32 bits;	// The number of bits in mask that are valid
	BYTE* start;
public:
	inline BitWriter(BYTE* out) : out(out+4), mask(0), bits(0)
	{
		mxASSERT(out);
		start = out;
		this->pntr[0] = (U16*)(out);
		this->pntr[1] = (U16*)(out+2);
	}
	//__forceinline bytes RawStream() { return this->out; }
	inline void WriteBits(U32 b, U32 n)
	{
		mxASSERT(n <= 16);
		this->mask |= b << (32 - (this->bits += n));
		if (this->bits > 16)
		{
			SET_UINT16(this->pntr[0], this->mask >> 16);
			this->mask <<= 16;
			this->bits &= 0xF; //this->bits -= 16;
			this->pntr[0] = this->pntr[1];
			this->pntr[1] = (U16*)(this->out);
			this->out += 2;
		}
	}
	__forceinline void WriteRawByte(BYTE x)       { *this->out++ = x; }
	__forceinline void WriteRawUInt16(U16 x) { SET_UINT16(this->out, x); this->out += 2; }
	__forceinline void WriteRawU32(U32 x) { SET_U32(this->out, x); this->out += 4; }
	__forceinline ptrdiff_t Finish()
	{
		SET_UINT16(this->pntr[0], this->mask >> 16); // if !bits then mask is 0 anyways
		SET_UINT16_RAW(this->pntr[1], 0);
		return ptrdiff_t(out) - ptrdiff_t(start);
	}

	void WriteZeroes( U32 _numBits )
	{
		this->WriteBits( 0, _numBits );
	}
};
#endif

template< typename TYPE >
static mxFORCEINLINE
bool GetBit( TYPE _x, U32 _index )
{
	mxASSERT( _index < sizeof(TYPE) * CHAR_BIT );
	return !!( _x & ( 1u << _index ) );
}

/// A very simple and easy to understand bit writer.
class BitWriter_Simple
{
	U8	m_BitBuffer;	//!< The next bits to be written to output stream; flushed to output when filled
	U32	m_BitsFilled;	//!< The number of valid, lower bits already filled in the bit buffer and pending output
	U8 *	m_start;
	U8 *	m_put;

public:
	BitWriter_Simple( U8 *_output )
		: m_start( _output ), m_put( _output )
	{
		m_BitBuffer = 0;
		m_BitsFilled = 0;
	}

	void Write_U32( const U32 _newValue )
	{
		mxASSERT(m_BitsFilled == 0);
		*(U32*)m_put = _newValue;
		m_put += sizeof(U32);
	}

	void WriteBit( bool _value, U32 _numBits = 1 )
	{
		for( U32 i = 0; i < _numBits; i++ )
		{
			m_BitBuffer |= ( _value << m_BitsFilled );
			m_BitsFilled++;

			if( m_BitsFilled == 8 )
			{
				*m_put++ = m_BitBuffer;
				m_BitsFilled = 0;
				m_BitBuffer = 0;
			}
		}
	}

	/// Writes N lower bits of X
	void WriteBits( U32 _value, U32 _numBits )
	{
		mxASSERT(_numBits <= 32);
		for( UINT i = 0; i < _numBits; i++ )
		{
			WriteBit( GetBit( _value, i ) );
		}
	}

	void WriteZeroes( U32 _numBits )
	{
		this->WriteBits( 0, _numBits );
	}

	/// Finish working, flushing any pending data.
	U32 Finish()
	{
		this->Flush();
		return m_put - m_start;
	}

	void Flush()
	{
		if( m_BitsFilled )
		{
			*m_put++ = m_BitBuffer;
			m_BitsFilled = 0;
			m_BitBuffer = 0;
		}
	}
};


/// A very simple and easy to understand bit reader.
class BitReader_Simple
{
	U8	m_BitBuffer;	//!< The next bits to be written to output stream; flushed to output when filled
	U32	m_BitsRead;	//!< The number of valid, lower bits already filled in the bit buffer and pending output
	const U8 *	m_get;
	//const U8 *	m_end;

public:
	BitReader_Simple( const U8* input )
		: m_get( input )//, m_end( _output )
	{
		m_BitBuffer = *m_get++;
		m_BitsRead = 0;
	}
	bool ReadBit()
	{
		if( m_BitsRead == 8 )
		{
			m_BitBuffer = *m_get++;
			m_BitsRead = 0;
		}
		return m_BitBuffer & (1u << m_BitsRead++);
	}
	U32 ReadBits( UINT _numBits )
	{
		mxASSERT(_numBits <= 32);
		U32	result = 0;
		for( UINT i = 0; i < _numBits; i++ )
		{
			result |= ( ReadBit() << i );
		}
		return result;
	}
};



template< class BIT_WRITER >
static inline
void WriteUnsigned32_Unary( BIT_WRITER & _writer, const U32 _value )
{
	// N in unary: N zeros and 1
	_writer.WriteZeroes( _value );
	_writer.WriteBits( 1, 1 );
}

/// [ Golomb-Rice | Rice | Golomb-power-of-2 (GPO2) ] codes
/// can be used to efficiently and losslessly encode sequences of nonnegative integers.
/// A Golomb-Rice code is a Golomb code where the divisor is a power of two,
/// enabling an efficient implementation using shifts and masks rather than division and modulo.
/// Rice coding depends on a parameter k and works the same as Golomb coding with a parameter m where m = 2^k.
/// @param 'k' - the divisor
template< class BIT_WRITER >
static inline
void WriteU32_Rice( BIT_WRITER & _writer, const U32 _x, const U32 _k )
{
	// Golomb encoding splits each value in two parts:
	// the quotient and the remainder of the division by the parameter.
	const U32 M = 1u << _k;	// M = 2^k
	const U32 q = _x / M;	// quotient (round fractions down)
	//const U32 r = _x - q * _k;	// remainder
	const U32 r = _x & ( M - 1 );	// since M is power of two, all remainders are coded in exactly log2(M) bits

	// We expect the quotient to be likely 0 or 1, less likely to be 2 and so on.
	// Smaller numbers are as compact as possible, higher and unlikely numbers get longer and longer.

	// Quotient is encoded in unary.
	// In canonical, 'pedagogical' codes, unary numbers are prefixed with ones...
	_writer.WriteBits( 0, q );	// ... but we write zeroes followed by a one (it's more efficient - clz/bsr);
	// if the quotient is written as a sequence of zeroes, the stop bit is a one
	_writer.WriteBits( 1, 1 );	// write the unary end bit.

	// Remainder is encoded in truncated binary notation.
	_writer.WriteBits( _x, _k );	// output k least significant bits of x using in binary
}

/// A unary code for a non negative integer N is N zeroes followed by a one (or N ones followed by a zero).
template< class BIT_READER >
static inline
U32 ReadU32_Unary( BIT_READER & _reader )
{
	// slow but readable version
	U32	result = 0;
	for(;;) {
		const bool bit = _reader.ReadBit();
		if( bit ) {
			break;
		}
		result++;
	}
	return result;
}

template< class BIT_READER >
static inline
U32 ReadU32_Rice( BIT_READER & _reader, const U32 _k )
{
	U32	q = ReadU32_Unary( _reader );

	const U32 r = _reader.ReadBits( _k );

	const U32 M = 1u << _k;	// M = 2^k

	return q * M + r;
}
