//
#include <Core/Core_PCH.h>
#pragma hdrstop
#include <bx/uint32_t.h>
#include <Base/Memory/MemoryBase.h>
#include <Base/Template/Containers/Array/DynamicArray.h>
#include <Base/Util/FourCC.h>
#include <Core/Assets/AssetReference.h>
#include <Core/Assets/AssetManagement.h>
#include <Core/Serialization/Serialization.h>
#include <Core/Util/ScopedTimer.h>
#include <Core/Serialization/Serialization_Private.h>


namespace Serialization
{
	//
	// Binary serialization
	//

	ERet SaveBinary( const void* o, const TbMetaClass& type, AWriter &stream )
	{
		NwBinarySaveFileHeader	header;
		mxZERO_OUT(header);
		{
			header.filetype = SAVE_FILE_BINARY;
			header.session = TbSession::CURRENT;
			header.classId = type.GetTypeGUID();
		}
		mxDO(stream.Put(header));

		//

		class BinarySerializer: public Reflection::Visitor3< AWriter& > {
		public:
			BinarySerializer()
			{}
			virtual ERet Visit_Field( void * _memory, const MetaField& _field, const Context3& _context, AWriter &stream )
			{
#if DEBUG_SERIALIZATION
				//String256 tmp;
				//Str::Format(tmp,"[FIELD: %s (%s)]",_field.name,_field.type.GetTypeName());
				//mxDO(tmp.SaveToStream(stream));
#endif
				mxDO(this->Visit( _memory, _field.type, _context, stream ));
				return ALL_OK;
			}
			virtual ERet Visit_Array( void * _array, const MetaArray& type, const Context3& _context, AWriter &stream ) override
			{
#if DEBUG_SERIALIZATION
				//String256 tmp;
				//Str::Format(tmp,"[ARRAY: %s (%u items, %u bytes)]",type.GetTypeName(),type.Generic_Get_Count( _array ),type.CalculateDataSize(_array));
				//mxDO(tmp.SaveToStream(stream));
#endif

				const U32 arrayCount = type.Generic_Get_Count( _array );
				mxDO(stream.Put( arrayCount ));

				const TbMetaType& itemType = type.m_itemType;
				const U32 itemStride = itemType.m_size;
				void* arrayBase = type.Generic_Get_Data( _array );

				if( Type_Is_Bitwise_Serializable( itemType.m_kind ) ) {
					mxDO(stream.Write( arrayBase, arrayCount * itemStride ));
				} else {
					mxDO(Reflection::Visit_Array_Elements< AWriter& >( arrayBase, arrayCount, itemType, _context, *this, stream ));
				}

				return ALL_OK;
			}
			virtual ERet Visit_POD( void * _memory, const TbMetaType& type, const Context3& _context, AWriter &stream ) override
			{
				mxDO(stream.Write( _memory, type.m_size ));
				return ALL_OK;
			}
			virtual ERet Visit_String( String * _string, const Context3& _context, AWriter &stream ) override
			{
				mxDO(_string->SaveToStream( stream ));
				return ALL_OK;
			}
			virtual ERet Visit_TypeId( SClassId * _pointer, const Context3& _context, AWriter &stream ) override
			{
				const TypeGUID typeId = ( _pointer->type != nil ) ? _pointer->type->GetTypeGUID() : mxNULL_TYPE_ID;
				mxDO(stream.Put( typeId ));
				return ALL_OK;
			}
			virtual ERet Visit_AssetId( AssetID * _assetId, const Context3& _context, AWriter &stream ) override
			{
				mxDO(writeAssetID( *_assetId, stream ));
				return ALL_OK;
			}
			virtual ERet Visit_Pointer( void * _pointer, const MetaPointer& type, const Context3& _context, AWriter &stream ) override
			{
				return ERR_NOT_IMPLEMENTED;
			}
		};

		BinarySerializer	serializer;
		mxDO(serializer.Visit( const_cast< void* >(o), type, stream ));

		return ALL_OK;
	}

