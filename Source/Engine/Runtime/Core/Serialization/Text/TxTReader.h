// SON (Simple Object Notation) parser
#pragma once

#include <Core/Serialization/Text/TxTCommon.h>

namespace SON
{

#if MX_DEBUG
	typedef unsigned char CharType;	// view character codes in debugger
#else
	typedef U32 CharType;		// register-sized integers are faster
#endif

// Use 32-bit integers as array indices/child counters.
//typedef U32 SizeType;

// Maximum level of nested structures (recursion depth).
//enum { PARSER_STACK_SIZE = 32 };

	// parser state
	struct Parser
	{
		char *	buffer;
		U32	length;

		U32	position;	// current position in the source file
		U32	line;		// current line (default value == 1)
		U32	column;		// current column (default == 0)

		const char*	file;	// file name for debugging

		INT32	errorCode;	// 0 == no error

	public:
		Parser();
		~Parser();
	};

	Node* ParseBuffer(
		Parser& _parser,
		NodeAllocator & nodeAllocator
	);

}//namespace SON
