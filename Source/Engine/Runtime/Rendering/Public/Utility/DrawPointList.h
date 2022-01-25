// draw particles/decals as a point list
#pragma once

namespace Rendering
{
namespace RrUtils
{

	ERet DrawPointListWithShader(
		// must be filled by the user
		void *&allocated_vertices_
		, const TbVertexFormat& vertex_type
		, const NwShaderEffect& shader_effect
		, const UINT num_points
		);

	template< class VERTEX >
	ERet T_DrawPointListWithShader(
		// must be filled by the user
		VERTEX *&allocated_vertices_
		, const NwShaderEffect& shader_effect
		, const UINT num_points
		)
	{
		return DrawPointListWithShader(
			(void*&) allocated_vertices_
			, VERTEX::metaClass()
			, shader_effect
			, num_points
			);
	}


}//namespace RrUtils
}//namespace Rendering
