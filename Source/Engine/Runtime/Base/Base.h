/*
=============================================================================
	File:	Base.h
	Desc:	Foundation Library public header file.
=============================================================================
*/
#pragma once

#include <GlobalEngineConfig.h>

//-----------------------------------------------------------
//	Public header files.
//-----------------------------------------------------------

#include "Build/BuildConfig.h"		// Build configuration options, compile settings.
#include <Base/CPP/ForEach.h>
#include <Base/System/ptPlatform.h>		// Platform-specific stuff.
#include <Base/System/ptMacros.h>

#include <Base/Debug/Assert.h>
#include <Base/Debug/ptDebug.h>
#include <Base/Debug/DbgNamedObject.h>
#include <Base/Debug/PrintSizeOf.h>

#include <Base/System/PtBaseTypes.h>
#include <Base/Types/TypeTraits.h>
#include <Base/Types/Troolean.h>
#include <Base/Types/Handle.h>
#include <Base/System/ptProfiler.h>

// Forward declarations of commonly used types and functions.

class String;

class ALog;

class URI;
class mxDataStream;
class AWriter;
class AReader;

template<
	typename TYPE,
	class DERIVED
> class TArrayBase;

template<
	class T
> class TArray;

template<
	typename TYPE,
	const UINT SIZE
> class TStaticArray;

template<
	typename TYPE,
	const UINT SIZE
> class TFixedArray;

template<
	typename KEY,
	typename VALUE,
	class HASH_FUNC,
	class EQUALS_FUNC
> class THashMap;

template<
	typename VALUE,
	class HASH_FUNC
> class TPointerMap;

// High-level concepts.
class AObject;

// Platform-independent application entry point.
extern int mxAppMain();

//------ Common ----------------------------------------------------

// dependencies
#include "IO/StreamIO.h"

//------ Memory management ------------------------------------------------

#include "Memory/MemoryBase.h"
#include "Memory/Allocator.h"
//#include "Memory/BlockAlloc/BlockAllocator.h"
//#include "Memory/BlockAlloc/DynamicBlockAlloc.h"

//------ Templates ------------------------------------------------

// Common stuff.
#include <Base/Template/Functions.h>
#include <Base/Template/TBits.h>
#include <Base/Template/TBool.h>
#include <Base/Template/TEnum.h>
#include <Base/Template/Singleton.h>

// Smart pointers.
#include "Template/SmartPtr/TPtr.h"
#include "Template/SmartPtr/TAutoPtr.h"
//#include "Template/SmartPtr/TSharedPtr.h"
#include "Template/SmartPtr/TSmartPtr.h"
#include "Template/SmartPtr/TReference.h"
#include "Template/SmartPtr/ReferenceCounted.h"

// Arrays.
#include <Base/Template/Containers/Array/ArraysCommon.h>
#include "Template/Containers/Array/TStaticArray.h"
#include "Template/Containers/Array/TFixedArray.h"
#include "Template/Containers/Array/DynamicArray.h"
#include "Template/Containers/Array/TArray.h"
#include "Template/Containers/Array/TBuffer.h"

// Lists.
#include <Base/Template/Containers/LinkedList/TLinkedList.h>
#include <Base/Template/Containers/LinkedList/TDoublyLinkedList.h>
#include "Template/Containers/HashMap/TKeyValue.h"

//------ String ----------------------------------------------------

#include "Text/StringTools.h"
#include "Text/String.h"		// String type.
//#include "Text/TextUtils.h"
//#include "Text/Token.h"
//#include "Text/Lexer.h"
//#include "Text/Parser.h"

