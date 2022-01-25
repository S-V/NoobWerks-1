// compiles *.fx files into NwShaderEffect blobs
#pragma once

#include <AssetCompiler/AssetPipeline.h>

#if WITH_SHADER_EFFECT_COMPILER
#include <Graphics/Public/graphics_formats.h>

namespace AssetBaking
{

struct ShaderEffectCompilerSettings: CStruct
{
public:
	mxDECLARE_CLASS(ShaderEffectCompilerSettings,CStruct);
	mxDECLARE_REFLECTION;
	ShaderEffectCompilerSettings();
};

struct ShaderEffectCompiler: public CompilerFor< NwShaderEffect >
{
	mxDECLARE_CLASS( ShaderEffectCompiler, AssetCompilerI );

	virtual ERet Initialize() override;
	virtual void Shutdown() override;

	virtual ERet CompileAsset(
		const AssetCompilerInputs& inputs,
		AssetCompilerOutputs &outputs
		) override;

private:
	ShaderEffectCompilerSettings	m_settings;
};


class MyFileInclude : public NwFileIncludeI
{
	struct OpenedFile: ReferenceCounted, NonCopyable
	{
		typedef TRefPtr<OpenedFile>	Ref;

		NameID	name;	// filename without path (e.g. "game_spaceship.fx")
		NameID	path;	// fully-qualified filepath (e.g. "R:/NoobWerks/Assets/materials/game_spaceship.fx")

		char *	data;	// file contents
		size_t	size;	// file size
	};

	/// for tracking #include dependencies
	AssetDatabase *		_asset_database;

	FilePathStringT	_current_directory;

	TInplaceArray< NameID, 32 >			_search_paths;
	TInplaceArray< OpenedFile::Ref, 32 >	_opened_files;

public:
	MyFileInclude( const char* filepath, AssetDatabase* db = NULL );
	~MyFileInclude();

	ERet AddSearchPath( const char* new_search_folder );

	virtual bool OpenFile(
		const char* included_filename
		, char **opened_file_data_, UINT *opened_file_size_
		) override;

	virtual void CloseFile( char* included_filedata ) override;

	virtual void DbgPrintIncludeStack() const override;

	virtual const char* CurrentFilePath() const override;

private:
	void _SetCurrentDirectoryFromFilePath( const char* filepath );

	bool _OpenFileInFolder(
		const char* filename, const char* folder
		, char **filedata_, UINT *filesize_
		);
};

}//namespace AssetBaking

#endif // WITH_SHADER_EFFECT_COMPILER
