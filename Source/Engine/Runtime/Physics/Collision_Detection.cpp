//#include <Physics/Physics_PCH.h>
#include <Base/Base.h>
#pragma hdrstop
#include <Physics/Collision_Detection.h>
#if 0

// to avoid testing for null
void Default_Collision_Callback(
							 const Physics_Shape& _shapeA, const Physics_Shape& _shapeB,
							 const M44f& _transformA, const M44f& _transformB,
							 const pxProcessCollisionInput& input,
							 pxProcessCollisionOutput & _result
							 )
{
}


mxFORCEINLINE const float Plane_Sphere_Distance(
	const V4f& _planeEquation,
	const V3f& _sphereCenter, const float _sphereRadius
	)
{
	const float plane_to_sphere_center_distance = Plane_PointDistance( _planeEquation, _sphereCenter );
	return plane_to_sphere_center_distance - _sphereRadius;
}

void Collide_Sphere_Vs_Plane(
							 const Physics_Shape& _shapeA, const Physics_Shape& _shapeB,
							 const M44f& _transformA, const M44f& _transformB,
							 const pxProcessCollisionInput& input,
							 pxProcessCollisionOutput & _result
							 )
{
	const Sphere16& sphere = _shapeA.as.sphere;
	const Vector4 plane = _shapeB.as.plane;

	const V3f		sphereOrigin = M44_getTranslation(_transformA) + sphere.center;
	const pxReal	sphereRadius = sphere.radius;

	const V3f planeNormal = Plane_GetNormal(plane);

	// distance from plane to sphere origin
	const pxReal dist = Plane_PointDistance( plane, sphereOrigin );

	// penetration depth (negative distance from plane to sphere)
	const pxReal depth = sphereRadius - dist;

	// if penetration has occurred
	if( depth > 0/*CONTACT_THRESHOLD*/ )
	{
		// separation distance in the direction of contact normal
		// or penetration depth if positive (>epsilon)
		//DBGOUT("contact depth: %f\n",depth);

		Contact	newContactPoint;

		newContactPoint.position_on_B = sphereOrigin - planeNormal * depth;
		newContactPoint.penetration_depth = depth;
		newContactPoint.normal_on_B = Plane_GetNormal(plane);

		//m_manifold->oA = objA;
		//m_manifold->oB = objB;
		newContactPoint.bodyA = input.iBodyA;
		newContactPoint.bodyB = input.iBodyB;
		//dbgout << "contact: depth=" << depth << ", pos=" << point0.position.As_Vec3D() << "\n";

		_result.contacts->add( newContactPoint );
	}
}

void Collide_Sphere_Vs_Sphere(
							 const Physics_Shape& _shapeA, const Physics_Shape& _shapeB,
							 const M44f& _transformA, const M44f& _transformB,
							 const pxProcessCollisionInput& input,
							 pxProcessCollisionOutput & _result
							 )
{
	const Sphere16& sphereA = _shapeA.as.sphere;
	const Sphere16& sphereB = _shapeB.as.sphere;

	const V3f center_A_to_B = M44_getTranslation(_transformA) - M44_getTranslation(_transformB);
	const float distance_between_centers = V3_Length( center_A_to_B );

	// positive means penetration
	const pxReal penetration_depth = (sphereA.radius + sphereB.radius) - distance_between_centers;

	// if penetration has occurred
	if( penetration_depth > 0 )
	{
		Contact	newContactPoint;

		const V3f normal_on_B = ( distance_between_centers > SMALL_NUMBER ) ? (center_A_to_B / distance_between_centers) : V3_UP;

		newContactPoint.position_on_B = M44_getTranslation(_transformB) + normal_on_B * sphereB.radius;
		newContactPoint.penetration_depth = penetration_depth;
		newContactPoint.normal_on_B = normal_on_B;

		newContactPoint.bodyA = input.iBodyA;
		newContactPoint.bodyB = input.iBodyB;

		_result.contacts->add( newContactPoint );
	}
}

CollisionDispatcher::CollisionDispatcher()
{
}
CollisionDispatcher::~CollisionDispatcher()
{
}

ERet CollisionDispatcher::Initialize()
{
	mxZERO_OUT(m_collisionMatrix);
	for( int i = 0; i < Physics_Shape::MAX; i++ ) {
		for( int j = 0; j < Physics_Shape::MAX; j++ ) {
			m_collisionMatrix[i][j].callback = &Default_Collision_Callback;
		}
	}

	SetCollider( Physics_Shape::Sphere, Physics_Shape::Plane, &Collide_Sphere_Vs_Plane );
	SetCollider( Physics_Shape::Sphere, Physics_Shape::Sphere, &Collide_Sphere_Vs_Sphere );

	return ALL_OK;
}

void CollisionDispatcher::SetCollider(
	Physics_Shape::Type _shapeTypeA, Physics_Shape::Type _shapeTypeB,
	ColliderFunction* _callback
	)
{
	ColliderEntry & entry1 = m_collisionMatrix[ _shapeTypeA ][ _shapeTypeB ];
	entry1.callback = _callback;
	entry1.swapBodies = false;

	ColliderEntry & entry2 = m_collisionMatrix[ _shapeTypeB ][ _shapeTypeA ];
	entry2.callback = _callback;
	entry2.swapBodies = true;
}

void CollisionDispatcher::Collide(
	const UINT _iBodyA, const UINT _iBodyB,
	const Collidable& _oA, const Collidable& _oB,
	const M44f& _transformA, const M44f& _transformB,
	ContactsArray &_contacts
)
{
	const Physics_Shape::Type shapeTypeA = _oA.shape->type;
	const Physics_Shape::Type shapeTypeB = _oB.shape->type;

	ColliderEntry & entry = m_collisionMatrix[ shapeTypeA ][ shapeTypeB ];

	//Collidable & oA = entry.swapBodies ? _oB : _oA;
	//Collidable & oB = entry.swapBodies ? _oA : _oB;

	const Collidable* oA = &_oA;
	const Collidable* oB = &_oB;
	const M44f* trA = &_transformA;
	const M44f* trB = &_transformB;

	if( entry.swapBodies ) {
		TSwap( oA, oB );
		TSwap( trA, trB );
	}

	pxProcessCollisionInput		input;
	input.iBodyA = _iBodyA;
	input.iBodyB = _iBodyB;

	pxProcessCollisionOutput	result;
	result.contacts = &_contacts;

	(*entry.callback)( *oA->shape, *oB->shape, *trA, *trB, input, result );
}
#endif
//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
