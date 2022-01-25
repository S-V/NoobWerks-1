#include <Base/Base.h>
#pragma hdrstop
#include <bx/uint32_t.h>
#include <Base/Template/Containers/Array/DynamicArray.h>
#include <Utility/Compression/Integer_Compression.h>


mxSTOLEN("simple9, Copyright (c) 2014-2015, Conor Stokes");
/// simple9, a word-aligned integer compression algorithm as described in
/// Vo Ngoc Anh and Alistair Moffat. Inverted index compression using
/// word-aligned binary codes. Information Retrieval, 8(1):151–166, 2005.
namespace Simple9
{

/// 28 has 9 divisors: 1, 2, 3, 4, 5, 7, 9, 14, and itself.
#define SIMPLE9_NUM_SELECTORS 9

/// Simple-9 packs several small integers into 28 bits of a 32-bit word,
/// leaving the remaining four bits as a selector.
/// If the next 28 integers to be compressed all have values 1 or 2,
/// then each can fit in one bit, making it possible to pack 28 integers in 28 bits.
/// The choice of 28 is particularly fortuitous,
/// because 28 is divisible by 1, 2, 3, 4, 5, 7, 9, 14, and itself.
struct Selector {
    U32 nMaxItems;
    U32 nBitsPerItem;	//!< number of bits for each item
    //U32 nWastedBits;	//!< number of wasted bits (3 at max)
};
static const Selector selectors[ SIMPLE9_NUM_SELECTORS ] = {
    {28,  1/*, 0*/},
    {14,  2/*, 0*/},
    { 9,  3/*, 1*/},
    { 7,  4/*, 0*/},
    { 5,  5/*, 3*/},
    { 4,  7/*, 0*/},
    { 3,  9/*, 1*/},
    { 2, 14/*, 0*/},
    { 1, 28/*, 0*/},
};

size_t simple9_encode(U32 *_items, size_t _nItems, FILE *fp)
{
	mxASSERT(_items);
	mxASSERT(_nItems > 0);
	mxASSERT(fp);

	size_t nBytesWritten = 0;

	//    nbytes = vbyte_encode(n, fp);
	U32 nItemsRead = 0;

	while (nItemsRead < _nItems)
	{
		// Try each selector to pack the maximum number of integers.
		for( U32 iSelector = 0; iSelector < SIMPLE9_NUM_SELECTORS; iSelector++ )
		{
			const Selector& selector = selectors[ iSelector ];
			const U32 maxItemValue = (1UL << selector.nBitsPerItem) - 1;

			U32 bitBuffer = iSelector;	// the first 4 bits of each int store the selector
			U32 shift = SIMPLE9_SELECTOR_BITS;
			U32 nPackedItems = 0;	// number of items currently stored in the bit buffer

			// Try to pack as many items as possible using the current selector.
			for( UINT i = nItemsRead; i < _nItems; i++ )
			{
				const U32 currentItem = _items[ i ];
				mxASSERT(currentItem <= SIMPLE9_MAX_VALUE);
mxBUG("Infinite loop when saving octrees with resolution >= 1024");
				if( nPackedItems == selector.nMaxItems ) {
					goto L_Selector_Found;	// all items have been 'maximally' packed into the current uint
				}
				if( currentItem > maxItemValue ) {
					goto L_Continue;	// need to try another selector which can store bigger values (up to 28 bits)
				}

				bitBuffer |= (currentItem << shift);
				shift += selector.nBitsPerItem;
				nPackedItems++;
			}

			if( nPackedItems == selector.nMaxItems || nItemsRead + nPackedItems == _nItems )
			{
L_Selector_Found:
				fwrite(&bitBuffer, sizeof bitBuffer, 1, fp);

				nBytesWritten += sizeof bitBuffer;

				nItemsRead += nPackedItems;

				break;
			}

L_Continue:
			;

		} /* End for selector ... */

	} /* End while index < n */

	return nBytesWritten;
}

/// Finish working, flushing any pending data.
void StreamWriter::Finish()
{
	if( m_bufferedItems )
	{
		this->TryFlush();
	}
	mxASSERT(m_bufferedItems == 0);
}

void StreamWriter::TryFlush()
{
	const U32 nItemsTotal = m_bufferedItems;
	mxASSERT( nItemsTotal );

	U32 nItemsRead = 0;

	while( nItemsRead < nItemsTotal )
	{
		// Try each selector to pack the maximum number of integers.
		for( U32 iSelector = 0; iSelector < SIMPLE9_NUM_SELECTORS; iSelector++ )
		{
			const Selector& selector = selectors[ iSelector ];
			const U32 maxItemValue = (1UL << selector.nBitsPerItem) - 1;

			U32 bitBuffer = iSelector;	// the first 4 bits of each int store the selector
			U32 shift = SIMPLE9_SELECTOR_BITS;
			U32 nPackedItems = 0;	// number of items currently stored in the bit buffer

			// Try to pack as many items as possible using the current selector.
			for( UINT i = nItemsRead; i < nItemsTotal; i++ )
			{
				const U32 currentItem = m_buffer[ i ];

				if( nPackedItems == selector.nMaxItems ) {
					goto L_Selector_Found;	// all items have been 'maximally' packed into the current uint
				}
				if( currentItem > maxItemValue ) {
					goto L_Continue;	// need to try another selector which can store bigger values (up to 28 bits)
				}

				bitBuffer |= (currentItem << shift);
				shift += selector.nBitsPerItem;
				nPackedItems++;
			}

			if( nPackedItems == selector.nMaxItems )
			{
L_Selector_Found:
				m_writer.Write( &bitBuffer, sizeof(bitBuffer) );
				nItemsRead += nPackedItems;
				break;
			}
L_Continue:
			;
		}//For each selector.
	}//For each item.

	m_bufferedItems = 0;
}

U32 StreamReader::Read_U32()
{
	if( m_bufferedItems )
	{
		const U32 stackTop = --m_bufferedItems;
		const U32 value = m_buffer[ stackTop ];
		return value;
	}
	else
	{
		const U32 newItem = *m_get++;

		const U32 selector = newItem & SIMPLE9_SELECTOR_MASK;
		const U32 payLoad = newItem >> SIMPLE9_SELECTOR_BITS;

		const U32 numItems = selectors[ selector ].nMaxItems;
		UINT	iLastItem = numItems;

		switch( selector )
		{
		case 0: /* 28 -- 1 bit elements */
			m_buffer[--iLastItem] = (payLoad) & 1;
			m_buffer[--iLastItem] = (payLoad >> 1) & 1;
			m_buffer[--iLastItem] = (payLoad >> 2) & 1;
			m_buffer[--iLastItem] = (payLoad >> 3) & 1;
			m_buffer[--iLastItem] = (payLoad >> 4) & 1;
			m_buffer[--iLastItem] = (payLoad >> 5) & 1;
			m_buffer[--iLastItem] = (payLoad >> 6) & 1;
			m_buffer[--iLastItem] = (payLoad >> 7) & 1;
			m_buffer[--iLastItem] = (payLoad >> 8) & 1;
			m_buffer[--iLastItem] = (payLoad >> 9) & 1;
			m_buffer[--iLastItem] = (payLoad >> 10) & 1;
			m_buffer[--iLastItem] = (payLoad >> 11) & 1;
			m_buffer[--iLastItem] = (payLoad >> 12) & 1;
			m_buffer[--iLastItem] = (payLoad >> 13) & 1;
			m_buffer[--iLastItem] = (payLoad >> 14) & 1;
			m_buffer[--iLastItem] = (payLoad >> 15) & 1;
			m_buffer[--iLastItem] = (payLoad >> 16) & 1;
			m_buffer[--iLastItem] = (payLoad >> 17) & 1;
			m_buffer[--iLastItem] = (payLoad >> 18) & 1;
			m_buffer[--iLastItem] = (payLoad >> 19) & 1;
			m_buffer[--iLastItem] = (payLoad >> 20) & 1;
			m_buffer[--iLastItem] = (payLoad >> 21) & 1;
			m_buffer[--iLastItem] = (payLoad >> 22) & 1;
			m_buffer[--iLastItem] = (payLoad >> 23) & 1;
			m_buffer[--iLastItem] = (payLoad >> 24) & 1;
			m_buffer[--iLastItem] = (payLoad >> 25) & 1;
			m_buffer[--iLastItem] = (payLoad >> 26) & 1;
			m_buffer[--iLastItem] = (payLoad >> 27) & 1;
			break;

		case 1: /* 14 -- 2 bit elements */
			m_buffer[--iLastItem] = (payLoad) & 3;
			m_buffer[--iLastItem] = (payLoad >> 2) & 3;
			m_buffer[--iLastItem] = (payLoad >> 4) & 3;
			m_buffer[--iLastItem] = (payLoad >> 6) & 3;
			m_buffer[--iLastItem] = (payLoad >> 8) & 3;
			m_buffer[--iLastItem] = (payLoad >> 10) & 3;
			m_buffer[--iLastItem] = (payLoad >> 12) & 3;
			m_buffer[--iLastItem] = (payLoad >> 14) & 3;
			m_buffer[--iLastItem] = (payLoad >> 16) & 3;
			m_buffer[--iLastItem] = (payLoad >> 18) & 3;
			m_buffer[--iLastItem] = (payLoad >> 20) & 3;
			m_buffer[--iLastItem] = (payLoad >> 22) & 3;
			m_buffer[--iLastItem] = (payLoad >> 24) & 3;
			m_buffer[--iLastItem] = (payLoad >> 26) & 3;
			break;

		case 2: /* 9 -- 3 bit elements (1 wasted bit) */
			m_buffer[--iLastItem] = (payLoad) & 7;
			m_buffer[--iLastItem] = (payLoad >> 3) & 7;
			m_buffer[--iLastItem] = (payLoad >> 6) & 7;
			m_buffer[--iLastItem] = (payLoad >> 9) & 7;
			m_buffer[--iLastItem] = (payLoad >> 12) & 7;
			m_buffer[--iLastItem] = (payLoad >> 15) & 7;
			m_buffer[--iLastItem] = (payLoad >> 18) & 7;
			m_buffer[--iLastItem] = (payLoad >> 21) & 7;
			m_buffer[--iLastItem] = (payLoad >> 24) & 7;
			break;

		case 3: /* 7 -- 4 bit elements */
			m_buffer[--iLastItem] = (payLoad) & 15;
			m_buffer[--iLastItem] = (payLoad >> 4) & 15;
			m_buffer[--iLastItem] = (payLoad >> 8) & 15;
			m_buffer[--iLastItem] = (payLoad >> 12) & 15;
			m_buffer[--iLastItem] = (payLoad >> 16) & 15;
			m_buffer[--iLastItem] = (payLoad >> 20) & 15;
			m_buffer[--iLastItem] = (payLoad >> 24) & 15;
			break;

		case 4: /* 5 -- 5 bit elements (3 wasted bits) */
			m_buffer[--iLastItem] = (payLoad) & 31;
			m_buffer[--iLastItem] = (payLoad >> 5) & 31;
			m_buffer[--iLastItem] = (payLoad >> 10) & 31;
			m_buffer[--iLastItem] = (payLoad >> 15) & 31;
			m_buffer[--iLastItem] = (payLoad >> 20) & 31;
			break;

		case 5: /* 4 -- 7 bit elements */
			m_buffer[--iLastItem] = (payLoad) & 127;
			m_buffer[--iLastItem] = (payLoad >> 7) & 127;
			m_buffer[--iLastItem] = (payLoad >> 14) & 127;
			m_buffer[--iLastItem] = (payLoad >> 21) & 127;
			break;

		case 6: /* 3 -- 9 bit elements (1 wasted bit) */
			m_buffer[--iLastItem] = (payLoad) & 511;
			m_buffer[--iLastItem] = (payLoad >> 9) & 511;
			m_buffer[--iLastItem] = (payLoad >> 18) & 511;
			break;

		case 7: /* 2 -- 14 bit elements */
			m_buffer[--iLastItem] = (payLoad) & 16383;
			m_buffer[--iLastItem] = (payLoad >> 14) & 16383;
			break;

		case 8: /* 1 -- 28 bit element */
			m_buffer[--iLastItem] = payLoad;
			break;
		}

		mxASSERT(iLastItem == 0);

		const U32 value = m_buffer[ numItems - 1 ];
		m_bufferedItems = numItems - 1;
		return value;
	}
}

#if 0
size_t simple9_decode(U32 **array, size_t *n, FILE *fp)
{
	U32 data;
	U32 select;
	U32 mask;

	size_t nbytes = 0;
	size_t nitems;
	size_t i;

	mxASSERT(array);
	mxASSERT(n);
	mxASSERT(fp);

	//    nbytes = vbyte_decode(n, fp);

	/* Look up at the sky. So many stars. It's... beautiful. */
	*array = malloc(*n * sizeof **array);
	mxASSERT(*array);

	nitems = 0;

	while (nitems < *n) {
		fread(&data, sizeof data, 1, fp);

		nbytes += sizeof data;

		select = data & SIMPLE9_SELECTOR_MASK;

		data >>= SIMPLE9_SELECTOR_BITS;

		mask = (1 << selectors[select].nbits) - 1;

		for (i = 0; i < selectors[select].nitems; i++) {
			(*array)[nitems] = data & mask;

			nitems++;

			if (nitems == *n)
				break;

			data >>= selectors[select].nbits;
		}
	}

	return nbytes;
}
#endif
}//namespace Simple9




