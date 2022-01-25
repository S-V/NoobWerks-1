#include <Lua/lua.hpp>
#include <LuaBridge/LuaBridge.h>

#include <Base/Base.h>
#include <Base/Template/Containers/Blob.h>
#include <Base/Template/Containers/BitSet/BitArray.h>
#include <Base/Text/String.h>
#include <Base/Math/ViewFrustum.h>

#include <bx/bx.h>	// uint8_t

//#include <libnoise/noise.h>
//#pragma comment( lib, "libnoise.lib" )

#include <Base/Template/Containers/Array/TLocalArray.h>
#include <Core/Client.h>
#include <Core/ConsoleVariable.h>
#include <Core/Serialization/Serialization.h>
#include <Core/Tasking/TaskSchedulerInterface.h>
#include <Core/Tasking/SlowTasks.h>
#include <Core/Util/ScopedTimer.h>
#include <Graphics/private/d3d_common.h>
#include <Driver/Driver.h>	// Window stuff

#include <Renderer/Renderer.h>
#include <Renderer/private/RenderSystem.h>

#include "TowerDefenseGame.h"
#include "RCCpp_WeaponBehavior.h"

void TowerDefenseGame::_processInput( const InputState& input_state )
{
	const bool isInGameState = ( _game_state_manager.CurrentState() == GameStates::main );

	const bool leftMouseButtonPressed = (input_state.mouse.held_buttons_mask & BIT(MouseButton_Left));
	const bool rightMouseButtonPressed = (input_state.mouse.held_buttons_mask & BIT(MouseButton_Right));

	if( input_state.keyboard.held[KEY_Pause] )
	{
		_worldState.isPaused ^= 1;

		Engine::setGamePaused( _worldState.isPaused );

		if( _worldState.isPaused ) {
			SET_BITS( m_settings.physics.flags.raw_value, tbPhysicsSystemFlags::dbg_pause_physics_simulation );
		} else {
			CLEAR_BITS( m_settings.physics.flags.raw_value, tbPhysicsSystemFlags::dbg_pause_physics_simulation );
		}
	}

	//
	if( _editor_manipulator.isGizmoEnabled() )
	{
		if( GizmoOperation::handle_InputEvent_to_change_GizmoOperation(
			&m_settings.voxel_editor_tools.gizmo_operation
			, input_state
			) )
		{
			return;
		}

		if( input_state.modifiers & KeyModifier_Ctrl )
		{
			if( input_state.keyboard.held[KEY_Enter] )
			{
				_voxel_world.addMesh(
					_stamped_mesh
					, _editor_manipulator.getTransformMatrix()
					, VoxMat_Snow
					);
				return;
			}
		}
	}




#if VOXEL_TERRAIN5_WITH_PHYSICS
	if( input_state.mouse.held_buttons_mask & MouseButton_Left )
	{
		//
	}

	if( input_state.keyboard.held[KEY_P] )
	{

#if DRAW_SOLDIER

		btRigidBody & rb = _worldState._npc_marine._rigidBody.bt_rb();

		btTransform t = rb.getCenterOfMassTransform();
		t.setOrigin( toBulletVec(camera.m_eyePosition) );

		rb.setCenterOfMassTransform(t);
		rb.setLinearVelocity(toBulletVec(CV3f(0)));
		rb.setAngularVelocity(toBulletVec(CV3f(0)));

		//
//		_sound_engine.playSound3D(AssetID());

#endif

	}

	if( input_state.keyboard.held[KEY_Q] )
	{
		_worldState.spawn_NPC_Spider(
			camera.m_eyePosition
			, _physics_world
			);
	}

#endif





	if( isInGameState )
	{
		if( input_state.keyboard.held[KEY_1] )
		{
			_game_player.selectWeapon( PLAYER_WEAPON_ID_GRENADE_LAUNCHER );
		}
		if( input_state.keyboard.held[KEY_2] )
		{
			_game_player.selectWeapon( PLAYER_WEAPON_ID_PLASMAGUN );
		}

		if( NwPlayerWeapon* current_weapon = _game_player.currentWeapon() )
		{
			if( leftMouseButtonPressed )
			{
				current_weapon->primaryAttack();
			}

			if( input_state.keyboard.held[KEY_R] )
			{
				current_weapon->reload();
			}
		}
	}


#if vx5_CFG_DEBUG_MESHING

	if( isInGameState )
	{
		if( leftMouseButtonPressed )
		{
			static U32 last_time_pressed;
			const U32 current_time = mxGetTimeInMilliseconds();

			if( current_time - last_time_pressed > 500 )
			{
				DBGOUT("");
				last_time_pressed = current_time;

				//
				V3d ray_dir = V3d::fromXYZ(camera.m_lookDirection);
				ray_dir.normalize();

				_voxel_world.dbg_castRay_and_displayDebugInfo(
					V3d::fromXYZ(camera.m_eyePosition)
					, ray_dir
					, m_dbgView
					);
			}
		}

		if( rightMouseButtonPressed )
		{
			DBGOUT("");
		}
	}

#endif // vx5_CFG_DEBUG_MESHING








	const V3f region_of_interest_position = m_settings.use_fake_camera ? m_settings.fake_camera.origin : camera.m_eyePosition;


#pragma region "CSG operations"
	if( isInGameState && !input_state.modifiers )
	{
		VX::BrushParams	brush;
		brush.center_in_world_space = camera.m_eyePosition;

		//
		if( input_state.keyboard.held[KEY_F] )
		{
			brush.from_CSG_Subtract( m_settings.dev_voxel_tools.small_geomod_sphere );
			_voxel_world.editWithBrush( brush );
		}
		if( input_state.keyboard.held[KEY_G] )
		{
			brush.from_CSG_Subtract( m_settings.dev_voxel_tools.large_geomod_sphere );
			_voxel_world.editWithBrush( brush );
		}
		if( input_state.keyboard.held[KEY_U] )
		{
			brush.from_CSG_Add( m_settings.dev_voxel_tools.large_add_sphere );
			_voxel_world.editWithBrush( brush );
		}

		//
		if( input_state.keyboard.held[KEY_V] )
		{
			brush.from_CSG_Add( m_settings.dev_voxel_tools.small_add_sphere );
			_voxel_world.editWithBrush( brush );
		}

		if( input_state.keyboard.held[KEY_B] )
		{
			brush.from_CSG_Add( m_settings.dev_voxel_tools.small_add_cube );
			_voxel_world.editWithBrush( brush );
		}
		if( input_state.keyboard.held[KEY_N] )
		{
			brush.from_CSG_Subtract( m_settings.dev_voxel_tools.small_geomod_cube );
			_voxel_world.editWithBrush( brush );
		}

		if( input_state.keyboard.held[KEY_Q] )
		{
			brush.from_Tool_Smoothing( m_settings.dev_voxel_tools.smooth );
			_voxel_world.editWithBrush( brush );
		}

		if( input_state.keyboard.held[KEY_T] )
		{
			brush.from_Tool_Painbrush( m_settings.dev_voxel_tools.paint_material );;
			_voxel_world.editWithBrush( brush );
		}

		////
		//if( input_state.keyboard.held[KEY_Semicolon] )
		//{
		//	brush.center = region_of_interest_position;

		//	const float chunk_size0 = VX5::getChunkSize(VX::ChunkID::MIN_LOD);
		//	//const float dist_between_csg_ops = chunk_size0 * 0.3f;
		//	//const float radius_of_csg_op = chunk_size0 * 0.5f;

		//	//const float lod0_radius = _voxel_world._settings.lod.split_distance_factor * 0.8f;
		//
		//	//const int num_csg_ops = 

		//	brush.radius = chunk_size0 * _voxel_world._settings.lod.selection.split_distance_factor * 1.9f;
		//	brush.editMode = VX::BrushEdit_Add;
		//	brush.type = VX::VoxelBrushShape_Cube;
		//	brush.material = VoxMat_Floor;

		//	_voxel_world.editWithBrush( brush );
		//}



#if 0
		if( input_state.buttons.held[KEY_P] )
		{
			const char* sourceMeshFileName = "D:/research/__test/meshes/arch/acropolis.x";
			//const char* sourceMeshFileName = "D:/research/__test/meshes/bolt.obj";
			//const char* sourceMeshFileName = "D:/research/__test/meshes/unit_cube.stl";
			//const char* sourceMeshFileName = "D:/research/__test/meshes/rotated_cube_z.obj";
			//const char* sourceMeshFileName = "D:/research/__test/meshes/rotated_cube1.obj";	// GOOD for showcasing distant LoD gen/simplified geom
			//const char* sourceMeshFileName = "D:/research/__test/meshes/fandisk_lite.obj";

			TRefPtr< VX::VoxelizedMesh >	model;
			if(mxSUCCEDED(this->loadOrCreateModel( model, sourceMeshFileName )))
			{
				// scale model from [-1..+1] to [-viewRadiusLoD0..+viewRadiusLoD0]
				//const float modelScale = _voxel_world._settings.lod.split_distance_factor * VX5::getChunkSize(VX::ChunkID::MIN_LOD) * 0.3f;
				//const float modelScale = _voxel_world._settings.lod.split_distance_factor * VX5::getChunkSize(VX::ChunkID::MIN_LOD) * 0.9f;
				//float modelScale = _voxel_world._settings.lod.split_distance_factor * VX5::getChunkSize(VX::ChunkID::MIN_LOD) * 1.5f;
				const float modelScale = _voxel_world._settings.lod.selection.split_distance_factor * VX5::getChunkSize(VX::ChunkID::MIN_LOD) * 1.9f;

				VX::MeshTransform	modelTransform;
				modelTransform.translation = region_of_interest_position;
				modelTransform.scaling = modelScale;

				const AABBf modelBoundsWorld = AABBf::fromSphere( modelTransform.translation, modelTransform.scaling );
				_voxel_world.stampMesh( model, modelBoundsWorld, VoxMat_Snow2 );
			}
		}
#endif


#if 0
		if( input_state.buttons.held[KEY_O] )
		{
			const char* sourceMeshFileName = "D:/research/__test/meshes/stanford/bunny_40k.obj";

			TRefPtr< VX::VoxelizedMesh >	model;
			if(mxSUCCEDED(this->loadOrCreateModel( model, sourceMeshFileName )))
			{
				// scale model from [-1..+1] to [-viewRadiusLoD0..+viewRadiusLoD0]
				//const float modelScale = _voxel_world._settings.lod.split_distance_factor * VX5::getChunkSize(VX::ChunkID::MIN_LOD) * 0.3f;
				//const float modelScale = _voxel_world._settings.lod.split_distance_factor * VX5::getChunkSize(VX::ChunkID::MIN_LOD) * 0.9f;
				//float modelScale = _voxel_world._settings.lod.split_distance_factor * VX5::getChunkSize(VX::ChunkID::MIN_LOD) * 1.5f;
				const float modelScale = _voxel_world._settings.lod.selection.split_distance_factor * VX5::getChunkSize(VX::ChunkID::MIN_LOD) * 1.9f;

				VX::MeshTransform	modelTransform;
				modelTransform.translation = region_of_interest_position;
				modelTransform.scaling = modelScale;

				const AABBf modelBoundsWorld = AABBf::fromSphere( modelTransform.translation, modelTransform.scaling );
				_voxel_world.stampMesh( model, modelBoundsWorld, VoxMat_Snow2 );
			}
		}

		if( input_state.buttons.held[KEY_I] )
		{
			const char* sourceMeshFileName = "D:/research/__test/meshes/fandisk_lite.obj";

			TRefPtr< VX::VoxelizedMesh >	model;
			if(mxSUCCEDED(this->loadOrCreateModel( model, sourceMeshFileName )))
			{
				// scale model from [-1..+1] to [-viewRadiusLoD0..+viewRadiusLoD0]
				//const float modelScale = _voxel_world._settings.lod.split_distance_factor * VX5::getChunkSize(VX::ChunkID::MIN_LOD) * 0.3f;
				//const float modelScale = _voxel_world._settings.lod.split_distance_factor * VX5::getChunkSize(VX::ChunkID::MIN_LOD) * 0.9f;
				//float modelScale = _voxel_world._settings.lod.split_distance_factor * VX5::getChunkSize(VX::ChunkID::MIN_LOD) * 1.5f;
				const float modelScale = _voxel_world._settings.lod.selection.split_distance_factor * VX5::getChunkSize(VX::ChunkID::MIN_LOD) * 1.5f;

				VX::MeshTransform	modelTransform;
				modelTransform.translation = region_of_interest_position;
				modelTransform.scaling = modelScale;

				const AABBf modelBoundsWorld = AABBf::fromSphere( modelTransform.translation, modelTransform.scaling );
				_voxel_world.stampMesh( model, modelBoundsWorld, VoxMat_Snow2 );
			}
		}
#endif

#pragma region "CSG hole"
		if( input_state.buttons.held[KEY_7] )
		{
			GameplayConstants::gs_shared.sphere_hole_pos_in_world_space.x -= 1;
		}
		if( input_state.buttons.held[KEY_8] )
		{
			GameplayConstants::gs_shared.sphere_hole_pos_in_world_space.x += 1;
		}
		if( input_state.buttons.held[KEY_9] )
		{
			GameplayConstants::gs_shared.sphere_hole_pos_in_world_space.y -= 1;
		}
		if( input_state.buttons.held[KEY_0] )
		{
			GameplayConstants::gs_shared.sphere_hole_pos_in_world_space.y += 1;
		}
		if( input_state.buttons.held[KEY_5] )
		{
			GameplayConstants::gs_shared.sphere_hole_pos_in_world_space.z -= 1;
		}
		if( input_state.buttons.held[KEY_6] )
		{
			GameplayConstants::gs_shared.sphere_hole_pos_in_world_space.z += 1;
		}
#pragma endregion













		if( input_state.modifiers & KeyModifier_Shift )
		{
			if( input_state.keyboard.held[KEY_1] )
			{
				dbg_showChunkOctree();
			}
			if( input_state.keyboard.held[KEY_2] )
			{
				UNDONE;
				//dbg_showStitching();
			}
			if( input_state.keyboard.held[KEY_0] )
			{
				dbg_showSeamOctreeBounds();
			}
			if( input_state.keyboard.held[KEY_B] )
			{
				implicit::debugDrawBoundingBoxes(
					m_blobTree,
					AABBf::make(
						V3f::fromXYZ( _voxel_world._octree._cube.minCorner() ),
						V3f::fromXYZ( _voxel_world._octree._cube.maxCorner() )
					),
					m_dbgView
					);
			}
		}



		if( !input_state.modifiers )
		{
			if( input_state.keyboard.held[KEY_Z] )
			{
				dbg_showChunkOctree();
			}
			if( input_state.keyboard.held[KEY_X] )
			{
				UNDONE;
				//dbg_showStitching();
			}
			if( input_state.keyboard.held[KEY_Slash] )
			{
				dbg_remesh_chunk();
			}

			//if( input_state.keyboard.held[KEY_Y] )
			//{
			//	dbg_updateGhostCellsAndRemesh();
			//}
		}
	}
#pragma endregion













#if 0&&VOXEL_TERRAIN5_WITH_PHYSICS

		if( isInGameState
			&& (leftMouseButtonPressed || rightMouseButtonPressed) )
		{
#if 0
			this->_applyEditingOnLeftMouseButtonClick( m_settings.voxel_tool, input_state );
#else
			const V3f rayFrom = camera.m_eyePosition;
			const V3f rayDir = camera.m_lookDirection;

			//const VX::ChunkedWorld::RayCastResult result = m_world.CastRay_Approx( rayFrom, rayDir );

			//const btVector3 rayFromWorld( rayFrom.x, rayFrom.y, rayFrom.z );
			//const V3f rayTo = rayFrom + rayDir * 9999.0f;
			//const btVector3 rayToWorld(  rayTo.x, rayTo.y, rayTo.z );

			const btVector3 rayFromWorld( rayFrom.x, rayFrom.y, rayFrom.z );
			const V3f rayTo = rayFrom + rayDir * 9999.0f;
			const btVector3 rayToWorld(  rayTo.x, rayTo.y, rayTo.z );

			btCollisionWorld::ClosestRayResultCallback	resultCallback( rayFromWorld, rayToWorld );
			_physics_world.m_dynamicsWorld.rayTest( rayFromWorld, rayToWorld, resultCallback );

			if( resultCallback.hasHit() )
			{
				const V3f hitPoint = V3f::set( resultCallback.m_hitPointWorld.x(), resultCallback.m_hitPointWorld.y(), resultCallback.m_hitPointWorld.z() );
				LogStream(LL_Warn)<<"Hit: ",hitPoint;


				if( leftMouseButtonPressed )
				{
m_dbgView.addSphere(hitPoint,1,RGBAf::RED);
					_worldState.test_set_point_to_attack_for_friendly_NPCs( hitPoint );
				}
				else
				{
m_dbgView.addSphere(hitPoint,1,RGBAf::BLACK);
					_worldState.test_set_destination_point_to_move_friendly_NPCs( hitPoint );
				}




				//float blast_radius = PHYSICS_FOR_DEMO_RELEASE ? smallCsgRadius : tinyCsgRadius;

				////m_dbgView.addPoint(hitPoint, RGBAf::RED, 4);
				////m_dbgView.addSphere(hitPoint,blast_radius,RGBAf::RED);

				//brush.center = hitPoint;
				//brush.radius = blast_radius;
				//brush.editMode = VX::BrushEdit_Remove;
				//brush.type = VX::VoxelBrushShape_Sphere;
				////brush.type = VX::VoxelBrushShape_Cube;

				//_world.editWithBrush( brush );
			}
#endif
		}

#if 0
		//if( input_state.mouse.held_buttons_mask & BIT(MouseButton_Left) )
		if( input_state.keyboard.held[KEY_Y] )
		{
			float radius = chunkSizeAtLoD0*0.5;
			float mass = cubef(radius) * 10;
			const V3f initialPos = camera.m_eyePosition + V3_Scale( camera.m_lookDirection, radius * 2 );
			const M44f	initialTransform = M44_BuildTS( initialPos, radius );
			const V3f initialVelocity = camera.m_lookDirection * 40.0f;

			TPtr< NwMesh >			unit_sphere_mesh;
			mxDO(Resources::Load(unit_sphere_mesh.Ptr
				//,MakeAssetID("unit_sphere_ico")
				,MakeAssetID("unit_cube")
				,&m_runtime_clump));

			RrMeshInstance *	renderModel;
			mxDO(RrMeshInstance::CreateFromMesh(
				renderModel,
				unit_sphere_mesh,
				M44_buildTranslationMatrix(initialPos),
				m_runtime_clump
				));

			renderModel->_transform->setLocalToWorldMatrix( initialTransform );

			NwMaterial* renderMat = nil;
			mxDO(Resources::Load(renderMat, MakeAssetID("material_plastic"), &m_runtime_clump));
			renderModel->_materials[0] = renderMat;

			//
			const AABBf model_aabb_world = AABBf::fromSphere(initialPos, radius);
			renderModel->m_proxy = _render_world->addEntity( renderModel, RE_RigidModel, model_aabb_world );

			//
			//_physics_world.CreateRigid_Sphere( initialPos, radius, renderModel->_transform, renderModel->m_proxy
			//	, m_runtime_clump, mass, initialVelocity );

			const CV3f collision_box_shape_half_size(radius*0.4f, radius*0.1f, radius);

			m_physicsWorld.spawnRigidBox( initialPos
				, collision_box_shape_half_size
				, renderModel->_transform, renderModel->m_proxy
				, m_runtime_clump, mass, initialVelocity
				, M44_scaling( collision_box_shape_half_size ) );
		}



		if( isInGameState && (input_state.mouse.buttons & BIT(MouseButton_Right)) )
		{
			float radius = chunkSizeAtLoD0*0.06;
			float mass = cubef(radius) * 10;

			const V3f initialPos = camera.m_eyePosition + V3_Scale( camera.m_lookDirection, radius * 2 );
			const M44f	initialTransform = M44_BuildTS( initialPos, radius );
			const V3f initialVelocity = camera.m_lookDirection * 50.0f;

			TPtr< NwMesh >			unit_sphere_mesh;
			mxDO(Resources::Load(unit_sphere_mesh.Ptr
				,MakeAssetID("unit_sphere_ico")
				,&m_runtime_clump));

			RrMeshInstance *	renderModel;
			mxDO(RrMeshInstance::CreateFromMesh(
				renderModel,
				unit_sphere_mesh,
				M44_buildTranslationMatrix(initialPos),
				m_runtime_clump
				));

			renderModel->_transform->setLocalToWorldMatrix( initialTransform );

			NwMaterial* renderMat = nil;
			mxDO(Resources::Load(renderMat, MakeAssetID("material_plastic"), &m_runtime_clump));
			renderModel->_materials[0] = renderMat;

			//
			const AABBf model_aabb_world = AABBf::fromSphere(initialPos, radius);
			renderModel->m_proxy = _render_world->addEntity( renderModel, RE_RigidModel, model_aabb_world );

			//
			m_physicsWorld.CreateRigid_Sphere( initialPos, radius, renderModel->_transform, renderModel->m_proxy
				, m_runtime_clump, mass, initialVelocity );
		}
#endif
#endif


}
