// Voxel terrain tools framework: high-level tool settings and interfaces.
// TODO: steal names and comments from:
// https://github.com/runedegroot/UE4VoxelTerrainEditor/blob/master/Source/VoxelTerrainRenderer/Private/VoxelTerrainSculptTool.cpp
#pragma once

#include <VoxelsSupport/VoxelTerrainRenderer.h>	// VoxMatID

namespace VX
{

///
struct VoxelToolType
{
	enum Enum
	{
		//== Sculpting

		// working with brushes (simple shapes):
		CSG_Add_Shape,		// Additive Brush
		CSG_Subtract_Shape,	// Subtractive Brush

		// working with (closed) meshes:
		CSG_Add_Model,
		CSG_Subtract_Model,

		//== volume_material painting
		Pick_Material,
		Paint_Brush,
	
		/// use a polygonal model as a paintbrush
		Paint_Brush_Model,


		// smoothing
		SmoothGeometry,			// Smoothing Brush
		//SmoothMaterialTransitions,
		SharpenEdges,	// removes noise, recovers sharp edges and corners

		Grow,
		Shrink,

		//== Voxel Clipboard
		Copy,
		Cut,
		Paste,

		COUNT,
	};
};
mxDECLARE_ENUM( VoxelToolType::Enum, U8, VoxelTool8 );




#if 0

typedef VoxMatID MaterialIndex;
static const MaterialIndex EMPTY_MATERIAL_ID = VoxMat_None;

#else

///NOTE: right now, the engine only supports 8-bit volume_material IDs.
typedef U8 MaterialIndex;

/// empty space/air
static const MaterialIndex EMPTY_MATERIAL_ID = 0;

#endif




///
enum EVoxelBrushShapeType
{
	VoxelBrushShape_Sphere,
	VoxelBrushShape_Cube,
};
mxDECLARE_ENUM( EVoxelBrushShapeType, U8, VoxelBrushShapeType );

/// The base class for all voxel tool settings.
struct SVoxelToolSettingsBase: CStruct
{
	//float	scale;
	//float	distance_along_view_direction;

public:
	mxDECLARE_CLASS(SVoxelToolSettingsBase, CStruct);
	mxDECLARE_REFLECTION;
	SVoxelToolSettingsBase()
	{
		//scale = 1.0f;
		//scale = VX4::CHUNK_SIZE * 0.5f;
		//distance_along_view_direction = 0.0f;
	}
};

///
struct SVoxelToolSettings_ShapeBrush: SVoxelToolSettingsBase
{
	VoxelBrushShapeType		brush_shape_type;

	/// half-size for cube
	float					brush_radius_in_world_units;

public:
	mxDECLARE_CLASS(SVoxelToolSettings_ShapeBrush, SVoxelToolSettingsBase);
	mxDECLARE_REFLECTION;
	SVoxelToolSettings_ShapeBrush()
	{
		brush_shape_type = VoxelBrushShape_Sphere;
		brush_radius_in_world_units = 1.0f;
	}
};




///
struct SVoxelToolSettings_CSG_Subtract: SVoxelToolSettings_ShapeBrush
{
	/// overrides the volume_material of mesh triangles
	VoxMatID	custom_surface_material;

public:
	mxDECLARE_CLASS(SVoxelToolSettings_CSG_Subtract, SVoxelToolSettings_ShapeBrush);
	mxDECLARE_REFLECTION;
	SVoxelToolSettings_CSG_Subtract()
	{
		custom_surface_material = VoxMat_None;
	}
};

///
struct SVoxelToolSettings_CSG_Add: SVoxelToolSettings_ShapeBrush
{
	/// must never be zero/EMPTY_SPACE !
	VoxMatID	fill_material;

	/// 'geomod' volume_material
	VoxMatID	custom_surface_material;

public:
	mxDECLARE_CLASS(SVoxelToolSettings_CSG_Add, SVoxelToolSettings_ShapeBrush);
	mxDECLARE_REFLECTION;
	SVoxelToolSettings_CSG_Add()
	{
		fill_material = VoxMat_RockA;
		custom_surface_material = VoxMat_None;
	}
};

///
struct SVoxelToolSettings_Paint_Brush: SVoxelToolSettings_ShapeBrush
{
	/// The volume_material the volume is filled with.
	VoxMatID	fill_material;

	/// The volume_material the surface will be painted with.
	VoxMatID	custom_surface_material;

	///
	bool					paint_surface_only;

public:
	mxDECLARE_CLASS(SVoxelToolSettings_Paint_Brush, SVoxelToolSettings_ShapeBrush);
	mxDECLARE_REFLECTION;
	SVoxelToolSettings_Paint_Brush()
	{
		fill_material = VoxMat_Ground;
		custom_surface_material = VoxMat_None;
		paint_surface_only = false;
	}

	bool isOk() const
	{
		return fill_material != VoxMat_None;
	}
};

///
struct SVoxelToolSettings_Place_Model: SVoxelToolSettingsBase
{
	VoxMatID	fill_material;
	V3f						angles_degrees;

	String128				model_name;	// filename

public:
	mxDECLARE_CLASS(SVoxelToolSettings_Place_Model, SVoxelToolSettingsBase);
	mxDECLARE_REFLECTION;
	SVoxelToolSettings_Place_Model()
	{
		fill_material = VoxMat_Floor;
		angles_degrees = V3f::zero();
	}
};

///
struct SVoxelToolSettings_Subtract_Model: SVoxelToolSettingsBase
{
	VoxMatID	fill_material;
	V3f						angles_degrees;

public:
	mxDECLARE_CLASS(SVoxelToolSettings_Subtract_Model, SVoxelToolSettingsBase);
	mxDECLARE_REFLECTION;
	SVoxelToolSettings_Subtract_Model()
	{
		fill_material = VoxMat_Floor;
		angles_degrees = V3f::zero();
	}
};

///
struct SVoxelToolSettings_SmoothGeometry: SVoxelToolSettingsBase
{
	float	brush_radius_in_world_units;
	float	strength;

