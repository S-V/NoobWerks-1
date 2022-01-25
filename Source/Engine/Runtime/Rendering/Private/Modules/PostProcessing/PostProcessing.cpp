#include <Base/Base.h>
#pragma hdrstop
#include <Core/ObjectModel/Clump.h>
#include <Graphics/Public/graphics_utilities.h>

#if 0
#include <Renderer/PostProcessing.h>

namespace Rendering
{

AmbientObscurance::AmbientObscurance()
{
	intensity = 1.0f;
	radius = 0.3f;	//Mathf.Max(_radius, 1e-4f); }
	sampleCount = SampleCount::Medium;
	downsampling = false;
}

ERet AmbientObscurance::Initialize( Clump * _storage )
{
	return ALL_OK;
}

void AmbientObscurance::Shutdown()
{

}

ERet AmbientObscurance::Render(
	GL::RenderContext & context,
	const RenderPath& _path,
	const NwCameraView& _camera,
	Clump * _storage
)
{
#if 0
	float tw = _camera.viewportWidth;
	float th = _camera.viewportHeight;
	float ts = downsampling ? 2.0f : 1.0f;
	var format = RenderTextureFormat.ARGB32;
	var useGBuffer = occlusionSource == OcclusionSource.GBuffer;

	// AO buffer
	{
		PooledRT	rtMask(
			tw / ts, th / ts, 0, format,
			tr.renderTargetPool
			);

		// AO estimation
		Graphics.Blit(source, rtMask, m, (int)occlusionSource);

		// Separable blur (horizontal pass)
		var rtBlur = RenderTexture.GetTemporary(tw, th, 0, format);
		Graphics.Blit(rtMask, rtBlur, m, useGBuffer ? 4 : 3);
		RenderTexture.ReleaseTemporary(rtMask);
	}
#endif

#if 0
	// Separable blur (vertical pass)
	rtMask = RenderTexture.GetTemporary(tw, th, 0, format);
	Graphics.Blit(rtBlur, rtMask, m, 5);
	RenderTexture.ReleaseTemporary(rtBlur);

	// Composition
	m.SetTexture("_OcclusionTexture", rtMask);
	Graphics.Blit(source, destination, m, 6);
	RenderTexture.ReleaseTemporary(rtMask);

	// Explicitly detach the temporary texture.
	// (This is needed to avoid conflict with CommandBuffer)
	m.SetTexture("_OcclusionTexture", null);
#endif

	return ALL_OK;
}
#if 0

HDR_Renderer::HDR_Renderer()
{
}

ERet HDR_Renderer::Initialize( Clump * _storage )
{
	return ALL_OK;
}

void HDR_Renderer::Shutdown()
{

}

ERet HDR_Renderer::Render(
	HContext context,
	const RenderPath& _path,
	const NwCameraView& _camera,
	Clump * _storage
)
{
#if 0
	float tw = _camera.viewportWidth;
	float th = _camera.viewportHeight;
	float ts = downsampling ? 2.0f : 1.0f;
	var format = RenderTextureFormat.ARGB32;
	var useGBuffer = occlusionSource == OcclusionSource.GBuffer;

	// AO buffer
	{
		PooledRT	rtMask(
			tw / ts, th / ts, 0, format,
			tr.renderTargetPool
			);

		// AO estimation
		Graphics.Blit(source, rtMask, m, (int)occlusionSource);

		// Separable blur (horizontal pass)
		var rtBlur = RenderTexture.GetTemporary(tw, th, 0, format);
		Graphics.Blit(rtMask, rtBlur, m, useGBuffer ? 4 : 3);
		RenderTexture.ReleaseTemporary(rtMask);
	}
#endif

#if 0
	// Separable blur (vertical pass)
	rtMask = RenderTexture.GetTemporary(tw, th, 0, format);
	Graphics.Blit(rtBlur, rtMask, m, 5);
	RenderTexture.ReleaseTemporary(rtBlur);

	// Composition
	m.SetTexture("_OcclusionTexture", rtMask);
	Graphics.Blit(source, destination, m, 6);
	RenderTexture.ReleaseTemporary(rtMask);

	// Explicitly detach the temporary texture.
	// (This is needed to avoid conflict with CommandBuffer)
	m.SetTexture("_OcclusionTexture", null);
#endif

	return ALL_OK;
}
#endif
}//namespace Rendering

#endif
