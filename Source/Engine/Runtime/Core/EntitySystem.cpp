/*
=============================================================================
	File:	Core.cpp
	Desc:	
=============================================================================
*/
#include <Core/Core_PCH.h>
#pragma hdrstop
#include <Core/Core.h>
#include <Core/EntitySystem.h>

mxDEFINE_CLASS(Entity);
mxBEGIN_REFLECTION(Entity)
	mxMEMBER_FIELD(m_components),
mxEND_REFLECTION;
Entity::Entity()
{
	m_components = NULL;
}
Entity::~Entity()
{
}

ERet Entity::AddComponent( EntityComponent* newComponent )
{
	chkRET_X_IF_NOT( newComponent->m_owner == NULL, ERR_INVALID_PARAMETER );

	chkRET_X_IF_NOT(
		this->FindComponentOfType( *newComponent->m_class ) == NULL
		, ERR_DUPLICATE_OBJECT
		);

	newComponent->AppendSelfToList( &m_components );

	newComponent->m_owner = this;

	return ALL_OK;
}

ERet Entity::removeComponent( EntityComponent* component )
{
	chkRET_X_IF_NOT( component->m_owner == this, ERR_INVALID_PARAMETER );

	component->RemoveSelfFromList( &m_components );
	component->m_owner = nil;

	return ALL_OK;
}

EntityComponent* Entity::FindComponentOfType( const TbMetaClass& baseType )
{
	EntityComponent* current = m_components;
	while( current )
	{
		if( current->m_class->IsDerivedFrom( baseType ) ) {
			return current;
		}
		current = current->_next;
	}
	return NULL;
}

mxDEFINE_CLASS(EntityComponent);
mxBEGIN_REFLECTION(EntityComponent)
	mxMEMBER_FIELD(_next),
	mxMEMBER_FIELD(m_owner),
	mxMEMBER_FIELD(m_class),
mxEND_REFLECTION;
EntityComponent::EntityComponent( const TbMetaClass& meta_class )
{
	m_class = &meta_class;
}

EntityComponent::EntityComponent()
{
	m_class = &EntityComponent::metaClass();
}

EntityComponent::~EntityComponent()
{
	if( m_owner != nil )
	{
		m_owner->removeComponent( this );
	}
}

const TbMetaClass* EntityComponent::GetType() const
{
	return m_class;
}

namespace EntitySystem
{
	//
}//namespace EntitySystem

/*
Useful references:

http://gamesfromwithin.com/managing-data-relationships
https://gamedev.stackexchange.com/a/58379/15235


ECS:
https://infogalactic.com/info/Entity_component_system

implementations:
https://github.com/SergeyMakeev/ecs
https://github.com/google/corgi

game object system:
http://www.gamedev.ru/code/forum/?id=120342

mixins:
http://sim0nsays.livejournal.com/27342.html

The Entity-Component-System - C++ Game Design Pattern (Part 1)
Published November 21, 2017 by Tobs, posted by Tobs 
https://www.gamedev.net/tutorials/programming/general-and-gameplay-programming/the-entity-component-system-c-game-design-pattern-part-1-r4803/

game dev patterns:
http://www.gamedev.ru/community/gamedev_lecture/articles/?id=571

http://www.gamedev.ru/code/forum/?id=120480&page=2

// Методика проектирования, итеративность + ОО. (полуночная лекция) (комментарии)
http://www.gamedev.ru/community/oo_design/forum/?id=886#8
*/

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
