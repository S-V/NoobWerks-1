/*
=============================================================================
	File:	Sorting.cpp
	Desc:
=============================================================================
*/

#include <Base/Base_PCH.h>
#pragma hdrstop
#include <Base/Base.h>

#include "Sorting.h"



void QSort( void* pBase, INT count, INT width, QSortComparisonFunction pCompareFunc )
{
	::qsort( pBase, count, width, pCompareFunc );
}



//----------------------------------------------------------//
//				End Of File.									//
//----------------------------------------------------------//
