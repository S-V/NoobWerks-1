#pragma once


/// Dummy declaration to determine the size of a class at compile-time.
/// The size of the class will be reported in a linking error message.
/// Taken from http://stackoverflow.com/a/2008577.
template< int SIZE_OF > struct TIncompleteType;

/// prints the size of the instance in the console
#define tbPRINT_SIZE_OF( TYPENAME )		TIncompleteType<sizeof(TYPENAME)>	PP_JOIN(SIZE_OF_, __LINE__)
