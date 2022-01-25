//
// Red Faction 1 model loader.
//
// Based on work done by Rafal Harabien:
// https://github.com/rafalh/rf-reversed
//
#pragma once

class TcModel;
class TestSkinnedModel;


namespace RF1
{

ERet load_V3D_from_file(
						TcModel *model_
						, const char* filename
						, ObjectAllocatorI * storage
						, TestSkinnedModel * test_model
						);

}//namespace RF1
