// 
#pragma once

#include <Rendering/Public/Globals.h>
#include <Rendering/Public/Core/VertexFormats.h>


namespace Rendering
{
typedef DynamicArray< ParticleVertex >	VisibleParticlesT;


class TbParticleSystem
{
public_internal:
	//DynamicArray< ParticleVertex >	_particles;

public:
	ERet Initialize();
	void Shutdown();

public:
	static ERet renderParticles(
		const RenderCallbackParameters& parameters
		);
};

}//namespace Rendering
