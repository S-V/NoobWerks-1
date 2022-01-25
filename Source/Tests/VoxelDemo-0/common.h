#pragma once


//
namespace Rendering
{

struct NwCameraView;
struct NwFloatingOrigin;

//
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
}//namespace Rendering
#include <Rendering/ForwardDecls.h>

//
struct NwTime;

struct NwWeaponDef;



// Physics.
class NwPhysicsWorld;
struct NwRigidBody;
class btCollisionObject;

class NwSoundSystemI;

struct NwModel;


// Scripting.
#include <Scripting/ForwardDecls.h>


// In-Game GUI
struct nk_context;

struct MyAppSettings;
struct MyGameControlsSettings;
struct GameCheatsSettings;
class MyGamePlayer;

class NwStar;
class MyApp;
class MyWorld;
class MyGameRenderer;
class GameParticleRenderer;

class EnemyNPC;
struct AI_Blackboard;

struct MyGameStates;


namespace gainput
{
	class InputMap;
	class InputDeviceKeyboard;
	class InputDeviceMouse;
	class InputState;
}


#include <Voxels/public/vx_forward_decls.h>

namespace VX5 {
class OctreeWorld;
}//namespace VX5



namespace SDF
{
	class Tape;
}

#include <Engine/ForwardDecls.h>
