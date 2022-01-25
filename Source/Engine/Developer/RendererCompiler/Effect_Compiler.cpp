#include <Base/Base.h>
#pragma hdrstop
//#include <Base/Text/StringTools.h>
#include <Core/Text/Lexer.h>
#include <Core/Serialization/Serialization.h>
#include <Graphics/Public/graphics_formats.h>
#include <Core/Serialization/Text/TxTSerializers.h>
#include "Effect_Compiler.h"


mxDEFINE_CLASS(FxDefine);
mxBEGIN_REFLECTION(FxDefine)
	mxMEMBER_FIELD(name),
	mxMEMBER_FIELD(value),
mxEND_REFLECTION;


FxOptions::FxOptions()
{
	include = nil;

	optimize_shaders = true;
	strip_debug_info = true;

	disassembly_path = nil;
}

#if 0

mxBEGIN_REFLECT_ENUM( ShaderTargetT )
	mxREFLECT_ENUM_ITEM( PC_Direct3D_11, EShaderTarget::PC_Direct3D_11 ),
	mxREFLECT_ENUM_ITEM( PC_OpenGL_4plus, EShaderTarget::PC_OpenGL_4plus ),
mxEND_REFLECT_ENUM

ERet FxCompileEffectFromFile(const char* filename,
							 ByteArrayT &effectBlob,
							 const FxOptions& options)
{
	Clump			clump;
	ShaderCode_d	cache;

	mxDO(CompileShaderLibrary( filename, options, clump, cache ));

	effectBlob.empty();
	ByteArrayWriter	writer(effectBlob);

	FxLibraryHeader_d	header;
	mxZERO_OUT(header);
	header.magicNumber = '41XF';

	mxDO(writer.Put(header));

	// We compile the source asset file into two parts:
	// 1) the memory-resident part (a clump containing effect library structures);
	// 2) the temporary data that is used only for initializing run-time structures and exists only during loading (compiled shader code);

	// save the memory-resident part to enable initial loading and hot-reloading
#if mxUSE_BINARY_EFFECT_FILE_FORMAT
	//mxTRY(Serialization::SaveBinary(&O, mxCLASS_OF(O), writer));
	mxTRY(Serialization::SaveClumpImage( clump, writer ));
#else
	mxTRY(SON::SaveClump( clump, writer ));
#endif

	{
		U32 currentOffset = effectBlob.rawSize();
		U32 alignedOffset = AlignUp( currentOffset, 4 );
		U32 paddingAmount = alignedOffset - currentOffset;
		if( paddingAmount ) {
			U32 padding = 0;
			mxDO(writer.Write( &padding, paddingAmount ));
		}
	}

	{
		header.runtimeSize = effectBlob.rawSize() - sizeof(FxLibraryHeader_d);
		((FxLibraryHeader_d*)effectBlob.raw())->runtimeSize = header.runtimeSize;
	}

	// save compiled shader bytecode
	mxTRY(cache.SaveToStream(writer));

	//DBGOUT("Saving shaders: %u VS, %u GS, %u PS\n",
	//	m_shaders[ShaderType::Vertex].num(),
	//	m_shaders[ShaderType::Geometry].num(),
	//	m_shaders[ShaderType::Pixel].num());

	return ALL_OK;
}

ERet FxCompileAndSaveEffect(const char* sourceFile,
							const char* destination,
							const FxOptions& options)
{
	ByteArrayT	effectBlob;
	mxTRY(FxCompileEffectFromFile(sourceFile, effectBlob, options));
	mxTRY(Util_SaveDataToFile(effectBlob.raw(), effectBlob.num(), destination));
	return ALL_OK;
}
#endif

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
