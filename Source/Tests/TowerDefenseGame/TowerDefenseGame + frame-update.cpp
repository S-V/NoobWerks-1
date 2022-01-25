#include <Base/Base.h>
#pragma once

#include <Base/Template/Containers/Array/TLocalArray.h>
#include <Core/Client.h>
#include <Core/ConsoleVariable.h>
#include <Core/Serialization/Serialization.h>
#include <Core/Tasking/TaskSchedulerInterface.h>
#include <Core/Tasking/SlowTasks.h>
#include <Core/Util/ScopedTimer.h>
#include <Graphics/private/d3d_common.h>
#include <Driver/Driver.h>	// Window stuff
#include <Developer/TypeDatabase/TypeDatabase.h>

#include <Utility/TxTSupport/TxTSerializers.h>

#include <Engine/Engine.h>

#include <Renderer/Core/Material.h>
#include <Renderer/private/RenderSystem.h>
#include <Renderer/Shaders/__shader_globals_common.h>	// G_VoxelTerrain

#include <Utility/Meshok/SDF.h>
#include <Utility/Meshok/BSP.h>
#include <Utility/Meshok/BIH.h>
#include <Utility/Meshok/Noise.h>
#include <Utility/Meshok/Volumes.h>
#include <Utility/MeshLib/Simplification.h>
#include <Utility/Meshok/Octree.h>
#include <Utility/TxTSupport/TxTConfig.h>
#include <Utility/VoxelEngine/VoxelizedMesh.h>

#include <Developer/RunTimeCompiledCpp.h>

#include "RCCpp_WeaponBehavior.h"

#include "TowerDefenseGame.h"


//#if VOXEL_TERRAIN5_WITH_PHYSICS
//void updateGraphicsTransformsFromPhysics( ACullingSystem* culler, const Clump& scene_data )
//{
//	Clump::Iterator< Physics::RigidBody >	rigidBodyIt( scene_data );
//	while( rigidBodyIt.isValid() )
//	{
//		Physics::RigidBody & rigidBody = rigidBodyIt.Value();
//
//		const btRigidBody& bulletRB = rigidBody.GetRigidBody();
//
//		if( bulletRB.isActive() )
//		{
//			const btTransform& btXform = bulletRB.getCenterOfMassTransform();
//
//			// 
//			btVector3	aabbMin, aabbMax;
//			bulletRB.getAabb( aabbMin, aabbMax );
//
//			const AABBf model_aabb_world = { fromBulletVec(aabbMin), fromBulletVec(aabbMax) };
//			culler->updateBoundsWorld( rigidBody._culling_proxy, model_aabb_world );
//		}
//
//		rigidBodyIt.MoveToNext();
//	}
//}
//#endif


