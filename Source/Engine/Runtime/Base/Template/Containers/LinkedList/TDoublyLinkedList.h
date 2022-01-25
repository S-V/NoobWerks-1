/*=============================================================================
//	File:	TDoublyLinkedList.h
//	Desc:	Doubly linked list template.
//	ToDo:	rewrite optimally
=============================================================================*/
#pragma once

///
///	Doubly-linked (aka 'bidirectionally-linked') list template
///
template< class CLASS >	// where CLASS : TDoublyLinkedList< CLASS >
struct TDoublyLinkedList
{
	CLASS *		next;	//!< the next item in a singly linked list, null if this is the last item
	CLASS *		prev;

public:
	typedef CLASS* Head;

	TDoublyLinkedList()
	{
		next = nil;
		prev = nil;
	}

	bool IsLoose() const
	{
		return next == nil
			&& prev == nil
			;
	}

	bool FindSelfInList( const CLASS* head ) const
	{
		const CLASS* curr = head;
		while(PtrToBool( curr ) && curr != this )
		{
			curr = curr->next;
		}
		return PtrToBool(curr);
	}

	/// insert 'this' between 'node.prev' and 'node'
	void InsertBefore( CLASS * node )
	{
		mxASSERT_PTR(node);
		this->RemoveSelfFromList();

		CLASS* me = static_cast< CLASS* >( this );

		CLASS* hisPrev = node->prev;

		this->next = node;
		this->prev = hisPrev;

		if( hisPrev ) {
			hisPrev->next = me;
		}
		node->prev = me;
	}

	/// insert 'this' between 'node' and 'node.next'
	void InsertAfter( CLASS * node )
	{
		mxASSERT_PTR(node);
		this->RemoveSelfFromList();

		CLASS* me = static_cast< CLASS* >( this );

		CLASS* hisNext = node->next;

		this->next = hisNext;
		this->prev = node;

		if( hisNext ) {
			hisNext->prev = me;
		}
		node->next = me;
	}

	/// unlinks this node from the containing list
	void RemoveSelfFromList()
	{
		if( prev ) {
			prev->next = next;
		}
		if( next ) {
			next->prev = prev;
		}
		prev = nil;
		next = nil;
	}

	/// prepends this node to the given list
	/// (will make this the head of the list if the list is nil)
	/// NOTE: the head of the list can be a null pointer (if empty list)
	void PrependSelfToList( CLASS ** head )
	{
		mxASSERT(this->IsLoose());
		mxASSERT_PTR(head);

		CLASS* me = static_cast< CLASS* >( this );

		if( *head )
		{
			CLASS* herPrev = (*head)->prev;
			//CLASS* herNext = (*head)->next;

			// insert this node between head->prev and head :
			//
			// ... herPrev <-> head <-> herNext ...
			// =>
			// ... herPrev <-> this <-> head <-> herNext ...

			this->next = *head;
			(*head)->prev = me;

			this->prev = herPrev;
			if( herPrev ) {
				herPrev->next = me;
			}
		}
		else
		{
			*head = me;
		}
	}

	/// appends this node to the given list
	/// (will make this the head of the list if the list is nil)
	/// NOTE: the head of the list can be a null pointer (if empty list)
	void AppendSelfToList( CLASS ** head )
	{
		mxASSERT(this->IsLoose());
		mxASSERT_PTR(head);

		CLASS* me = static_cast< CLASS* >( this );

		if( *head )
		{
			//CLASS* herPrev = (*head)->prev;
			CLASS* herNext = (*head)->next;

			// insert this node after head and before head->next :
			//
			// ... herPrev <-> head <-> herNext ...
			// =>
			// ... herPrev <-> head <-> this <-> herNext ...

			this->prev = *head;
			(*head)->next = me;

			this->next = herNext;
			if( herNext ) {
				herNext->prev = me;
			}
		}
		else
		{
			*head = me;
		}
	}

public:
	static UINT CountItemsInList( const CLASS* head )
	{
		UINT result = 0;
		const CLASS* current = head;
		while(PtrToBool( current ))
		{
			result++;
			current = current->next;
		}
		return result;
	}
	static CLASS* FindTail( CLASS* head, UINT *count = nil )
	{
		CLASS* result = head;
		CLASS* current = head;
		UINT itemCount = 0;
		while(PtrToBool( current ))
		{
			itemCount++;
			result = current;
			current = current->next;
		}
		if( count != nil ) { *count = itemCount; }
		return result;
	}
};


/// Doubly-linked list of queued items
template< class ITEM >
struct TLinkedDequeue
{
	ITEM *	head;	//!< items are taken from the beginning
	ITEM *	tail;	//!< new items are appended to the end
public:
	TLinkedDequeue()
	{
		this->SetEmpty();
	}
	void SetEmpty()
	{
		head = tail = nil;
	}

	/// Returns true if the queue is empty
	bool IsEmpty() const
	{
		return head == nil;	// NOTE: if the queue is empty, 'tail' must also be null
	}

	/// Adds a new item to the end of the queue.
	void Append( ITEM * newItem )
	{
		mxASSERT(newItem->prev == nil);
		mxASSERT(newItem->next == nil);

		if( tail )
		{
			// Append to the list
			mxASSERT(head != nil);
			newItem->prev = tail;
			// Fix links
			tail->next = newItem;
			tail = newItem;
		}
		else
		{
			// The queue is empty
			mxASSERT(head == nil);
			// add the first element
			head = newItem;
			tail = newItem;
		}
	}

	/// Removes the item at the front of the queue.
	ITEM* TakeFirst()
	{
		ITEM * result = head;	// front item
		if( result )
		{
			mxASSERT(head->prev == nil);
			head = head->next;	// update head
			if( head ) {
				head->prev = nil;	// unlink head from 'head->next'
			} else {
				tail = nil;	// New head is null => The queue becomes empty
			}
			// unlink item from the linked list
			result->prev = nil;
			result->next = nil;
		}
		else
		{
			mxASSERT( tail == nil );
		}
		return result;
	}
};
