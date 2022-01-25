// Rendering pipeline.
// https://urho3d.github.io/documentation/1.6/_render_paths.html
// https://github.com/amethyst/amethyst/wiki/Renderer-Design
#pragma once

#include <Base/Template/Algorithm/Search.h>	// LowerBoundAscending()
#include <Graphics/Public/graphics_formats.h>	// shader system


namespace Rendering
{

/*
=====================================================================
    RENDERING PIPELINE
=====================================================================
*/

enum ESortMode
{
	Sort_FrontToBack,
	Sort_BackToFront,
};
mxDECLARE_ENUM( ESortMode, U8, SortModeT );


typedef NameHash32 RrSceneLayerNameT;





/// used for filtering batches during submission
typedef U8 TbPassIndexT;

/// used for filtering batches during submission
typedef U32 TbPassMaskT;


/// used for filtering batches during submission
///TODO: WIP!!!
struct PassFilter
{
	enum Enum
	{
		Bad,

		Opaque,
		Translucent,


		Debug,
		DebugWireframe,

		PostFx,

		UI,

		Count
	};
};
mxDECLARE_FLAGS( PassFilter::Enum, U32, PassFilters );

mxSTATIC_ASSERT2(PassFilter::Count <= 32, Pass_filter_mask_must_fit_into_32_bits);



// The number of passes must fit into TbPassIndexT!
enum { tbMAX_PASSES_IN_RENDER_PIPELINE = 256 };

/// must be compact
struct ScenePassInfo: CStruct
{
	/// must be recomputed whenever the number of sub-passes changes
	NGpu::LayerID		draw_order;

	/// the number of sub-passes for this pass
	/// (e.g. the shadow map rendering pass will have as many sub-passes
	/// as there are shadow cascades); must be >= 1
	U8				num_sublayers;

	/// used for filtering batches during submission
	TbPassIndexT	filter_index;

	///
	U8				passDataIndex;

public:
	mxDECLARE_CLASS( ScenePassInfo, CStruct );
	mxDECLARE_REFLECTION;
	ScenePassInfo();
};
ASSERT_SIZEOF(ScenePassInfo, sizeof(U32));

/// Scene [Pass|Layer|Stage|Phase]
struct ScenePassData: CStruct
{
	/// index of the pass's name hash
	U8	sortedNameHashIndex;

	/// render targets to set
	TBuffer< NameHash32 >	render_targets;	//!< namehash == 0 -> default RT

	NameHash32		depth_stencil_name_hash;	//!< 0 == none
	//RenderState32		renderState;
	//Viewport		viewport;

	String		profiling_scope;	//!< should be empty in release builds

	U32			view_flags;	//!< NGpu::GfxViewStateFlags (e.g. which render targets to clear)

	/// the value to clear the depth buffer with (default=1.)
	float		depth_clear_value;

	/// the value to clear the stencil buffer with (default=0)
	U8			stencil_clear_value;

public:
	mxDECLARE_CLASS( ScenePassData, CStruct );
	mxDECLARE_REFLECTION;
	ScenePassData();

	enum { BACKBUFFER_ID = 0 };
};

mxDECLARE_32BIT_HANDLE(ScenePassHandle);

/// Render [Pipeline|Loop|Path|Config] for data-driven rendering.
/// A render path is described in a simple config file (similar to Horde3D's '.pipeline').
struct RenderPath: CStruct
{
	mxOPTIMIZE("store [name: NGpu::Layer/Scene pass] table");
	/// these are sorted by increasing order for binary search
	TBuffer< NameHash32 >		sortedPassNameHashes;

	/// must be recomputed whenever the number of sub-passes changes
	TBuffer< ScenePassInfo >	passInfo;

	/// in the original order, as declared in the render pipeline script
	TBuffer< ScenePassData >	passData;

public:
	mxDECLARE_CLASS( RenderPath, CStruct );
	mxDECLARE_REFLECTION;

	mxFORCEINLINE const ScenePassHandle findScenePass( NameHash32 passNameHash ) const
	{
		const NameHash32* nameHashes = sortedPassNameHashes.raw();
		const UINT index = LowerBoundAscending( passNameHash, nameHashes, sortedPassNameHashes.num() );
		return ( nameHashes[index] == passNameHash ) ? ScenePassHandle::MakeHandle( index ) : ScenePassHandle::MakeNilHandle();
	}

	const ScenePassInfo getPassInfo( NameHash32 passNameHash ) const
	{
		const ScenePassHandle handle = findScenePass( passNameHash );
		mxASSERT(handle.IsValid());
		return passInfo[ handle.id ];
	}

	const NGpu::LayerID getPassDrawingOrder( NameHash32 passNameHash ) const
	{
		return getPassInfo( passNameHash ).draw_order;
	}

public:

	void setNumSubpasses( const ScenePassHandle handle, U8 num_sublayers );

	void recalculatePassDrawingOrder();
};

/// Enumeration of possible scene [passes|layers|stages].
/// Ideally, they shouldn't be hardcoded into the engine.
/// NOTE: the order is not important and doesn't influence the rendering order!
/// It's mainly to prevent typing errors when using 'StringHash("ScenePassName")'.
namespace ScenePasses {
/// NOTE: we're using hashes instead of enum values, because they are 'stable'
#define DECLARE_SCENE_PASS( NAME, FILTER )		const NameHash32 NAME = mxHASH_STR(TO_STR(##NAME));
	#include <Rendering/Public/Core/RenderPipeline.inl>
#undef DECLARE_SCENE_PASS
}//namespace ScenePasses

}//namespace Rendering
