// draw particles/decals as a point list
#include <Base/Base.h>
#pragma hdrstop
#include <Rendering/Public/Globals.h>
#include <Rendering/Public/Settings.h>
#include <Rendering/Private/RenderSystemData.h>
#include <Rendering/Public/Utility/DrawPointList.h>

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
		)
	{
		mxASSERT(num_points > 0);
		mxASSERT2(num_points < MAX_UINT16, "Only 16-bit indices are supported!");

		//
		const RenderPath& render_path = Globals::GetRenderPath();


		//
		NwTransientBuffer	transient_vertex_buffer;
		mxDO(NGpu::allocateTransientVertexBuffer(
			transient_vertex_buffer
			, num_points
			, vertex_type.m_size
			));

		//
		allocated_vertices_ = transient_vertex_buffer.data;

		//
		NGpu::NwRenderContext & render_context = NGpu::getMainRenderContext();


		//
		const TSpan< const NwShaderEffect::Pass >	passes = shader_effect.getPasses();

		//
		for( UINT iPass = 0; iPass < passes._count; iPass++ )
		{
			const NwShaderEffect::Pass& pass = passes._data[ iPass ];
			const U32 scene_pass_index = render_path.getPassDrawingOrder( pass.name_hash );

			//
			NGpu::NwPushCommandsOnExitingScope	applyShaderCommands(
				render_context,
				NGpu::buildSortKey( scene_pass_index, 0 )
			nwDBG_CMD_SRCFILE_STRING
				);

			//
			NGpu::CommandBufferWriter	cmd_writer( render_context._command_buffer );

			//
			shader_effect.pushAllCommandsInto( render_context._command_buffer );

			//
			NGpu::Cmd_Draw	batch;
			{
				batch.program = pass.default_program_handle;

				batch.SetMeshState(
					vertex_type.input_layout_handle,
					transient_vertex_buffer.buffer_handle,
					HBuffer::MakeNilHandle(),
					NwTopology::PointList,
					false	// 32-bit indices?
					);

				batch.base_vertex = transient_vertex_buffer.base_index;
				batch.vertex_count = num_points;
				batch.start_index = 0;
				batch.index_count = 0;
			}
			IF_DEBUG batch.src_loc = GFX_SRCFILE_STRING;
			cmd_writer.SetRenderState(pass.render_state);
			cmd_writer.Draw( batch );
		}//For each pass.

		return ALL_OK;
	}

}//namespace RrUtils

}//namespace Rendering
