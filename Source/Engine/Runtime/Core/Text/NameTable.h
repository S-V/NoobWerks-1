/*
=============================================================================
	File:	NameTable.h
	Desc:	global string table:
			shared immutable reference-counted strings for saving memory.
			String interning is a method of storing only one copy
			of each distinct string value, which must be immutable.
			Interning strings makes some string processing tasks
			more time- or space-efficient at the cost of requiring
			more time when the string is created or interned.
	Note:	NOT thread save!
=============================================================================
*/
#pragma once

mxSTOLEN("static_string by IronPeter");

/*
-----------------------------------------------------------------------------
	NameID
	A pointer to an interned string.
	Use operator bool to test whether the pointer is valid, and operator * to get the string if so.
	This is a lightweight value class with storage requirements equivalent to a single pointer,
	but it does have reference-counting overhead when copied.

	is a shared immutable reference-counted string (for saving memory and fast comparisons).
	Pooled strings are immutable. So that interned strings can eventually be freed,
	strings in the string pool are reference-counted (automatically).
-----------------------------------------------------------------------------
*/
class NameID {
public:
	inline const char *c_str() const throw()
	{
		return &pointer->body[0];
	}

	inline bool operator == ( const NameID& other ) const throw()
	{
		return pointer == other.pointer;
	}
	inline bool operator != ( const NameID& other ) const throw()
	{
		return pointer != other.pointer;
	}
	// for std::map / std::multimap
	inline bool operator < ( const NameID& other ) const {
		return pointer < other.pointer;
	}

	inline NameID &operator = ( const NameID& other ) throw()
	{
		if( *this != other )
		{
			NameID swp( other );
			StringPointer *temp = swp.pointer;
			swp.pointer = pointer;
			pointer = temp;
		}
		return *this;
	}

	inline NameID() throw()
	{
		pointer = &empty;
		pointer->refcounter++;
	}

	inline NameID( const NameID& other  ) throw()
	{
		pointer = other.pointer;
		pointer->refcounter++;
	}

	inline explicit NameID( const char *buff ) throw()
	{
		U32 hash, length;

		if( buff == 0 || buff[0] == 0 )
		{
			pointer = &empty;
			pointer->refcounter++;
			return;
		}

		GetStringHash( buff, hash, length );

		U32 position = hash % HASH_SIZE;
		StringPointer *entry = table[position];
		while( entry )
		{
			if( entry->length == length && entry->hash == hash && !memcmp( entry->body, buff, length ) )
			{
				pointer = entry;
				++entry->refcounter;
				return;
			}
			entry = entry->next;
		}

		++strnum;

		pointer = alloc_string( length );
		char *buffer = &pointer->body[0];
		memcpy( buffer, buff, length + 1 );
		pointer->hash = hash;
		pointer->next = table[position];
		pointer->refcounter = 1;
		pointer->length = length;
		table[position] = pointer;
	}

	inline ~NameID() throw()
	{
		if( --pointer->refcounter == 0 )
		{
			clear();
		}
	}

	inline U32 size() const throw()
	{
		return pointer->length;
	}

	inline U32 hash() const throw()
	{
		return pointer->hash;
	}

	//============================================================
	inline const char* raw() const throw()
	{
		return c_str();
	}
	inline UINT Length() const throw()
	{
		return size();
	}
	inline bool IsEmpty() const throw()
	{
		return size() == 0;
	}

	inline UINT NumRefs() const
	{
		return this->pointer->refcounter;
	}

	friend AWriter& operator << ( AWriter& file, const NameID& o );
	friend AReader& operator >> ( AReader& file, NameID& o );

	template< class S >
	friend S& operator & ( S & serializer, NameID & o )
	{
		return serializer.SerializeViaStream( o );
	}

public:

	static size_t get_str_num() throw()
	{
		return strnum;
	}

	static size_t get_str_memory() throw()
	{
		return strmemory;
	}

	static void StaticInitialize() throw();
	static void StaticShutdown() throw();


	enum
	{
		HASH_SIZE	= 65536,
		CACHE_SIZE	= 32,
		ALLOC_GRAN	= 8,
		ALLOC_SIZE	= 1024,
		MAX_LENGTH	= 512,
	};

	struct StringPointer
	{
		StringPointer *	next;
		U32			refcounter;
		U32			length;
		U32			hash;
		char			body[4];
	};

	struct ListNode
	{
		ListNode *	next;
	};

protected:
	StringPointer *	pointer;

	static StringPointer    empty;
	static size_t			strnum;
	static size_t			strmemory;
	static StringPointer    *table[HASH_SIZE];
	static ListNode *pointers[CACHE_SIZE];
	static ListNode *allocs;

	void* Alloc( size_t bytes );
	void Free( void* ptr, size_t bytes );

	StringPointer *alloc_string( U32 length ) throw();

	void release_string( StringPointer *ptr ) throw();

	inline void GetStringHash( const char *_str, U32 &hash, U32 &length ) throw()
	{       
		U32 res = 0;
		U32 i = 0;

		for(;;)
		{
			U32 v = _str[i];
			res = res * 5 + v;
			if( v == 0 )
			{
				hash = res;
				length = i;
				return;
			}
			++i;
		};
	}

	void clear() throw();
};

template<>
struct THashTrait< NameID >
{
	mxFORCEINLINE static UINT ComputeHash32( const NameID& key )
	{
		return key.hash();
	}
};

#if 0


struct NameID2
{
	inline NameID2()
	{
		hdr = &empty_string;
		hdr->ref_count++;
	}
	inline NameID2( const NameID2& _other  )
	{
		hdr = _other.hdr;
		hdr->ref_count++;
	}
	inline NameID2( const char* _chars )
	{
		if(mxUNLIKELY( _chars == 0 || _chars[0] == 0 ))
		{
			hdr = &empty;
			hdr->ref_count++;
			return;
		}

		U32 hash, length;
		GetStringHash( _chars, hash, length );

		Entry * entry = table[hash % HASH_SIZE];
		while( entry )
		{
			if( entry->length == length && entry->hash == hash && !memcmp( entry->body, _chars, length ) )
			{
				hdr = entry;
				++entry->ref_count;
				return;
			}
			entry = entry->next;
		}

		++strnum;

		hdr = alloc_string( length );
		char *buffer = &hdr->body[0];
		memcpy( buffer, _chars, length + 1 );
		hdr->hash = hash;
		hdr->next = table[position];
		hdr->ref_count = 1;
		hdr->length = length;
		table[position] = hdr;
	}

	inline ~NameID2()
	{
		if( --hdr->ref_count == 0 )
		{
			clear();
		}
	}

	Entry *	hdr;	//!< pointer to the internal data (header + string body)

public:
	static void Static_Initialize( AllocatorI &_heap );
	static void Static_Shutdown();

public:	// internal
	enum
	{
		MAX_LENGTH	= 1024,	//!< maximum allowed string length
		HASH_SIZE	= 65536,//!< number of name hash buckets
	};

	/// This is the value of an entry in the pool's interning table (header + string body).
	struct Entry : TLinkedList< Entry >
	{
		U32	ref_count;	// number of external references
		U32	length;		// excluding the null terminator
		U32	hash;
		//NOTE: the following field is variable-sized
		char	body[ MAX_LENGTH ];	// large size allows to inspect string contents in debugger
	};


	// std::basic_string-compatible interface

private:
	static Entry empty_string;	// empty string
};
#endif

//--------------------------------------------------------------//
//				End Of File.									//
//--------------------------------------------------------------//
