// Skeletal animation system
#include <Base/Base.h>
#pragma hdrstop
#include <Base/Template/TRange.h>

#include <Core/Client.h>
#include <Core/CoreDebugUtil.h>
#include <Core/Memory/MemoryHeaps.h>
#include <Core/Serialization/Serialization.h>
#include <Core/Tasking/TaskSchedulerInterface.h>	// threadLocalHeap()
#include <Core/Util/Tweakable.h>

#include <Rendering/Private/Modules/Decals/_DecalsSystem.h>
#include <Rendering/Private/RenderSystemData.h>
#include <Rendering/Public/Utility/DrawPointList.h>
#include <Rendering/Public/Globals.h>


namespace Rendering
{
NwDecalsSystem::NwDecalsSystem()
{
}

NwDecalsSystem::~NwDecalsSystem()
{
}


ERet NwDecalsSystem::Initialize()
{
	return ALL_OK;
}

void NwDecalsSystem::Shutdown()
{

}

void NwDecalsSystem::AdvanceDecalsOncePerFrame(
	const NwTime& game_time
	)
{
	const SecondsF PLASMA_BURN_GLOW_SECONDS = 4.0f;
	const F32 inv_plasma_burn_glow_seconds = 1.0f / PLASMA_BURN_GLOW_SECONDS;

	////
	static float SCORCHMARK_MIN_RADIUS = 0.27f;
	HOT_VAR(SCORCHMARK_MIN_RADIUS);

	static float SCORCHMARK_MAX_RADIUS = 0.3f;
	HOT_VAR(SCORCHMARK_MAX_RADIUS);

	//
	ScorchMark* scorch_marks_ptr = scorch_marks.raw();
	UINT num_alive_scorch_marks = scorch_marks.num();

	UINT iDecal = 0;

	while( iDecal < num_alive_scorch_marks )
	{
		ScorchMark &	scorch_mark = scorch_marks_ptr[ iDecal ];

		const F32 particle_life = scorch_mark.life;
		const F32 particle_life_new = particle_life + game_time.real.delta_seconds * inv_plasma_burn_glow_seconds;

		if( particle_life_new < 1.0f )
		{
			// the scorch_mark is still glowing
			//
			const F32 decal_radius_new = SCORCHMARK_MIN_RADIUS
				+ (SCORCHMARK_MAX_RADIUS - SCORCHMARK_MIN_RADIUS) * particle_life_new
				;

			scorch_mark.radius = decal_radius_new;
			scorch_mark.life = particle_life_new;
		}
		else
		{
			// the scorch_mark is "cold"
			scorch_mark.life = BIG_NUMBER;

			//particle_life_new = minf(particle_life_new, 1.0f);
			// the scorch_mark is dead - remove it
			// 
//			TSwap( scorch_mark, scorch_marks_ptr[ --num_alive_scorch_marks ] );
		}

		// we don't remove decals at all
		++iDecal;
	}

	//
	scorch_marks.setNum( num_alive_scorch_marks );
}

/// Builds a right-handed orthonormal basis,
/// e.g. N = (0,0,1) => T = (1,0,0), B = (0,1,0).
///
void BuildOrthonormalBasisFromNormal_Duff(
	const V3f& N,
	V3f &T_, V3f &B_
	)
{
	mxASSERT(N.isNormalized());

	const float  s  = (0.0f > N.z) ? -1.0f : 1.0f;
	const float  a0 = -1.0f / (s + N.z);
	const float  a1 = N.x * N.y * a0;

	T_.x = 1.0f + s * N.x * N.x * a0;
	T_.y = s * a1;
	T_.z = -s * N.x;

	B_.x = a1;
	B_.y = s + N.y * N.y * a0;
	B_.z = -N.y;
}


ERet NwDecalsSystem::RenderDecalsIfAny(
	SpatialDatabaseI* spatial_database
	)
{
	const UINT num_alive_scorch_marks = scorch_marks.num();
	if(!num_alive_scorch_marks) {
		return ALL_OK;
	}

	// Sort smoke puffs to render them back-to-front.

	AllocatorI& scratch_alloc = threadLocalHeap();

	//
	RrDecalVertex *	allocated_vertices;

	NwShaderEffect* shader_effect = nil;
	mxDO(Resources::Load( shader_effect,
		MakeAssetID("plasma_impact_decals")
		));

	{
		const RenderSystemData& render_system_data = *Globals::g_data;	
		//
		/*mxDO*/(shader_effect->SetInput(
			nwNAMEHASH("DepthTexture"),
			NGpu::AsInput(render_system_data._deferred_renderer.m_depthRT->handle)
			));

		//
#if 0
		/*mxDO*/(shader_effect->SetInput(
			mxHASH_STR("GBufferTexture0"),
			GL::AsInput(render_system_data._deferred_renderer.m_colorRT0->handle)
			));

		/*mxDO*/(shader_effect->SetInput(
			mxHASH_STR("GBufferTexture1"),
			GL::AsInput(render_system_data._deferred_renderer.m_colorRT1->handle)
			));

		/*mxDO*/(shader_effect->SetInput(
			mxHASH_STR("GBufferTexture2"),
			GL::AsInput(render_system_data._deferred_renderer.m_colorRT2->handle)
			));
#endif

		//NwTexture *	decal_texture;
		//mxDO(Resources::Load( decal_texture, MakeAssetID("plasma_burn") ));

		///*mxDO*/(shader_effect->SetInput(
		//	mxHASH_STR("t_decal"),
		//	decal_texture->m_resource
		//	));
	}

	mxDO(RrUtils::T_DrawPointListWithShader(
		allocated_vertices
		, *shader_effect
		, num_alive_scorch_marks
		));

	//
	const V3f PLASMA_GLOW_COLOR = CV3f(0.95f, 0.3f, 0.2f);

UNDONE;
#if 0
	//
	nwFOR_LOOP(UINT, i, num_alive_scorch_marks)
	{
		const ScorchMark& scorch_mark = scorch_marks._data[ i ];

		//
		RrDecalVertex &decal_vertex = *allocated_vertices++;
		decal_vertex.position_and_size = V4f::set(
			scorch_mark.center,
			scorch_mark.radius
			);
		decal_vertex.color_and_life.w = scorch_mark.life;

		//
		V3f	decal_axisX, decal_axisY;
		BuildOrthonormalBasisFromNormal_Duff(
			scorch_mark.normal,
			decal_axisX, decal_axisY
			);
mxOPTIMIZE("precompute basis");
		decal_vertex.decal_axis_x = V4f::set(
			decal_axisX,
			0
			);
		decal_vertex.decal_normal = V4f::set(
			scorch_mark.normal,
			0
			);

		// if it's still glowing:
		mxOPTIMIZE("put glowing at the end!");
		if( scorch_mark.life < 1.0f )
		{
			TbDynamicPointLight	plasma_light;
			plasma_light.position = scorch_mark.center + scorch_mark.normal * 0.1f;
			plasma_light.radius = 1.0f; // in meters
			plasma_light.color = PLASMA_GLOW_COLOR * maxf(1 - scorch_mark.life, 0);

			spatial_database->AddDynamicPointLight(plasma_light);
		}
	}
#endif

	return ALL_OK;
}

void NwDecalsSystem::AddScorchMark(
	const V3f& pos_world
	, const V3f& normal
	)
{
	mxASSERT(normal.isNormalized());

	ScorchMark	new_scorch_mark;
	{
		new_scorch_mark.center = pos_world;
		new_scorch_mark.radius = 0;
		new_scorch_mark.normal = normal;
		new_scorch_mark.life = 0;
	}

	scorch_marks.AddWithWraparoundOnOverflow(new_scorch_mark);
}




tbDEFINE_VERTEX_FORMAT(RrDecalVertex);
mxBEGIN_REFLECTION( RrDecalVertex )
	mxMEMBER_FIELD( position_and_size ),
	mxMEMBER_FIELD( color_and_life ),
	mxMEMBER_FIELD( decal_axis_x ),
	mxMEMBER_FIELD( decal_normal ),
mxEND_REFLECTION
void RrDecalVertex::buildVertexDescription( NwVertexDescription *description_ )
{
	description_->begin()
		.add(NwAttributeType::Float4, NwAttributeUsage::Position, 0)
		.add(NwAttributeType::Float4, NwAttributeUsage::TexCoord, 0)
		.add(NwAttributeType::Float4, NwAttributeUsage::TexCoord, 1)
		.add(NwAttributeType::Float4, NwAttributeUsage::TexCoord, 2)
		.end();
}

}//namespace Rendering
