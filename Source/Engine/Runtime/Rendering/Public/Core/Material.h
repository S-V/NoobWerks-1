/*
	Graphics materials describe how geometry is rendered.
*/
#pragma once

#include <Base/Template/Containers/Array/TInplaceArray.h>	// TInplaceArray<>

#include <Core/Assets/AssetManagement.h>
#include <Core/ObjectModel/Clump.h>
#include <Core/Assets/AssetLoader.h>

#include <Graphics/Public/graphics_utilities.h>
#include <Graphics/Public/graphics_shader_system.h>

#include <Rendering/Public/Core/RenderPipeline.h>
#include <Rendering/Public/Core/Material.h>


namespace Rendering
{

/// Pre-computes data from the corresponding effect pass,
/// based on the current rendering pipeline,
/// and caches it to reduce pointer chasing
/// during batch filtering and submission.
struct MaterialPass: CStruct
{
	NwRenderState32		render_state;
	TbPassIndexT		filter_index;	// used for filtering batches during submission
	NGpu::LayerID			draw_order;	//
	HProgram 			program;
public:
	mxDECLARE_CLASS(MaterialPass, CStruct);
	mxDECLARE_REFLECTION;
};
ASSERT_SIZEOF(MaterialPass, 8);


/// A graphics material = shader code (technique) + shader data (parameters).
class Material
	: public DeclareTypedefsForSharedResource< Material >
{
public:

	/// Cached passes to avoid pointer chasing when dereferencing the effect/data.
	TSpan< const MaterialPass >	passes;

	/// A non-owning reference to the buffer with commands for setting shader state
	CommandBufferChunk		command_buffer;


	/// A pointer to the corresponding shader effect, must always be valid
	TPtr< NwShaderEffect >		effect;



	/// loaded as a single memory blob
	struct VariableLengthData: CStruct
	{
		TBuffer< MaterialPass >	passes;

		/// contains graphics commands
		/// to update and bind uniform buffers, bind shader resources and setup the render states
		/// for rendering with this material
		CommandBufferChunk		command_buffer;

		///
		UniformBufferT			uniforms;

		/// keeps strong references to used textures
		TBuffer< NwShaderTextureRef >	referenced_textures;

		/// used for draw-call storing;
		/// the idea is that similar materials will have similar hashes
		//U32							sort_key;

	public:
		mxDECLARE_CLASS(VariableLengthData, CStruct);
		mxDECLARE_REFLECTION;
	};

	VariableLengthData *	data;

	///for debugging
	AssetID					_id;

public:
	mxDECLARE_CLASS( Material, NwSharedResource );
	mxDECLARE_REFLECTION;

	Material();
	~Material();

	ERet setUniform(
		const NameHash32 name_hash
		, const void* uniform_data
		);

	ERet SetInput(
		const NwNameHash& name_hash
		, HShaderInput handle
		);

public:
	static AssetID getDefaultAssetId();
	static Material* getFallbackMaterial();
};






/*
----------------------------------------------------------
	MaterialLoader
----------------------------------------------------------
*/
class MaterialLoader: public TbAssetLoader_Null
{
	typedef TbAssetLoader_Null Super;
public:
	MaterialLoader( ProxyAllocator & parent_allocator );

	virtual ERet create( NwResource **new_instance_, const TbAssetLoadContext& context ) override;
	virtual ERet load( NwResource * instance, const TbAssetLoadContext& context ) override;
	virtual void unload( NwResource * instance, const TbAssetLoadContext& context ) override;
	virtual ERet reload( NwResource * instance, const TbAssetLoadContext& context ) override;

public:

	enum {
		SIGNATURE = MCHAR4('M','T','R','L'),
		PASSDATA_ALIGNMENT = 8,
	};

#pragma pack (push,1)
	/// declarations for loading/parsing renderer assets
	struct MaterialHeader_d
	{
		U32			fourCC;	//!< four-character code
	};
	ASSERT_SIZEOF(MaterialHeader_d, 4);
#pragma pack (pop)

};
}//namespace Rendering

/* References:

Godot:
https://docs.godotengine.org/en/3.0/tutorials/3d/spatial_material.html

Material system overview in Torque 3D game engine:
http://wiki.torque3d.org/coder:material-system-overview

Accessing and Modifying Material parameters via script
https://docs.unity3d.com/Manual/MaterialsAccessingViaScript.html
*/