	/// so that you can paint on the surface while smoothing sharp corners
	mxTODO("not supported yet!");
	VoxMatID	custom_surface_material;

public:
	mxDECLARE_CLASS(SVoxelToolSettings_SmoothGeometry, SVoxelToolSettingsBase);
	mxDECLARE_REFLECTION;
	SVoxelToolSettings_SmoothGeometry()
	{
		brush_radius_in_world_units = 1;
		strength = 1;
		custom_surface_material = VoxMat_None;
	}
};

///
struct SVoxelToolSettings_All: CStruct
{
	SVoxelToolSettings_CSG_Add			csg_add;
	SVoxelToolSettings_CSG_Subtract		csg_subtract;

	SVoxelToolSettings_Place_Model		csg_add_model;
	SVoxelToolSettings_Subtract_Model	csg_subtract_model;

	SVoxelToolSettings_Paint_Brush		paint_brush;

	VoxelTool8	current_tool;

	//
	U16			snap_to_voxel_grid;	//!< 0 - no snapping, 1 - snap to min-sized voxel
	bool		align_to_surface;

public:
	mxDECLARE_CLASS(SVoxelToolSettings_All, CStruct);
	mxDECLARE_REFLECTION;
	SVoxelToolSettings_All()
	{
		current_tool = VoxelToolType::CSG_Subtract_Shape;

		snap_to_voxel_grid = 0;
		align_to_surface = true;
	}

	void resetAllToDefaultSettings();
};

//////////////////////////////////////////////////////////////////////////

enum EBrushEditMode
{
	BrushEdit_Remove,	//!< CSG Difference/Subtraction
	BrushEdit_Add,		//!< CSG Union/Addition

	//
	BrushEdit_Paint,	//!< Overwrite solid portions with a custom volume_material

	//
	BrushEdit_Smooth,	//!< smoothen sharp corners
};

enum EBrushAlignment
{
	BrushAlign_Center,
	BrushAlign_ToSurface,
};

///
struct BrushParams
{
	V3f					center_in_world_space;
	float				radius_in_world_units;	//!< half-size

	VX::EBrushEditMode			edit_mode;
	VX::EVoxelBrushShapeType	shape_type;

	float	strength;	//!< [0..1] for smoothing

	/// if op is Paintbrush:
	/// sets the solid portions of the volume to this volume_material (volume_material 0 - undefined).
	///
	/// if op is CSG subtract:
	/// the created surface will have this volume_material (pass 0 to take the original volume_material of the volume).
	/// 
	MaterialIndex	volume_material;

	/// 'geomod' volume_material
	MaterialIndex	custom_surface_material;

public:
	BrushParams()
	{
		edit_mode = VX::BrushEdit_Remove;
		shape_type = VX::VoxelBrushShape_Sphere;
		center_in_world_space = CV3f(0);
		radius_in_world_units = 0;
		strength = 0.5f;
		volume_material = EMPTY_MATERIAL_ID;
		custom_surface_material = EMPTY_MATERIAL_ID;
	}

	bool skipEmptySpace() const
	{
		return edit_mode != BrushEdit_Add;
	}

	const AABBd getAabbInWorldSpace() const
	{
		return AABBd::fromSphere(
			V3d::fromXYZ(center_in_world_space), radius_in_world_units
			);
	}

public:
	void from_CSG_Subtract(
		const SVoxelToolSettings_CSG_Subtract& csg_brush
		)
	{
		this->radius_in_world_units = csg_brush.brush_radius_in_world_units;
		this->edit_mode = VX::BrushEdit_Remove;
		this->shape_type = csg_brush.brush_shape_type;
		this->volume_material = csg_brush.custom_surface_material;
		this->custom_surface_material = csg_brush.custom_surface_material;
	}

	void from_CSG_Add(
		const SVoxelToolSettings_CSG_Add& tool_csg_add
		)
	{
		this->radius_in_world_units = tool_csg_add.brush_radius_in_world_units;
		this->edit_mode = VX::BrushEdit_Add;
		this->shape_type = tool_csg_add.brush_shape_type;
		this->volume_material = tool_csg_add.fill_material;
		this->custom_surface_material = tool_csg_add.custom_surface_material;
	}

	void from_Tool_Smoothing(
		const SVoxelToolSettings_SmoothGeometry& smoothing
		)
	{
		this->radius_in_world_units = smoothing.brush_radius_in_world_units;
		this->edit_mode = VX::BrushEdit_Smooth;
		this->shape_type = VoxelBrushShape_Sphere;
		this->volume_material = EMPTY_MATERIAL_ID;
		this->custom_surface_material = smoothing.custom_surface_material;
	}

	void from_Tool_Painbrush(
		const SVoxelToolSettings_Paint_Brush& painbrush
		)
	{
		this->radius_in_world_units = painbrush.brush_radius_in_world_units;
		this->edit_mode = VX::BrushEdit_Paint;
		this->shape_type = painbrush.brush_shape_type;
		this->volume_material = painbrush.fill_material;
		this->custom_surface_material = painbrush.custom_surface_material;
	}
};

}//namespace VX
