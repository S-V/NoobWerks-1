#include <Base/Base.h>
#pragma hdrstop
#include <GPU/Public/render_context.h>
#include <Graphics/Public/graphics_shader_system.h>

#include <Rendering/Public/Core/RenderPipeline.h>
#include <Rendering/Private/RenderSystemData.h>
#include <Rendering/Private/Modules/Particles/Particle_Rendering.h>


namespace Rendering
{
ERet TbParticleSystem::Initialize()
{
	return ALL_OK;
}

void TbParticleSystem::Shutdown()
{

}

ERet TbParticleSystem::renderParticles(
	const RenderCallbackParameters& parameters
	)
{
	tbPROFILE_FUNCTION;

	const RenderSystemData& render_sys = *Globals::g_data;

	const NwCameraView& scene_view = parameters.scene_view;
	const RenderPath& render_path = parameters.render_path;
	NGpu::NwRenderContext & render_context = parameters.render_context;

	//
	const U32 scene_pass_index = render_path.getPassDrawingOrder( ScenePasses::VolumetricParticles );

	//
	const VisibleParticlesT& visible_particles = *(VisibleParticlesT*) parameters.entities._data[0];

	//
	NwShaderEffect* shader_effect = nil;
	mxDO(Resources::Load( shader_effect, MakeAssetID("particles") ));

	const NwShaderEffect::Variant shader_effect_variant = shader_effect->getDefaultVariant();

	//
	NwTransientBuffer	vertex_buffer;
	mxDO(NGpu::allocateTransientVertexBuffer(
		vertex_buffer
		, visible_particles.num(), sizeof(visible_particles[0])
		));

	TCopyArray( (ParticleVertex*)vertex_buffer.data, visible_particles.raw(), visible_particles.num() );

	//
	NGpu::NwPushCommandsOnExitingScope	applyShaderCommands(
		render_context,
		NGpu::buildSortKey( scene_pass_index, 0 )
			nwDBG_CMD_SRCFILE_STRING
		);

	//
	NGpu::CommandBufferWriter	cmd_writer( render_context._command_buffer );

	//
	mxDO(shader_effect->SetInput(
		nwNAMEHASH("DepthTexture"),
		NGpu::AsInput(render_sys._deferred_renderer.m_depthRT->handle)
		));

//shader_effect->debugPrint();

	shader_effect->pushShaderParameters( render_context._command_buffer );

	//
	NGpu::Cmd_Draw	batch;
	{
		batch.program = shader_effect_variant.program_handle;

		batch.SetMeshState(
			ParticleVertex::metaClass().input_layout_handle,
			vertex_buffer.buffer_handle,
			HBuffer::MakeNilHandle(),
			NwTopology::PointList,
			false	// 32-bit indices?
			);

		batch.base_vertex = vertex_buffer.base_index;
		batch.vertex_count = visible_particles.num();
		batch.start_index = 0;
		batch.index_count = 0;
	}
	IF_DEBUG batch.src_loc = GFX_SRCFILE_STRING;
	cmd_writer.SetRenderState(shader_effect_variant.render_state);
	cmd_writer.Draw( batch );

	return ALL_OK;
}

}//namespace Rendering