void TowerDefenseGame::Tick( const InputState& input_state )
{
	rmt_ScopedCPUSample(Tick, 0);
	PROFILE;

#if ENABLE_HOT_RELOADING_OF_SCENE_SCRIPT
	if(AtomicLoad(m_sceneScriptChanged))
	{
		AtomicExchange(&m_sceneScriptChanged, 0);

		this->rebuildBlobTreeFromLuaScript();
	}
#endif

	SlowTasks::Tick();

	_render_system->update( input_state.frameNumber, input_state.totalSeconds * 1000 );

	//
	DemoApp::Tick(input_state);

	//
	if( _game_state_manager.CurrentState() == GameStates::main ) {
		camera.processInput(input_state);
	}

	this->_processInput( input_state );

	camera.Update(input_state.delta_time_seconds);

	//

	const M44f camera_world_matrix = M44_OrthoInverse( camera.BuildViewMatrix() );

	_game_player.update(
		input_state
		, camera_world_matrix
		, m_settings.renderer
		, Rendering::NwRenderSystem::Get().m_debug_line_renderer
		);


	//
#if TB_TEST_USE_SOUND_ENGINE
	_sound_engine.UpdateListener(
		camera.m_eyePosition
		, CV3f(0)mxTODO("velocity")
		, camera.m_lookDirection
		, camera.m_upDirection
		);
	_sound_engine.UpdateOncePerFrame();
#endif // #if TB_TEST_USE_SOUND_ENGINE


#if VOXEL_TERRAIN5_WITH_PHYSICS

	#if VOXEL_TERRAIN5_WITH_PHYSICS_CHARACTER_CONTROLLER
		if(camera.m_movementFlags)
		{
			_player_controller.getRigidBody()->applyCentralImpulse(toBulletVec(camera.m_linearVelocity * 10.f));
		}
	#endif

	#if VOXEL_TERRAIN5_WITH_PHYSICS_CHARACTER_CONTROLLER
		_player_controller.prePhysicsStep(input_state.delta_time_seconds);
	#endif

		if( !TEST_BIT( m_settings.physics.flags, tbPhysicsSystemFlags::dbg_pause_physics_simulation ) )
		{
			_physics_world.Update( input_state.delta_time_seconds, m_runtime_clump );
		}

	#if VOXEL_TERRAIN5_WITH_PHYSICS_CHARACTER_CONTROLLER
		_player_controller.postPhysicsStep(input_state.delta_time_seconds);
	#endif


		//updateGraphicsTransformsFromPhysics( _scene_renderer.m_cullingSystem, m_runtime_clump );


	#if VOXEL_TERRAIN5_WITH_PHYSICS_CHARACTER_CONTROLLER
		if( m_settings.physics.enable_player_collisions )
		{
			camera.m_eyePosition = _player_controller.getPos();
		}
		else
		{
			_player_controller.setPos(camera.m_eyePosition);
					btTransform	t;
			t.setIdentity();
			t.setOrigin(toBulletVec(camera.m_eyePosition));
			_player_controller.getRigidBody()->setCenterOfMassTransform(t);		
		}
	#endif

#endif // VOXEL_TERRAIN5_WITH_PHYSICS


	//_bt_manager.tick(
	//	input_state.delta_time_seconds
	//	);

	_worldState.update(
		input_state
		, m_settings.renderer
		, _render_system->m_debugRenderer
		);

	//
	const V3d region_of_interest_position = m_settings.use_fake_camera
		? V3d::fromXYZ( m_settings.fake_camera.origin )
		: getCameraPosition()
		;

	_voxel_world.updateOncePerFrame(
		region_of_interest_position,
		input_state.frameNumber,
		this
		);

	//
	_render_world->update(
		V3d::fromXYZ( region_of_interest_position )
		, microsecondsToSeconds( Testbed::gameTimeElapsed() )
		);
	//_render_world->setGlobalShadowConfig( _render_system->_global_shadow );
	//_render_system->_global_shadow.dirty_cascades = 0;

	//

#if VOXEL_TERRAIN5_WITH_PHYSICS

	const V3f rayFrom = camera.m_eyePosition;
	const V3f rayDir = camera.m_lookDirection;
	const btVector3 rayFromWorld( rayFrom.x, rayFrom.y, rayFrom.z );

	const V3f rayTo = rayFrom + rayDir * m_settings.phys_max_raycast_distance_for_voxel_tool;
	const btVector3 rayToWorld(  rayTo.x, rayTo.y, rayTo.z );

	btCollisionWorld::ClosestRayResultCallback	resultCallback( rayFromWorld, rayToWorld );
	_physics_world.m_dynamicsWorld.rayTest( rayFromWorld, rayToWorld, resultCallback );


	_voxel_tool_raycast_hit_anything = resultCallback.hasHit();

	if( resultCallback.hasHit() )
	{
		const V3f hitPoint = fromBulletVec( resultCallback.m_hitPointWorld );
		//LogStream(LL_Warn)<<"Hit: ",hitPoint;
		_last_pos_where_voxel_tool_raycast_hit_world = hitPoint;
	}

#else // !VOXEL_TERRAIN5_WITH_PHYSICS

	_last_pos_where_voxel_tool_raycast_hit_world = camera.m_eyePosition;
	_voxel_tool_raycast_hit_anything = true;

#endif // !VOXEL_TERRAIN5_WITH_PHYSICS







		//if( NwPlayerWeapon* current_weapon = _game_player.currentWeapon() )
		//{
		//	current_weapon->behavior->updateWeapon( *current_weapon );
		//}




#if ENGINE_CONFIG_ENABLE_RUN_TIME_COMPILED_CPP

	RunTimeCompiledCpp::update( input_state.delta_time_seconds );

#endif // ENGINE_CONFIG_ENABLE_RUN_TIME_COMPILED_CPP

}

void TowerDefenseGame::VX_onCameraPositionChanged(
	const UInt3 previous_chunk_coord
	, const UInt3 current_chunk_coord
	)
{
	LogStream(LL_Warn),"VX_onCameraPositionChanged: ",previous_chunk_coord," -> ",current_chunk_coord;

	//Rendering::VoxelConeTracing::Get().modify( Rendering::g_settings. );

	//
}
