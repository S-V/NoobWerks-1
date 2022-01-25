# Graphics - an abstraction of DirectX/OpenGL/Vulkan/... .

###

Contains low-level graphics stuff (common macros,types and functions).
Graphics only provides a platform-independent access to the low-level graphics device.
Graphics does not dictate how the higher-level scene management or rendering should be organized.



### TODO


template< class ConstantBufferType >
ERet BindCBufferDataByReference(
	const ConstantBufferType& uniform_data
	, const UINT buffer_slot
	, const HBuffer buffer_handle
	, const char* dbgname = nil
	)
{
	mxSTATIC_ASSERT2(sizeof(ConstantBufferType) >= 16, Cannot_pass_pointers);
	return Commands::BindCBufferDataByReference(
		&uniform_data
		, sizeof(ConstantBufferType)
		, _command_buffer
		, buffer_slot
		, buffer_handle
		, dbgname
		);
}

template< class ConstantBufferType >
ERet BindCBufferDataWithDeepCopy


- shader effect should include a full description of render states

+ description structs (e.g. NwBlendDescription) should not inherit from NamedObject,
they should have a fixed size






## References:

Writing a modern rendering engine
https://gist.github.com/Leandros/1183c124d326978e5b6f1550d57f8e78

Stateless, layered, multi-threaded rendering
http://molecularmusings.wordpress.com/2014/11/06/stateless-layered-multi-threaded-rendering-part-1/
http://molecularmusings.wordpress.com/2014/11/13/stateless-layered-multi-threaded-rendering-part-2-stateless-api-design/
https://molecularmusings.wordpress.com/2014/12/16/stateless-layered-multi-threaded-rendering-part-3-api-design-details/

Implementing a Graphic Driver Abstraction
http://diligentgraphics.com/2017/11/23/designing-a-modern-cross-platform-graphics-library/
http://entland.homelinux.com/blog/2008/03/03/implementing-a-graphic-driver-abstraction/
Order your graphics draw calls around!	
http://realtimecollisiondetection.net/blog/?p=86
Input latency
http://realtimecollisiondetection.net/blog/?p=30
Streamlined 3D API for current/upcoming GPU
http://forum.beyond3d.com/showthread.php?t=63565
How to make a rendering engine
http://c0de517e.blogspot.ru/2014/04/how-to-make-rendering-engine.html
My little rendering engine
http://c0de517e.blogspot.ru/2009/03/my-little-rendering-engine.html

http://seanmiddleditch.com/journal/2014/02/direct3d-11-debug-api-tricks/

http://www.gamedev.ru/code/forum/?id=195323
http://www.gamedev.ru/code/forum/?id=194691


### Reference implementations:
	
https://github.com/tuxalin/CommandBuffer
	

### Design notes:

https://www.chromium.org/developers/design-documents/gpu-command-buffer
http://fabiensanglard.net/doom3/doom3_notes.txt
http://fabiensanglard.net/quake3/renderer.php
https://pplux.github.io/why_px_render.html


### Shader/Effect/Material system:

Filament Materials Guide [2020.07.30]
https://github.com/google/filament/blob/main/docs/Materials.md.html

GPU Gems
Chapter 36. Integrating Shaders into Applications (John O'Rorke, Monolith Productions)
http://developer.download.nvidia.com/books/HTML/gpugems/gpugems_ch36.html

Apple's SceneKit
https://developer.apple.com/documentation/scenekit/scntechnique


### Optimization:

GPU Optimization for GameDev
https://gist.github.com/silvesthu/505cf0cbf284bb4b971f6834b8fec93d
