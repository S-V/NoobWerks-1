#pragma once


/// scale the world down so that we can simulate large game scenes
/// without losing precision too much
static const float GAME_TO_METERS = 0.001f;
static const float METERS_TO_GAME = 1.f/GAME_TO_METERS;

/// can be made 1e-4f
static const float MIN_NEAR_CLIP_PLANE_DIST = 0.001f;


//


// Core
struct SimdAabb;
struct NwTime;
struct NwEngineSettings;
class NwHandleTable;
class NwJobSchedulerI;

#include <Core/Tasking/TaskSchedulerInterface.h>



// Rendering

#include <Renderer/ForwardDecls.h>

class NwMesh;
class SpatialDatabaseI;
class NwRenderSystemI;
struct RrGlobalSettings;

struct NwAnimClip;
struct NwSkinnedMesh;
struct NwAnimatedModel;
class NwAnimPlayer;
struct NwAnimEventParameter;
class NwDecalsSystem;
class TbPrimitiveBatcher;
struct NwModel;

struct SgGameplayRenderData;


// Physics
class NwPhysicsWorld;
struct NwRigidBody;
class btCollisionObject;



// Scripting.
#include <Scripting/ForwardDecls.h>


// In-Game GUI
struct nk_context;



namespace gainput
{
	class InputMap;
	class InputDeviceKeyboard;
	class InputDeviceMouse;
	class InputState;
}


namespace VX5
{
	class WorldI;
}


// Physics
class SgAABBTree;
class SgPhysicsMgr;


// Sound
class NwSoundSystemI;


// Game Settings
struct SgUserSettings;
struct SgAppSettings;
struct MyGameControlsSettings;
struct SgCheatSettings;


// Gameplay
class SgHumanPlayer;

class NwStar;
class SgGameApp;
class SgWorld;
class SgRenderer;
class GameParticleRenderer;

class SgShip;
struct SgAppStates;
class SgGameplayMgr;

struct SgProjectile;
