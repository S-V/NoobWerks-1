// Voxel terrain tools, settings and interfaces.
#pragma once

namespace VisualDebugger
{

///
class Writer: NonCopyable
{
	FileWriter	_file;

public:
	Writer( const char* dump_filename );
	~Writer();
};


}//namespace VisualDebugger
