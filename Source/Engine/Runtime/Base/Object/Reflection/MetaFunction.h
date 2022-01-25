#pragma once

#include <Base/Object/Reflection/TypeDescriptor.h>
#include <Base/Object/Reflection/Reflection.h>

#if 0
// free function signature
struct FunctionSignature
{
	const TbMetaType* returnType;
	const TbMetaType** argumentTypes;
	const int argumentsCount;
};

//struct MethodSignature
//{
//
//};

//#define BEGIN_METHOD_TABLE
//#define END_METHOD_TABLE
//
//#define REFLECT_METHOD


// returns 'R' and accepts 'void'
template< typename R >
inline FunctionSignature GetFunctionSignature( R (*)() )
{
	FunctionSignature signature = {
		&T_DeduceTypeInfo< R >(),
		NULL,
		0
	};
	return signature;
}

template< typename R, typename P1 >
static FunctionSignature GetFunctionSignature( R (*)(P1) )
{
	static const TbMetaType* argumentTypes[] = {
		&T_DeduceTypeInfo< P1 >(),
	};
	FunctionSignature signature = {
		&T_DeduceTypeInfo< R >(),
		argumentTypes,
		mxCOUNT_OF(argumentTypes)
	};
	return signature;
}

template< typename R, typename P1, typename P2 >
static FunctionSignature GetFunctionSignature( R (*)(P1,P2) )
{
	static const TbMetaType* argumentTypes[] = {
		&T_DeduceTypeInfo< P1 >(),
		&T_DeduceTypeInfo< P2 >(),
	};
	FunctionSignature signature = {
		&T_DeduceTypeInfo< R >(),
		argumentTypes,
		mxCOUNT_OF(argumentTypes)
	};
	return signature;
}
#endif

#define mxBEGIN_METHOD_TABLE( CLASS )

//----------------------------------------------------------//
//				End Of File.									//
//----------------------------------------------------------//
