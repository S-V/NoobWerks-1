/*
=============================================================================
	File:	Core.h
	Desc:	
	https://infogalactic.com/info/Entity_component_system
=============================================================================
*/
#pragma once

class EntityComponent;

// Upper design limits
enum {
	MAX_ENTITIES = 1024
};

//mxDECLARE_32BIT_HANDLE(HEntity);
//mxREFLECT_AS_BUILT_IN_INTEGER(HEntity);

///
struct EntityID
{
	union {
		U32			id;	// zero == nil
		struct {
			U16		slot;
			U16		version;
		};
	};

public:
	// so that it can be put into unions
	//mxFORCEINLINE EntityID()
	//{
	//	this->SetNil();
	//}

	mxFORCEINLINE void SetNil()	{ this->id = 0; }
	mxFORCEINLINE bool isNull() const	{ return this->id == 0; }
	mxFORCEINLINE bool IsValid() const	{ return this->id != 0; }
	mxFORCEINLINE bool operator == ( const EntityID& other ) const { return this->id == other.id; }
	mxFORCEINLINE bool operator != ( const EntityID& other ) const { return this->id != other.id; }
};

/*
----------------------------------------------------------
	Entity

	The basic building block of a game, that does not do much on its own.
	It can be associated with many Components to achieve more complex behavior.
----------------------------------------------------------
*/
class Entity: public CStruct
{
	EntityComponent *	m_components;	// linked list of components

public:
	mxDECLARE_CLASS(Entity,CStruct);
	mxDECLARE_REFLECTION;

	Entity();
	~Entity();

	ERet AddComponent( EntityComponent* newComponent );
	ERet removeComponent( EntityComponent* component );

	EntityComponent* FindComponentOfType( const TbMetaClass& baseType );

	template< class COMPONENT_TYPE >	// where COMPONENT_TYPE : EntityComponent
	COMPONENT_TYPE* findComponentOfType()
	{
		return static_cast< COMPONENT_TYPE* >( this->FindComponentOfType( COMPONENT_TYPE::metaClass() ) );
	}
};


/*
----------------------------------------------------------
	EntityComponent

	An object that contains the logic and data pertaining to a particular system in the game.
----------------------------------------------------------
*/
class EntityComponent: public CStruct, public TLinkedList< EntityComponent >
{
public:
	TPtr< const TbMetaClass >	m_class;
	TPtr< Entity >			m_owner;	// will be set by the parent entity

public:
	mxDECLARE_CLASS(EntityComponent,CStruct);
	mxDECLARE_REFLECTION;

	EntityComponent( const TbMetaClass& meta_class );
	EntityComponent();
	~EntityComponent();

	const TbMetaClass* GetType() const;
};

namespace EntitySystem
{

	// Entity Manager
	// This is a the object that ties Entities and Components together. It acts as the main point of interface for game logic to create Entities and register them with Components.


}//namespace EntitySystem

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