//------ Math ------------------------------------------------------
// low-level math
#include <Base/Math/Base/Intrinsics.h>
#include <Base/Math/Base/Integer.h>
#include <Base/Math/Base/Float16.h>
#include <Base/Math/Base/Float32.h>
#include <Base/Math/MiniMath.h>
#include <Base/Math/Axes.h>
#include <Base/Math/Interpolate.h>
#include <Base/Math/Plane.h>
#include <Base/Math/Matrix/M33f.h>
#include <Base/Math/Matrix/M44f.h>
#include <Base/Math/Matrix/PackedMatrix.h>
#include <Base/Math/Quaternion.h>
#include <Base/Math/BoundingVolumes/Sphere.h>
#include <Base/Math/BoundingVolumes/AABB.h>
#include <Base/Math/BoundingVolumes/Cube.h>
// Hashing
#include "Math/Hashing/HashFunctions.h"
//#include "Math/Hashing/CRC8.h"
//#include "Math/Hashing/CRC16.h"
//#include "Math/Hashing/CRC32.h"
//#include "Math/Hashing/Honeyman.h"
//#include "Math/Hashing/MD4.h"
//#include "Math/Hashing/MD5.h"

// Common
//#include "Math/Math.h"

// Input/Output system.
#include "IO/StreamIO.h"
#include "IO/FileIO.h"
//#include "IO/IOServer.h"
//#include "IO/URI.h"
//#include "IO/DataStream.h"
//#include "IO/Archive.h"
//#include "IO/FileStream.h"
//#include "IO/MemoryStream.h"
#include "IO/Log.h"

//#include "Template/Containers/HashMap/TStringMap.h"
//#include "Template/Containers/HashMap/BTree.h"
//#include "Template/Containers/HashMap/RBTreeMap.h"
//#include "Template/Containers/HashMap/THashMap.h"
//#include "Template/Containers/HashMap/TKeyValue.h"
//#include "Template/Containers/HashMap/Dictionary.h"
//#include "Template/Containers/HashMap/TPointerMap.h"
//#include "Template/Containers/HashMap/StringMap.h"

//#include "Template/Containers/BitSet/BitField.h"
//#include "Template/Containers/BitSet/BitArray.h"

//------ Object system ------------------------------------------------------

//#include "Object/Object.h"			// Base class. NOTE: must be included after 'TypeDescriptor.h' !
#include "Object/Reflection/TypeDescriptor.h"			// Run-Time Type Information.
#include "Object/Reflection/Reflection.h"
#include "Object/Reflection/StructDescriptor.h"
#include "Object/Reflection/ClassDescriptor.h"
// for reflecting array types
#include "Object/Reflection/ArrayDescriptor.h"
#include "Object/Reflection/PointerType.h"
#include "Object/Reflection/EnumType.h"
#include "Object/Reflection/FlagsType.h"
#include "Object/Reflection/UserPointerType.h"
#include "Object/Reflection/BinaryBlobType.h"
#include <Base/Object/Reflection/MetaDictionary.h>
#include <Base/Object/Reflection/MetaAssetReference.h>
#include "Object/ManualSerialization.h"
//#include "Object/Message.h"			// Messaging.

//------ Miscellaneous Utilities --------------------------------------------

//#include "Util/Sorting.h"
//#include "Util/Rectangle.h"
//#include "Util/FourCC.h"
//#include "Util/Color.h"
//#include "Util/Misc.h"
#include "Util/Version.h"

//-----------------------------------------------------------
//	Don't forget to call these functions before/after
//	using anything from the base system!
//-----------------------------------------------------------

// must be called on startup by each system dependent on the base system (increments internal ref counter)
ERet	mxInitializeBase();

// must be called on shutdown by each system dependent on the base system (decrements internal ref counter);
// returns 'true' if the system has really been shut down
bool	mxShutdownBase();

bool	mxBaseSystemIsInitialized();

// unsafe, terminates the app immediately
void	mxForceExit( int exitCode );

// sets the function to be called before closing the base subsystem.
// can be used to clean up resources after a fatal error has occurred
void	mxSetExitHandler( FCallback* pFunc, void* pArg );

void	mxGetExitHandler( FCallback **pFunc, void **pArg );


class DependsOn_Base: NonCopyable {
protected:
	DependsOn_Base() { mxInitializeBase(); }
	~DependsOn_Base() { mxShutdownBase(); }
};

struct NwSetupMemorySystem: DependsOn_Base
{
	NwSetupMemorySystem();
	~NwSetupMemorySystem();
};