	ERet LoadBinary( void *o, const TbMetaClass& type, AReader& stream_reader )
	{
		NwBinarySaveFileHeader	header;
		mxDO(stream_reader.Get(header));

		mxDO(ValidatePlatformAndType(header, SAVE_FILE_BINARY, type));

		// Read object data and allocate memory for everything

		class BinaryDeserializer: public Reflection::Visitor3< AReader& > {
		public:
#if DEBUG_SERIALIZATION
			virtual void DebugPrint( const char* _format, ... )
			{
				va_list	args;
				va_start( args, _format );
				mxGetLog().PrintV( LL_Debug, _format, args );
				va_end( args );
			}
			virtual void DebugPrint( const Context3& _context, const char* _format, ... )
			{
				va_list	args;
				va_start( args, _format );
				LogStream(LL_Debug).Repeat('\t', _context.depth).PrintV(_format, args);
				va_end( args );
			}
#endif
			virtual ERet Visit_Field( void * _memory, const MetaField& _field, const Context3& _context, AReader& stream )
			{
#if DEBUG_SERIALIZATION
				//String256 tmp;
				//mxDO(tmp.LoadFromStream(stream));
#endif
				mxDO(this->Visit( _memory, _field.type, _context, stream ));
				return ALL_OK;
			}
			virtual ERet Visit_Array( void * _array, const MetaArray& type, const Context3& _context, AReader& stream ) override
			{
#if DEBUG_SERIALIZATION
				//String256 tmp;
				//mxDO(tmp.LoadFromStream(stream));
#endif
				U32 arrayCount = 0;
				mxDO(stream.Get( arrayCount ));
				mxASSERT( arrayCount < DBG_MAX_ARRAY_CAPACITY );

				mxDO(type.Generic_Set_Count( _array, arrayCount ));

				void* arrayBase = type.Generic_Get_Data( _array );

				const TbMetaType& itemType = type.m_itemType;
				const U32 itemStride = itemType.m_size;
				const UINT arrayDataSize = arrayCount * itemStride;

				if( arrayCount )
				{
					if( Type_Is_Bitwise_Serializable( itemType.m_kind ) ) {
						mxDO(stream.Read( arrayBase, arrayDataSize ));
					} else {
						mxDO(Reflection::Visit_Array_Elements< AReader& >( arrayBase, arrayCount, itemType, _context, *this, stream ));
					}
				}

				return ALL_OK;
			}
			virtual ERet Visit_POD( void * _memory, const TbMetaType& type, const Context3& _context, AReader &stream ) override
			{
				mxDO(stream.Read( _memory, type.m_size ));
				return ALL_OK;
			}
			virtual ERet Visit_String( String * _string, const Context3& _context, AReader &stream ) override
			{
				mxDO(_string->LoadFromStream( stream ));
				return ALL_OK;
			}
			virtual ERet Visit_TypeId( SClassId * _pointer, const Context3& _context, AReader &stream ) override
			{
				TypeGUID typeId;
				mxDO(stream.Get( typeId ));
				if( typeId != mxNULL_TYPE_ID ) {
					_pointer->type = TypeRegistry::FindClassByGuid( typeId );
					if( !_pointer->type ) {
						DebugPrint(_context,"No class with id=%d",typeId);
						return ERR_OBJECT_NOT_FOUND;
					}
				}
				return ALL_OK;
			}
			virtual ERet Visit_AssetId( AssetID * _assetId, const Context3& _context, AReader &stream ) override
			{
				mxDO(readAssetID( *_assetId, stream ));
				return ALL_OK;
			}
			virtual ERet Visit_Pointer( void * _pointer, const MetaPointer& type, const Context3& _context, AReader &stream ) override
			{
				return ERR_NOT_IMPLEMENTED;
			}
		};
		BinaryDeserializer	deserializer;
		mxDO(deserializer.Visit( const_cast< void* >(o), type, stream_reader ));

		return ALL_OK;
	}

	ERet SaveBinaryToFile( const void* o, const TbMetaClass& type, const char* file )
	{
		FileWriter	stream( file, FileWrite_NoErrors );
		if( !stream.IsOpen() ) {
			return ERR_FAILED_TO_OPEN_FILE;
		}
		return SaveBinary(o, type, stream);
	}
	ERet LoadBinaryFromFile( void *o, const TbMetaClass& type, const char* file )
	{
		FileReader	reader(file, FileRead_NoErrors);
		if( !reader.IsOpen() ) {
			return ERR_FAILED_TO_OPEN_FILE;
		}
		return LoadBinary( o, type, reader );
	}

}//namespace Serialization
