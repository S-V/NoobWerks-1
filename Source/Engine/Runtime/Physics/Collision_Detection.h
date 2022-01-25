#pragma once

#include <Physics/Physics_Config.h>
#if 0

/// Rigid Body Transform
//struct Transform : M44f
//{
//};

///
///	Physics Shape - represents collision geometry as seen by the physics engine.
///	It is usually a surface/volume representation for doing collision detection.
///	pxShape is the base class for all shapes used in narrowphase collision detection.
///
struct Physics_Shape
{
	enum Type : U32 {
		Plane,
		Sphere,
		Box,
		SDF,	//!< Signed Distance Field
		MAX
	};

	/// The shape's type. This is used by the collision dispatcher to dispatch between pairs of shapes.
	Type	type;

	//void *	user_data;	//!< user-manageable pointer

	// type-dependent:
	union {
		Vector4		plane;
		Sphere16	sphere;
	} as;
};

struct Collidable
{
	Physics_Shape *	shape;
	//void *			user_data;
	U32				iBody;
//	V3f		current_position;
//	V3f		previous_position;
};

//
//	pxContactPoint - a simple structure class representing a contact point with a normal and a penetration depth.
//
//	NOTE: the normal always points "into" A, and "out of" B.
// The penetration depth of a pair of intersecting objects is the shortest vector
// over which one object needs to be translated in order to bring the pair in touching contact.
// NOTE: usually, A represents dynamic (moving) objects and B - static ones.
//
mxPREALIGN(16) struct Contact
{
	/// position of the contact point (in world space) on bodyB;
	/// position on bodyA == position_on_B + contact_normal_B_to_A * penetration_depth;
	V3f		position_on_B;

	U16			bodyA;
	U16			bodyB;

	/// (xyz) - contact normal vector (toward feature on body A, in world space)
	/// (w) - separation distance in the direction of contact normal (penetration depth if positive (>epsilon)),
	/// lies in range [ -maximum_penetration_depth, maximum_separation_distance ]
	V3f		normal_on_B;	//!< points from B to A
	F32		penetration_depth;
};

typedef TArray< Contact >	ContactsArray;

struct pxProcessCollisionInput
{
	U16	iBodyA;
	U16	iBodyB;
};
struct pxProcessCollisionOutput
{
	ContactsArray *	contacts;
};

typedef void ColliderFunction(
	const Physics_Shape& _shapeA, const Physics_Shape& _shapeB,
	const M44f& _transformA, const M44f& _transformB,
	const pxProcessCollisionInput& input,
	pxProcessCollisionOutput & _result
);

/// ColliderEntry is a single entry in collision matrix
struct ColliderEntry
{
	ColliderFunction *	callback;	//!< collider function
	FASTBOOL		swapBodies;		//!< 1 -> reverse body A and body B
};

///
/// This class is used to find the correct function which performs collision detection between pairs of shapes.
/// It's based on the following rules:
///  - Each shapes has a type
///  - For each pair of types, there must be a function registered to handle this pair of types
///
class CollisionDispatcher: NonCopyable {
public:
	CollisionDispatcher();
	~CollisionDispatcher();

	ERet Initialize();

	void SetCollider(
		Physics_Shape::Type _shapeTypeA, Physics_Shape::Type _shapeTypeB,
		ColliderFunction* _callback
		);

public:	// Run-Time

	void Collide(
		const UINT _iBodyA, const UINT _iBodyB,
		const Collidable& _oA, const Collidable& _oB,
		const M44f& _transformA, const M44f& _transformB,
		ContactsArray &_contacts
	);

private:
	/// collision matrix for invoking proper near-phase callbacks
	mxPREALIGN(16)	ColliderEntry	m_collisionMatrix[ Physics_Shape::MAX ][ Physics_Shape::MAX ];
};
#endif

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