#if 0
// vsencNN: compress array with n unsigned (NN bits in[n]) values to the buffer out. Return value = end of compressed buffer out
unsigned char *vsenc32(unsigned           *__restrict in, int n, unsigned char      *__restrict out);

// vsdecNN: decompress buffer into an array of n unsigned values. Return value = end of decompressed buffer in
unsigned char *vsdec32(unsigned char      *__restrict in, int n, unsigned           *__restrict out);
#endif

#if 0

typedef UINT uint_t;

#define TEMPLATE2_(__x, __y) __x##__y
#define TEMPLATE2(__x, __y) TEMPLATE2_(__x,__y)

#define VSENC vsenc
#define VSDEC vsdec

#define __builtin_prefetch(X,Y)


unsigned char *TEMPLATE2(VSENC, USIZE)(uint_t *__restrict in, int n, unsigned char *__restrict op) {
	unsigned xm,m,r,x; 
	uint_t *e = in+n,*ip;
	for(ip = in; ip < e; ) { 										__builtin_prefetch(ip+64, 0);
#ifdef USE_RLE 
	if(ip+4 < e && *ip == *(ip+1)) { 
		uint_t *q = ip+1; 
		while(q+1 < e && *(q+1) == *ip) q++; 
		r = q - ip; 
		if(r*TEMPLATE2(bsr, USIZE)(*ip) > 16 || (!*ip && r>4)) { 
			m = (*ip)?(USIZE<=32?33:65):0; 
			goto a; 
		}
	} else 
#endif
		r = 0;
	for(m = x = TEMPLATE2(bsr, USIZE)(*ip);(r+1)*(xm = x > m?x:m) <= TEMPLATE2(s_lim, USIZE)[xm] && ip+r<e;) m = xm, x = TEMPLATE2(bsr, USIZE)(ip[++r]); 
	while(r < TEMPLATE2(s_itm, USIZE)[m]) m++;

a:; 
	switch(m) {
	  case 0: ip += r; 
		  if(--r >= 0xf) { 
			  *op++ = 0xf0; 
			  if(n <= 0x100)
				  *op++ = r; 
			  else
				  vbput32(op, r);            
		  } else *op++ = r<<4; 
		  break;
	  case 1:
		  *(unsigned *)op =    1 |
			  (unsigned)ip[ 0] <<  4 | 
			  (unsigned)ip[ 1] <<  5 |
			  (unsigned)ip[ 2] <<  6 |
			  (unsigned)ip[ 3] <<  7 |
			  (unsigned)ip[ 4] <<  8 |
			  (unsigned)ip[ 5] <<  9 |
			  (unsigned)ip[ 6] << 10 |
			  (unsigned)ip[ 7] << 11 |
			  (unsigned)ip[ 8] << 12 |
			  (unsigned)ip[ 9] << 13 |  
			  (unsigned)ip[10] << 14 |
			  (unsigned)ip[11] << 15 |
			  (unsigned)ip[12] << 16 |
			  (unsigned)ip[13] << 17 |
			  (unsigned)ip[14] << 18 |
			  (unsigned)ip[15] << 19 |
			  (unsigned)ip[16] << 20 |
			  (unsigned)ip[17] << 21 |
			  (unsigned)ip[18] << 22 |
			  (unsigned)ip[19] << 23 |
			  (unsigned)ip[20] << 24 |
			  (unsigned)ip[21] << 25 |
			  (unsigned)ip[22] << 26 |
			  (unsigned)ip[23] << 27 |
			  (unsigned)ip[24] << 28 |
			  (unsigned)ip[25] << 29 |
			  (unsigned)ip[26] << 30 |
			  (unsigned)ip[27] << 31;		ip += 28; op += 4;
		  break;
	  case 2: 
		  *(unsigned *)op =    2 |
			  (unsigned)ip[ 0] <<  4 | 
			  (unsigned)ip[ 1] <<  6 |
			  (unsigned)ip[ 2] <<  8 |
			  (unsigned)ip[ 3] << 10 |
			  (unsigned)ip[ 4] << 12 |
			  (unsigned)ip[ 5] << 14 |
			  (unsigned)ip[ 6] << 16 |
			  (unsigned)ip[ 7] << 18 |
			  (unsigned)ip[ 8] << 20 |
			  (unsigned)ip[ 9] << 22 |  
			  (unsigned)ip[10] << 24 |
			  (unsigned)ip[11] << 26 |
			  (unsigned)ip[12] << 28 |
			  (unsigned)ip[13] << 30;		ip += 14; op += 4;
		  break;
	  case 3: 
		  *(unsigned *)op =    3 |
			  (unsigned)ip[ 0] <<  4 | 
			  (unsigned)ip[ 1] <<  7 |
			  (unsigned)ip[ 2] << 10 |
			  (unsigned)ip[ 3] << 13 |
			  (unsigned)ip[ 4] << 16 |
			  (unsigned)ip[ 5] << 19 |
			  (unsigned)ip[ 6] << 22 |
			  (unsigned)ip[ 7] << 25 |
			  (unsigned)ip[ 8] << 28;		ip +=  9; op += 4;
		  break;
	  case 4: 
		  *(U64_t *)op =    4 |
			  (unsigned)ip[ 0] <<  4 | 
			  (unsigned)ip[ 1] <<  8 |
			  (unsigned)ip[ 2] << 12 |
			  (unsigned)ip[ 3] << 16 |
			  (unsigned)ip[ 4] << 20 |
			  (unsigned)ip[ 5] << 24 |
			  (unsigned)ip[ 6] << 28;		ip += 7; op += 4;
		  break;
	  case 5: 
		  *(U64_t *)op =    5 |
			  (unsigned)ip[ 0] <<  4 | 
			  (unsigned)ip[ 1] <<  9 |
			  (unsigned)ip[ 2] << 14 |
			  (unsigned)ip[ 3] << 19 |
			  (unsigned)ip[ 4] << 24 |
			  (U64_t)ip[ 5] << 29 |
			  (U64_t)ip[ 6] << 34;		ip += 7; op += 5;
		  break;
	  case 6: 
		  *(U64_t *)op =    6 |
			  (unsigned)ip[ 0] <<  4 | 
			  (unsigned)ip[ 1] << 10 |
			  (unsigned)ip[ 2] << 16 |
			  (unsigned)ip[ 3] << 22 |
			  (U64_t)ip[ 4] << 28 |
			  (U64_t)ip[ 5] << 34; 	ip +=  6; op += 5;
		  break;
	  case 7: 
		  *(U64_t *)op =    7 |
			  (unsigned)ip[ 0] <<  5 | 
			  (unsigned)ip[ 1] << 12 |
			  (unsigned)ip[ 2] << 19 |
			  (U64_t)ip[ 3] << 26 |
			  (U64_t)ip[ 4] << 33; 	ip += 5; op += 5;
		  break;
	  case  8: 
	  case  9: 
		  *(U64_t *)op =    9 |
			  (unsigned)ip[ 0] <<  4 | 
			  (unsigned)ip[ 1] << 13 |
			  (unsigned)ip[ 2] << 22 |
			  (U64_t)ip[ 3] << 31; 	ip += 4; op += 5;
		  break;
	  case 10:
		  *(U64_t *)op =   10 |
			  (unsigned)ip[ 0] <<  4 | 
			  (unsigned)ip[ 1] << 14 |
			  (U64_t)ip[ 2] << 24 |
			  (U64_t)ip[ 3] << 34 |
			  (U64_t)ip[ 4] << 44 |
			  (U64_t)ip[ 5] << 54; 	ip += 6; op += 8;
		  break;

	  case 11: 
	  case 12: 
		  *(U64_t *)op =   12 |
			  (unsigned)ip[ 0] <<  4 | 
			  (unsigned)ip[ 1] << 16 |
			  (U64_t)ip[ 2] << 28 |
			  (U64_t)ip[ 3] << 40 |
			  (U64_t)ip[ 4] << 52; 	ip += 5; op += 8;
		  break;      
	  case 13:
	  case 14:      
	  case 15:
		  *(U64_t *)op =   15 |
			  (unsigned)ip[ 0] <<  4 | 
			  (U64_t)ip[ 1] << 19 |
			  (U64_t)ip[ 2] << 34 |
			  (U64_t)ip[ 3] << 49; 	ip += 4; op += 8;
		  break;
	  case 16:
#if USIZE > 16
	  case 17:      
	  case 18:
	  case 19:      
	  case 20: 
#endif
		  *(U64_t *)op =   11 |
			  (unsigned)ip[ 0] <<  4 | 
			  (U64_t)ip[ 1] << 24 |
			  (U64_t)ip[ 2] << 44; 	ip += 3; op += 8;
		  break;
#if USIZE > 16
	  case 21:
	  case 22:      
	  case 23:
	  case 24:      
	  case 25:
	  case 26:      
	  case 27:
	  case 28:      
	  case 29:
	  case 30:
		  *(U64_t *)op =   13 |
			  (U64_t)ip[ 0] <<  4 | 
			  (U64_t)ip[ 1] << 34; 	ip += 2; op += 8;
		  break;
	  case 31:
	  case 32:
#if USIZE == 64
	  case 33: case 34: case 35: case 36:
#endif
		  *(U64_t *)op =   14 |
			  (U64_t)ip[ 0] <<  4; 	ip++;    op += 5;
		  break;
#if USIZE == 64
	  case 37 ... 64: xm = (m+7)/8;        
		  *op++ =   0x17 | (xm-1) << 5;
		  *(U64_t *)op = (U64_t)ip[ 0]; ip++;   op += xm; 
		  break;
#endif
#endif
#ifdef USE_RLE 
	  case USIZE<=32?33:65: 		ip += r;
		  if(--r >= 0xf) { 
			  *op++ = 0xf0|8; 
			  if(n <= 0x100)
				  *op++ = r; 
			  else
				  vbput32(op, r);
		  } else *op++ = r<<4|8;
		  TEMPLATE2(vbput, USIZE)(op, ip[0]);
		  break;
#endif

	}    
	} 
  return op;   
}
#endif

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
