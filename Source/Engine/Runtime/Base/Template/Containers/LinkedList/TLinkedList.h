/*=============================================================================
//	File:	TLinkedList.h
//	Desc:	Intrusive singly linked list template.
=============================================================================*/
#pragma once

#define DBG_LINKED_LISTS	(0)

/*
----------------------------------------------------------
	Intrusive singly linked list template.
	A singly linked list is a list where each element
	is linked to the next element, but not to the previous element.
	That is, it is a Sequence that supports forward
	but not backward traversal,
	and (amortized) constant time insertion
	and removal of elements.
----------------------------------------------------------
*/
template< class CLASS >	// where CLASS : TLinkedList< CLASS >
struct TLinkedList
{
	CLASS *		_next;	// the next item in the linked list, null if this is the last item

public:
	typedef CLASS* Head;

	TLinkedList()
	{
		_next = nil;
	}

	bool FindSelfInList( const CLASS* head ) const
	{
		const CLASS* curr = head;
		while(PtrToBool( curr ) && curr != this )
		{
			curr = curr->_next;
		}
		return PtrToBool(curr);
	}

	void PrependSelfToList( CLASS ** head )
	{
#if DBG_LINKED_LISTS
		mxASSERT(!this->FindSelfInList(*head));
#endif
		_next = *head;
		*head = static_cast< CLASS* >(this);
	}

	void AppendSelfToList( CLASS ** head )
	{
		mxASSERT_PTR(head);
#if DBG_LINKED_LISTS
		mxASSERT(!this->FindSelfInList(*head));
#endif
		CLASS** curr = head;
		while(PtrToBool( *curr ))
		{
			curr = &((*curr)->_next);
		}
		*curr = static_cast< CLASS* >(this);
		_next = nil;
	}

	CLASS* RemoveSelfFromList( CLASS ** head )
	{
		CLASS** prev = head;
		CLASS*  curr = *prev;
		while( PtrToBool(curr) && curr != this )
		{
			prev = &curr->_next;
			curr = curr->_next;
		}
		if(PtrToBool( curr ))
		{
			*prev = curr->_next;
			curr->_next = nil;
		}
		return curr;
	}

public:
	class Iterator
	{
		CLASS *		m_current;
	public:
		Iterator( CLASS* listHead )
			: m_current( listHead )
		{}
		bool IsValid() const
		{
			return m_current != nil;
		}
		void MoveToNext()
		{
			m_current = m_current->_next;
		}
		CLASS& Value()
		{
			mxASSERT(this->IsValid());
			return *m_current;
		}
	private:
		PREVENT_COPY(Iterator);
	};

	static UINT CountItemsInList( const CLASS* head )
	{
		UINT result = 0;
		const CLASS* curr = head;
		while(PtrToBool( curr ))
		{
			result++;
			curr = curr->next;
		}
		return result;
	}
};
