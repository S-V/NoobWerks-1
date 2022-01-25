
#ifndef __CRC32_H__
#define __CRC32_H__
mxSTOLEN("idSoftware, Doom3/Prey SDK");

/*
===============================================================================

	Calculates a checksum for a block of data
	using the CRC-32.

===============================================================================
*/

void CRC32_InitChecksum( UINT32 &crcvalue );
void CRC32_UpdateChecksum( UINT32 &crcvalue, const void *data, size_t length );
void CRC32_FinishChecksum( UINT32 &crcvalue );
UINT32 CRC32_BlockChecksum( const void *data, size_t length );



#endif /* !__CRC32_H__ */
