#include "stdafx.h"
#pragma hdrstop

#include <Base/Template/Containers/Blob.h>
#include <Core/Text/Lexer.h>
#include <Graphics/Public/graphics_utilities.h>
#include <Developer/RendererCompiler/Effect_Compiler.h>
#include <Developer/RendererCompiler/EffectCompiler_Direct3D.h>
#include <Developer/RendererCompiler/Target_Common.h>

#include <AssetCompiler/AssetCompilers/Graphics/ShaderEffectCompiler.h>
#include <AssetCompiler/ShaderCompiler/ShaderCompiler.h>


#if WITH_SHADER_EFFECT_COMPILER


#define DBG_FILE_INCLUDE	(0)


namespace AssetBaking
{

mxDEFINE_CLASS( ShaderEffectCompilerSettings );
mxBEGIN_REFLECTION( ShaderEffectCompilerSettings )
	//mxMEMBER_FIELD( search_paths ),
mxEND_REFLECTION
ShaderEffectCompilerSettings::ShaderEffectCompilerSettings()
{
}

mxDEFINE_CLASS(ShaderEffectCompiler);

ERet ShaderEffectCompiler::Initialize()
{
	return ALL_OK;
}

void ShaderEffectCompiler::Shutdown()
{
}

ERet ShaderEffectCompiler::CompileAsset(
	const AssetCompilerInputs& inputs,
	AssetCompilerOutputs &outputs
	)
{
	NwBlob	fileData(MemoryHeaps::temporary());
	mxDO(NwBlob_::loadBlobFromFile( fileData, inputs.path.c_str() ));

	//
	mxTRY(ShaderCompilation::compileEffectFromBlob(
		fileData
		, inputs.path.c_str()
		, inputs.cfg.shader_compiler
		, inputs.asset_database
		, outputs
	));

	return ALL_OK;
}


MyFileInclude::MyFileInclude( const char* filepath, AssetDatabase* db /*= NULL*/ )
	: _asset_database( db )
{
	// add an empty search path to avoid checks when opening a file in the current folder
	_search_paths.AddNew();

	//
	_SetCurrentDirectoryFromFilePath(filepath);

	//
	String256 root_file_name;
	Str::CopyS(root_file_name, filepath);
	Str::StripPath(root_file_name);

	//
	OpenedFile::Ref root_file = new OpenedFile();
	root_file->name = NameID(root_file_name.c_str());
	root_file->path = NameID(filepath);
	root_file->data = NULL;
	root_file->size = 0;

	_opened_files.add(root_file);
}

MyFileInclude::~MyFileInclude()
{
}

ERet MyFileInclude::AddSearchPath( const char* new_search_folder )
{
	FilePathStringT normalized_path;
	mxDO(Str::CopyS(normalized_path, new_search_folder));
	Str::StripFileName(normalized_path);
	Str::NormalizePath(normalized_path);

	_search_paths.AddUnique( NameID(normalized_path.c_str()) );

	return ALL_OK;
}

bool MyFileInclude::OpenFile(
							 const char* included_filename
							 , char **opened_file_data_, UINT *opened_file_size_
							 )
{
	// First search in the folder containing the currently #included file.

	if( _OpenFileInFolder(
		included_filename, _current_directory.c_str()
		, opened_file_data_, opened_file_size_
		) )
	{
		return true;
	}

	//
	for( U32 i = 0; i < _search_paths.num(); i++ )
	{
		const char* search_path = _search_paths[i].c_str();

		if( _OpenFileInFolder(
			included_filename, search_path
			, opened_file_data_, opened_file_size_
			) )
		{
			return true;
		}
	}

	return false;
}

void MyFileInclude::_SetCurrentDirectoryFromFilePath( const char* filepath )
{
	Str::CopyS(_current_directory, filepath);
	Str::StripFileName(_current_directory);
}

bool MyFileInclude::_OpenFileInFolder(
									  const char* filename, const char* folder
									  , char **filedata_, UINT *filesize_
									  )
{
	String256 filepath;
	Str::ComposeFilePath( filepath, folder, filename );

	if(mxSUCCEDED(Util_LoadFileToString2(
		filepath.c_str()
		, filedata_, filesize_
		, MemoryHeaps::strings()
		)))
	{
		const OpenedFile& parent_file = *_opened_files.getLast();

#if DBG_FILE_INCLUDE
		DEVOUT("#Including '%s' from '%s'", filename, parent_file.path.c_str());
#endif

		if( _asset_database ) {
			_asset_database->addDependency( filepath.c_str(), parent_file.path.c_str() );
		}

		OpenedFile::Ref new_opened_file = new OpenedFile();
		new_opened_file->name = NameID(filename);
		new_opened_file->path = NameID(filepath.c_str());
		new_opened_file->data = *filedata_;
		new_opened_file->size = *filesize_;

		_opened_files.add(new_opened_file);

		//
		_SetCurrentDirectoryFromFilePath(filepath.c_str());

		return true;
	}

	return false;
}

void MyFileInclude::CloseFile( char* included_filedata )
{
	OpenedFile * last_opened_file = _opened_files.getLast();

	mxASSERT(last_opened_file->data == included_filedata);

#if DBG_FILE_INCLUDE
	const NameID& lastFileName = last_opened_file->name;
	DEVOUT("Closing '%s'", lastFileName.c_str());
#endif

	Util_DeleteString2(
		last_opened_file->data, last_opened_file->size
		, MemoryHeaps::strings()
		);
	last_opened_file->data = NULL;
	
	_opened_files.PopLastValue();

	//
	if( !_opened_files.IsEmpty() )
	{
		_SetCurrentDirectoryFromFilePath(_opened_files.getLast()->path.c_str());
	}
	else
	{
		_current_directory.empty();
	}
}

void MyFileInclude::DbgPrintIncludeStack() const
{
	for( U32 i = 0; i < _opened_files.num(); i++ )
	{
		const OpenedFile& opened_file = *_opened_files[i];
		ptPRINT("[%d]:\n", i, opened_file.name.c_str());
	}
	ptPRINT("Current directory: '%s'\n", _current_directory.c_str());
}

const char* MyFileInclude::CurrentFilePath() const
{
	const OpenedFile& last_opened_file = *_opened_files.getLast();
	return last_opened_file.path.c_str();
}

}//namespace AssetBaking

#endif // WITH_SHADER_EFFECT_COMPILER
