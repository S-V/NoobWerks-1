#pragma once

//
class NwMesh;
class ARenderWorld;
class TbRenderSystemI;
struct RrRuntimeSettings;

struct NwAnimClip;
struct NwSkinnedMesh;
struct NwAnimatedModel;
class NwAnimPlayer;
struct NwAnimEventParameter;
class NwDecalsSystem;

//
struct NwTime;

struct NwWeaponDef;

struct NwEngineSettings;

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

struct MyGameSettings;
struct MyGameControlsSettings;
struct GameCheatsSettings;
class MyGamePlayer;

class NwStar;
class FPSGame;
class MyGameWorld;
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


namespace VX5
{
	class WorldI;
}


namespace SDF
{
	class Tape;
}
