#pragma once


/// used for compiling shaders, represents a shader #define
struct Macro
{
	const char* name;
	const char* value;
};

struct NwFileIncludeI
{
	//NOTE: each OpenFile() must be paired with the corresponding CloseFile()!
	virtual bool OpenFile( const char* filename, char **filedata_, U32 *filesize_ ) = 0;
	virtual void CloseFile( char* filedata_ ) = 0;

public:	//	Debugging
	// should return the full path to the currently opened file (for debugging)
	virtual const char* CurrentFilePath() const { return mxSTRING_QUESTION_MARK; }

	virtual void DbgPrintIncludeStack() const {}

protected:
	virtual ~NwFileIncludeI() {}
};
