## Skeletal animation system.




### TODO

- OPTIMIZE: use float4x3 matrices both in HLSL and C++ code:
https://github.com/microsoft/DirectXTK/blob/master/Src/Shaders/DGSLEffect.fx

- OPTIMIZE: use milliseconds - ints are faster to compare!
bool NwAnimatedModel::IsAnimDone(
	const NameHash32 anim_name_hash,
	const SecondsF remaining_time_seconds
	) const
{
	const NwAnimClip* anim_clip = _skinned_mesh->findAnimByNameHash( anim_name_hash );
	if(anim_clip) {
		return anim_player.RemainingPlayTime(anim_clip) <= remaining_time_seconds;
	}
	return true;
}

```

#if 0

// Animation/AnimClip/AnimSequence
// A basic key-framed animation - a collection of related animation curves.
// An animation clip contains a set of tracks.
struct rxAnimClip: CStruct
{
	// The 'tracks' array has the same number of entries as 'nodes'.
	TBuffer< rxAnimTrack >	tracks;	// bone tracks - raw animation data
	TBuffer< String >		nodes;	// names of the animated component (e.g. a bone or node)
	String			name;
	float		length;	// duration of the animation in seconds
public:
	mxDECLARE_CLASS(rxAnimClip,CStruct);
	mxDECLARE_REFLECTION;
	rxAnimClip();
	const rxAnimTrack* FindTrackByName( const char* name ) const;
};

// AnimSet is a collection of related animation clips
// (for instance, all animation clips of a character).
struct rxAnimSet: CStruct
{
	TBuffer< rxAnimClip >	animations;	// aka 'animations', 'sequences'
	String					name;		// e.g. 'NPC_Soldier_Anims'
public:
	mxDECLARE_CLASS(rxAnimSet,CStruct);
	mxDECLARE_REFLECTION;
	rxAnimSet();
	const rxAnimClip* FindAnimByName(const char* name) const;
};

// AnimInstance/AnimController
// AnimInstance is used for animating a skeleton instance (Skeleton).
class rxAnimInstance: CStruct
{
public:
	Skeleton *	target;

public:
	mxDECLARE_CLASS(rxAnimInstance,CStruct);
	mxDECLARE_REFLECTION;
	rxAnimInstance();
};

#endif // !TB_USE_OZZ_ANIMATION


//
// animation channels.  make sure to change script/doom_defs.script if you add any channels, or change their order
//
enum EAnimationChannel
{
	 ANIMCHANNEL_ALL			= 0,
	 ANIMCHANNEL_TORSO			= 1,
	 ANIMCHANNEL_LEGS			= 2,
	 ANIMCHANNEL_HEAD			= 3,
	 ANIMCHANNEL_EYELIDS		= 4,
	 ANIMCHANNEL_COUNT};

const int ANIM_MaxAnimsPerChannel	= 3;
const int ANIM_MaxSyncedAnims		= 3;

enum
{
	/// must be at least 3: idle, walk, run
	NUM_MOVEMENT_ANIMS = 2,

	FIRST_CUSTOM_ANIM = NUM_MOVEMENT_ANIMS,

	//// the first NUM_MOVEMENT_ANIMS are reserved!
	//MAX_BLENDED_ANIMS = 4,
	//NUM_CUSTOM_ANIMS = MAX_BLENDED_ANIMS - FIRST_CUSTOM_ANIM,
};

enum ANIM_TYPE
{
	ANIM_IDLE = 0,

	ANIM_WALK,

	ANIM_RUN,

	ANIM_EVADE_LEFT,
	ANIM_EVADE_RIGHT,

	ANIM_BELLY_PAIN,

	ANIM_ATTACK,


	ANIM_MAX_COUNT
};


```



### References:

https://blog.demofox.org/2012/09/21/anatomy-of-a-skeletal-animation-system-part-1/
https://blog.demofox.org/2012/09/21/anatomy-of-a-skeletal-animation-system-part-2/
https://blog.demofox.org/2012/09/21/anatomy-of-a-skeletal-animation-system-part-3/

https://docs.godotengine.org/en/3.1/tutorials/animation/animation_tree.html

http://www.downloads.redway3d.com/downloads/public/documentation/bk_skeletal_animation_blender.html

https://technology.riotgames.com/news/compressing-skeletal-animation-data

Animation Blending: Achieving Inverse Kinematics and More
by Jerry Edsall [Programming, Art] July 4, 2003
https://www.gamasutra.com/view/feature/131863/animation_blending_achieving_.php?page=2

Animation in Nebula Device 3:
http://flohofwoe.blogspot.com/2008/11/coreanimation.html
http://flohofwoe.blogspot.com/2009/01/animation.html
https://documentation.help/Nebula-3Device-3/class_animation_1_1_anim_sequencer.html



### Animation-Driven character movement (using Root Motion)

https://docs.unrealengine.com/en-US/AnimatingObjects/SkeletalMeshAnimation/RootMotion/index.html
https://www.casualdistractiongames.com/post/2017/05/09/ue4-root-motion

https://docs.godotengine.org/en/stable/tutorials/animation/animation_tree.html#root-motion


// Unity, Coupling Animation and Navigation:
https://docs.unity3d.com/Manual/nav-CouplingAnimationAndNavigation.html

https://docs.unity3d.com/ScriptReference/Animator-applyRootMotion.html
https://forum.unity.com/threads/animator-root-motion-or-character-controller.269639/?_ga=2.252974733.543378269.1614313333-1684129149.1568267398


https://forum.babylonjs.com/t/root-motion-animation-system/11488/8

https://takinginitiative.wordpress.com/2016/07/10/blending-animation-root-motion/


Common Mistakes In Game Animation
Animtips
https://www.gameanim.com/2016/09/30/common-mistakes-game-animation/

### Ragdolls / IK

#### PuppetMaster (Middleware)

http://root-motion.com/puppetmasterdox/html/page9.html
TOC:
http://root-motion.com/puppetmasterdox/html/pages.html
DOC:
http://root-motion.com/puppetmasterdox/html/annotated.html
