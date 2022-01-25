#pragma once

#include <Renderer/Pipelines/TiledDeferred.h>
#include <Renderer/Pipelines/Simple.h>

#include <Utility/DemoFramework/DemoFramework.h>
#include <Utility/DemoFramework/Camera.h>
#include <Utility/Meshok/BSP.h>
#include <Utility/Meshok/BIH.h>
#include <Utility/Meshok/BVH.h>

struct AppSettings : CStruct
{
public:
	mxDECLARE_CLASS(AppSettings, CStruct);
	mxDECLARE_REFLECTION;
	AppSettings()
	{
	}
};

struct App_Test_Chunked_World : Demos::DemoApp
{
	AppSettings			m_settings;

	// Rendering
	DeferredRenderer	m_sceneRenderer;

	MyBatchRenderer		m_batch_renderer;
	PrimitiveBatcher	m_prim_batcher;

	AxisArrowGeometry	m_gizmo_renderer;


	TPtr<RrMaterial>	m_voxelMaterial;




	// Scene management
	Clump			m_runtime_clump;
	FlyingCamera	camera;
	TPtr< RrLocalLight >	m_torchlight;
	//TPtr< RrMaterialSet >	m_terrainMaterials;


	TMovingAverageN< 128 >	m_FPS_counter;
	TempAllocator1024	m_SDF_alloc;

public:
	virtual ERet Initialize( const EngineSettings& _settings ) override;
	virtual void Shutdown() override;
	virtual void Tick( const InputState& inputState ) override;
	virtual ERet Draw( const InputState& inputState ) override;
};
