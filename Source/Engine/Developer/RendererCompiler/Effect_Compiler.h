// effect compiler library interface, public header file
#pragma once

#include <Base/Template/Containers/Array/TInplaceArray.h>
#include <Base/Text/FileInclude.h>
#include <Graphics/Public/graphics_formats.h>

enum EShaderTarget {
	PC_Direct3D_11,
	PC_OpenGL_4plus,
};
mxDECLARE_ENUM( EShaderTarget, U32, ShaderTargetT );

/// static shader switches are set by the artist during development;
/// they are used to create different material shader variations;
struct FxDefine : CStruct
{
	String64	name;
	String64	value;
public:
	mxDECLARE_CLASS( FxDefine, CStruct );
	mxDECLARE_REFLECTION;
};


/// E.g. "ENABLE_SPECULAR_MAP" = "1"
typedef TKeyValue< String64, int >	FeatureSwitch;


struct FxOptions
{
	String64	source_file;	//!< shader source file name (e.g. "cool_shader.hlsl")

	TInplaceArray< FxDefine, 32 >	defines;

	NwFileIncludeI *	include;	//!< handles opening #included source files

	bool	optimize_shaders;
	bool	strip_debug_info;	//!< remove metadata and string names?

	/// folder where disassembled shaders will be stored
	const char*	disassembly_path;	// whereToSaveDisassembledShaderCode

public:
	FxOptions();
};

#if 0

enum EShaderTarget {
	PC_Direct3D_11,
	PC_OpenGL_4plus,
};
mxDECLARE_ENUM( EShaderTarget, U32, ShaderTargetT );

struct FxOptions : CStruct
{
///	ShaderTargetT	target;

	TArray< FxDefine > defines;

	StringListT	search_paths;	// paths for opening source files
	//String	pathToGenCppFiles;	// path to generated .h/.cpp files

	//// testing and debugging
	//String		outputPath;
	//String	whereToSavePreprocessedShaderCode;
	//String	whereToSaveDisassembledShaderCode;
	//String	whereToSaveParsedShaderCode;	// folder to save source tree dump into
	//String	whereToSaveGeneratedShaderCode;
	//bool		dumpPreprocessedShaders;
	//bool		dumpDisassembledShaders;

	// whereToSaveDisassembledShaderCode
	String	debugDumpPath;

	bool	dumpPipeline;		// dump text-serialized library?
	bool	dumpShaderCode;		// save generated HLSL code on disk?
	bool	optimizeShaders;
	//bool	dumpDisassembly;
	bool	stripReflection;	// remove shader metadata
	bool	stripSymbolNames;	// remove string names (but leave name hashes)
	bool	generateCppHeaders;	// generate C/C++ headers?

public:
	mxDECLARE_CLASS( FxOptions, CStruct );
	mxDECLARE_REFLECTION;
	FxOptions();
};

// Compiles the source asset file into two parts:
// 1) the memory-resident part (a clump containing effect library structures);
// 2) the temporary data that is used only for initializing run-time structures and exists only during loading (compiled shader code);
ERet FxCompileEffectFromFile(const char* filename,
							 ByteArrayT &effectBlob,
							 const FxOptions& options);

ERet FxCompileAndSaveEffect(const char* sourceFile,
							const char* destination,
							const FxOptions& options);
#endif

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
