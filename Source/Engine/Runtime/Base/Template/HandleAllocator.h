// Stolen from bgfx/bx
/*
 * Copyright 2010-2018 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx#license-bsd-2-clause
 */

#pragma once


namespace bx
{
	static const U16 kInvalidHandle = UINT16_MAX;

	///
	class HandleAlloc
	{
	public:
		///
		HandleAlloc(U16 _maxHandles);

		///
		~HandleAlloc();

		///
		const U16* getHandles() const;

		///
		U16 getHandleAt(U16 _at) const;

		///
		U16 getNumHandles() const;

		///
		U16 getMaxHandles() const;

		///
		U16 alloc();

		///
		bool IsValid(U16 _handle) const;

		///
		void free(U16 _handle);

		///
		void reset();

	//private:
		HandleAlloc();

		///
		U16* getDensePtr() const;

		///
		U16* getSparsePtr() const;

		U16 m_numHandles;
		U16 m_maxHandles;
	};

	///
	HandleAlloc* createHandleAlloc(AllocatorI* _allocator, U16 _maxHandles);

	///
	void destroyHandleAlloc(AllocatorI* _allocator, HandleAlloc* _handleAlloc);

	///
	template <U16 MaxHandlesT>
	class HandleAllocT: public HandleAlloc
	{
	public:
		///
		HandleAllocT();

		///
		~HandleAllocT();

	private:
		U16 m_padding[2*MaxHandlesT];
	};

	///
	template <U16 MaxHandlesT>
	class HandleListT
	{
	public:
		///
		HandleListT();

		///
		void pushBack(U16 _handle);

		///
		U16 popBack();

		///
		void pushFront(U16 _handle);

		///
		U16 popFront();

		///
		U16 getFront() const;

		///
		U16 getBack() const;

		///
		U16 getNext(U16 _handle) const;

		///
		U16 getPrev(U16 _handle) const;

		///
		void remove(U16 _handle);

		///
		void reset();

	private:
		///
		void insertBefore(U16 _before, U16 _handle);

		///
		void insertAfter(U16 _after, U16 _handle);

		///
		bool IsValid(U16 _handle) const;

		///
		void updateFrontBack(U16 _handle);

		U16 m_front;
		U16 m_back;

		struct Link
		{
			U16 m_prev;
			U16 m_next;
		};

		Link m_links[MaxHandlesT];
	};

	///
	template <U16 MaxHandlesT>
	class HandleAllocLruT
	{
	public:
		///
		HandleAllocLruT();

		///
		~HandleAllocLruT();

		///
		const U16* getHandles() const;

		///
		U16 getHandleAt(U16 _at) const;

		///
		U16 getNumHandles() const;

		///
		U16 getMaxHandles() const;

		///
		U16 alloc();

		///
		bool IsValid(U16 _handle) const;

		///
		void free(U16 _handle);

		///
		void touch(U16 _handle);

		///
		U16 getFront() const;

		///
		U16 getBack() const;

		///
		U16 getNext(U16 _handle) const;

		///
		U16 getPrev(U16 _handle) const;

		///
		void reset();

	private:
		HandleListT<MaxHandlesT>  m_list;
		HandleAllocT<MaxHandlesT> m_alloc;
	};

	///
	template <U32 MaxCapacityT, typename KeyT = U32>
	class HandleHashMapT
	{
	public:
		///
		HandleHashMapT();

		///
		~HandleHashMapT();

		///
		bool insert(KeyT _key, U16 _handle);

		///
		bool removeByKey(KeyT _key);

		///
		bool removeByHandle(U16 _handle);

		///
		U16 find(KeyT _key) const;

		///
		void reset();

		///
		U32 getNumElements() const;

		///
		U32 getMaxCapacity() const;

		///
		struct Iterator
		{
			U16 handle;

		private:
			friend class HandleHashMapT<MaxCapacityT, KeyT>;
			U32 pos;
			U32 num;
		};

		///
		Iterator first() const;

		///
		bool next(Iterator& _it) const;

	private:
		///
		U32 findIndex(KeyT _key) const;

		///
		void removeIndex(U32 _idx);

		///
		U32 mix(U32 _x) const;

		///
		U64 mix(U64 _x) const;

		U32 m_maxCapacity;
		U32 m_numElements;

		KeyT     m_key[MaxCapacityT];
		U16 m_handle[MaxCapacityT];
	};

	///
	template <U16 MaxHandlesT, typename KeyT = U32>
	class HandleHashMapAllocT
	{
	public:
		///
		HandleHashMapAllocT();

		///
		~HandleHashMapAllocT();

		///
		U16 alloc(KeyT _key);

		///
		void free(KeyT _key);

		///
		void free(U16 _handle);

		///
		U16 find(KeyT _key) const;

		///
		const U16* getHandles() const;

		///
		U16 getHandleAt(U16 _at) const;

		///
		U16 getNumHandles() const;

		///
		U16 getMaxHandles() const;

		///
		bool IsValid(U16 _handle) const;

		///
		void reset();

	private:
		HandleHashMapT<MaxHandlesT+MaxHandlesT/2, KeyT> m_table;
		HandleAllocT<MaxHandlesT> m_alloc;
	};

} // namespace bx

namespace bx
{
	inline HandleAlloc::HandleAlloc(U16 _maxHandles)
		: m_numHandles(0)
		, m_maxHandles(_maxHandles)
	{
		reset();
	}

	inline HandleAlloc::~HandleAlloc()
	{
	}

	inline const U16* HandleAlloc::getHandles() const
	{
		return getDensePtr();
	}

	inline U16 HandleAlloc::getHandleAt(U16 _at) const
	{
		return getDensePtr()[_at];
	}

	inline U16 HandleAlloc::getNumHandles() const
	{
		return m_numHandles;
	}

	inline U16 HandleAlloc::getMaxHandles() const
	{
		return m_maxHandles;
	}

	inline U16 HandleAlloc::alloc()
	{
		if (m_numHandles < m_maxHandles)
		{
			U16 index = m_numHandles;
			++m_numHandles;

			U16* dense  = getDensePtr();
			U16  handle = dense[index];
			U16* sparse = getSparsePtr();
			sparse[handle] = index;
			return handle;
		}

		return kInvalidHandle;
	}

	inline bool HandleAlloc::IsValid(U16 _handle) const
	{
		U16* dense  = getDensePtr();
		U16* sparse = getSparsePtr();
		U16  index  = sparse[_handle];

		return index < m_numHandles
			&& dense[index] == _handle
			;
	}

	inline void HandleAlloc::free(U16 _handle)
	{
		U16* dense  = getDensePtr();
		U16* sparse = getSparsePtr();
		U16 index = sparse[_handle];
		--m_numHandles;
		U16 temp = dense[m_numHandles];
		dense[m_numHandles] = _handle;
		sparse[temp] = index;
		dense[index] = temp;
	}

	inline void HandleAlloc::reset()
	{
		m_numHandles = 0;
		U16* dense = getDensePtr();
		for (U16 ii = 0, num = m_maxHandles; ii < num; ++ii)
		{
			dense[ii] = ii;
		}
	}

	inline U16* HandleAlloc::getDensePtr() const
	{
		uint8_t* ptr = (uint8_t*)reinterpret_cast<const uint8_t*>(this);
		return (U16*)&ptr[sizeof(HandleAlloc)];
	}

	inline U16* HandleAlloc::getSparsePtr() const
	{
		return &getDensePtr()[m_maxHandles];
	}

	inline HandleAlloc* createHandleAlloc(AllocatorI* _allocator, U16 _maxHandles)
	{
		uint8_t* ptr = (uint8_t*)BX_ALLOC(_allocator, sizeof(HandleAlloc) + 2*_maxHandles*sizeof(U16) );
		return new(ptr) HandleAlloc(_maxHandles);
	}

	inline void destroyHandleAlloc(AllocatorI* _allocator, HandleAlloc* _handleAlloc)
	{
		_handleAlloc->~HandleAlloc();
		BX_FREE(_allocator, _handleAlloc);
	}

	template <U16 MaxHandlesT>
	inline HandleAllocT<MaxHandlesT>::HandleAllocT()
		: HandleAlloc(MaxHandlesT)
	{
	}

	template <U16 MaxHandlesT>
	inline HandleAllocT<MaxHandlesT>::~HandleAllocT()
	{
	}

	template <U16 MaxHandlesT>
	inline HandleListT<MaxHandlesT>::HandleListT()
	{
		reset();
	}

	template <U16 MaxHandlesT>
	inline void HandleListT<MaxHandlesT>::pushBack(U16 _handle)
	{
		insertAfter(m_back, _handle);
	}

	template <U16 MaxHandlesT>
	inline U16 HandleListT<MaxHandlesT>::popBack()
	{
		U16 last = kInvalidHandle != m_back
			? m_back
			: m_front
			;

		if (kInvalidHandle != last)
		{
			remove(last);
		}

		return last;
	}

	template <U16 MaxHandlesT>
	inline void HandleListT<MaxHandlesT>::pushFront(U16 _handle)
	{
		insertBefore(m_front, _handle);
	}

	template <U16 MaxHandlesT>
	inline U16 HandleListT<MaxHandlesT>::popFront()
	{
		U16 front = m_front;

		if (kInvalidHandle != front)
		{
			remove(front);
		}

		return front;
	}

	template <U16 MaxHandlesT>
	inline U16 HandleListT<MaxHandlesT>::getFront() const
	{
		return m_front;
	}

	template <U16 MaxHandlesT>
	inline U16 HandleListT<MaxHandlesT>::getBack() const
	{
		return m_back;
	}

	template <U16 MaxHandlesT>
	inline U16 HandleListT<MaxHandlesT>::getNext(U16 _handle) const
	{
		BX_CHECK(IsValid(_handle), "Invalid handle %d!", _handle);
		const Link& curr = m_links[_handle];
		return curr.m_next;
	}

	template <U16 MaxHandlesT>
	inline U16 HandleListT<MaxHandlesT>::getPrev(U16 _handle) const
	{
		BX_CHECK(IsValid(_handle), "Invalid handle %d!", _handle);
		const Link& curr = m_links[_handle];
		return curr.m_prev;
	}

	template <U16 MaxHandlesT>
	inline void HandleListT<MaxHandlesT>::remove(U16 _handle)
	{
		BX_CHECK(IsValid(_handle), "Invalid handle %d!", _handle);
		Link& curr = m_links[_handle];

		if (kInvalidHandle != curr.m_prev)
		{
			Link& prev  = m_links[curr.m_prev];
			prev.m_next = curr.m_next;
		}
		else
		{
			m_front = curr.m_next;
		}

		if (kInvalidHandle != curr.m_next)
		{
			Link& next  = m_links[curr.m_next];
			next.m_prev = curr.m_prev;
		}
		else
		{
			m_back = curr.m_prev;
		}

		curr.m_prev = kInvalidHandle;
		curr.m_next = kInvalidHandle;
	}

	template <U16 MaxHandlesT>
	inline void HandleListT<MaxHandlesT>::reset()
	{
		memSet(m_links, 0xff, sizeof(m_links) );
		m_front = kInvalidHandle;
		m_back  = kInvalidHandle;
	}

	template <U16 MaxHandlesT>
	inline void HandleListT<MaxHandlesT>::insertBefore(U16 _before, U16 _handle)
	{
		Link& curr = m_links[_handle];
		curr.m_next = _before;

		if (kInvalidHandle != _before)
		{
			Link& link = m_links[_before];
			if (kInvalidHandle != link.m_prev)
			{
				Link& prev = m_links[link.m_prev];
				prev.m_next = _handle;
			}

			curr.m_prev = link.m_prev;
			link.m_prev = _handle;
		}

		updateFrontBack(_handle);
	}

	template <U16 MaxHandlesT>
	inline void HandleListT<MaxHandlesT>::insertAfter(U16 _after, U16 _handle)
	{
		Link& curr = m_links[_handle];
		curr.m_prev = _after;

		if (kInvalidHandle != _after)
		{
			Link& link = m_links[_after];
			if (kInvalidHandle != link.m_next)
			{
				Link& next = m_links[link.m_next];
				next.m_prev = _handle;
			}

			curr.m_next = link.m_next;
			link.m_next = _handle;
		}

		updateFrontBack(_handle);
	}

	template <U16 MaxHandlesT>
	inline bool HandleListT<MaxHandlesT>::IsValid(U16 _handle) const
	{
		return _handle < MaxHandlesT;
	}

	template <U16 MaxHandlesT>
	inline void HandleListT<MaxHandlesT>::updateFrontBack(U16 _handle)
	{
		Link& curr = m_links[_handle];

		if (kInvalidHandle == curr.m_prev)
		{
			m_front = _handle;
		}

		if (kInvalidHandle == curr.m_next)
		{
			m_back = _handle;
		}
	}

	template <U16 MaxHandlesT>
	inline HandleAllocLruT<MaxHandlesT>::HandleAllocLruT()
	{
		reset();
	}

	template <U16 MaxHandlesT>
	inline HandleAllocLruT<MaxHandlesT>::~HandleAllocLruT()
	{
	}

	template <U16 MaxHandlesT>
	inline const U16* HandleAllocLruT<MaxHandlesT>::getHandles() const
	{
		return m_alloc.getHandles();
	}

	template <U16 MaxHandlesT>
	inline U16 HandleAllocLruT<MaxHandlesT>::getHandleAt(U16 _at) const
	{
		return m_alloc.getHandleAt(_at);
	}

	template <U16 MaxHandlesT>
	inline U16 HandleAllocLruT<MaxHandlesT>::getNumHandles() const
	{
		return m_alloc.getNumHandles();
	}

	template <U16 MaxHandlesT>
	inline U16 HandleAllocLruT<MaxHandlesT>::getMaxHandles() const
	{
		return m_alloc.getMaxHandles();
	}

	template <U16 MaxHandlesT>
	inline U16 HandleAllocLruT<MaxHandlesT>::alloc()
	{
		U16 handle = m_alloc.alloc();
		if (kInvalidHandle != handle)
		{
			m_list.pushFront(handle);
		}
		return handle;
	}

	template <U16 MaxHandlesT>
	inline bool HandleAllocLruT<MaxHandlesT>::IsValid(U16 _handle) const
	{
		return m_alloc.IsValid(_handle);
	}

	template <U16 MaxHandlesT>
	inline void HandleAllocLruT<MaxHandlesT>::free(U16 _handle)
	{
		BX_CHECK(IsValid(_handle), "Invalid handle %d!", _handle);
		m_list.remove(_handle);
		m_alloc.free(_handle);
	}

	template <U16 MaxHandlesT>
	inline void HandleAllocLruT<MaxHandlesT>::touch(U16 _handle)
	{
		BX_CHECK(IsValid(_handle), "Invalid handle %d!", _handle);
		m_list.remove(_handle);
		m_list.pushFront(_handle);
	}

	template <U16 MaxHandlesT>
	inline U16 HandleAllocLruT<MaxHandlesT>::getFront() const
	{
		return m_list.getFront();
	}

	template <U16 MaxHandlesT>
	inline U16 HandleAllocLruT<MaxHandlesT>::getBack() const
	{
		return m_list.getBack();
	}

	template <U16 MaxHandlesT>
	inline U16 HandleAllocLruT<MaxHandlesT>::getNext(U16 _handle) const
	{
		return m_list.getNext(_handle);
	}

	template <U16 MaxHandlesT>
	inline U16 HandleAllocLruT<MaxHandlesT>::getPrev(U16 _handle) const
	{
		return m_list.getPrev(_handle);
	}

	template <U16 MaxHandlesT>
	inline void HandleAllocLruT<MaxHandlesT>::reset()
	{
		m_list.reset();
		m_alloc.reset();
	}

	template <U32 MaxCapacityT, typename KeyT>
	inline HandleHashMapT<MaxCapacityT, KeyT>::HandleHashMapT()
		: m_maxCapacity(MaxCapacityT)
	{
		reset();
	}

	template <U32 MaxCapacityT, typename KeyT>
	inline HandleHashMapT<MaxCapacityT, KeyT>::~HandleHashMapT()
	{
	}

	template <U32 MaxCapacityT, typename KeyT>
	inline bool HandleHashMapT<MaxCapacityT, KeyT>::insert(KeyT _key, U16 _handle)
	{
		if (kInvalidHandle == _handle)
		{
			return false;
		}

		const KeyT hash = mix(_key);
		const U32 firstIdx = hash % MaxCapacityT;
		U32 idx = firstIdx;
		do
		{
			if (m_handle[idx] == kInvalidHandle)
			{
				m_key[idx]    = _key;
				m_handle[idx] = _handle;
				++m_numElements;
				return true;
			}

			if (m_key[idx] == _key)
			{
				return false;
			}

			idx = (idx + 1) % MaxCapacityT;

		} while (idx != firstIdx);

		return false;
	}

	template <U32 MaxCapacityT, typename KeyT>
	inline bool HandleHashMapT<MaxCapacityT, KeyT>::removeByKey(KeyT _key)
	{
		U32 idx = findIndex(_key);
		if (UINT32_MAX != idx)
		{
			removeIndex(idx);
			return true;
		}

		return false;
	}

	template <U32 MaxCapacityT, typename KeyT>
	inline bool HandleHashMapT<MaxCapacityT, KeyT>::removeByHandle(U16 _handle)
	{
		if (kInvalidHandle != _handle)
		{
			for (U32 idx = 0; idx < MaxCapacityT; ++idx)
			{
				if (m_handle[idx] == _handle)
				{
					removeIndex(idx);
				}
			}
		}

		return false;
	}

	template <U32 MaxCapacityT, typename KeyT>
	inline U16 HandleHashMapT<MaxCapacityT, KeyT>::find(KeyT _key) const
	{
		U32 idx = findIndex(_key);
		if (UINT32_MAX != idx)
		{
			return m_handle[idx];
		}

		return kInvalidHandle;
	}

	template <U32 MaxCapacityT, typename KeyT>
	inline void HandleHashMapT<MaxCapacityT, KeyT>::reset()
	{
		memSet(m_handle, 0xff, sizeof(m_handle) );
		m_numElements = 0;
	}

	template <U32 MaxCapacityT, typename KeyT>
	inline U32 HandleHashMapT<MaxCapacityT, KeyT>::getNumElements() const
	{
		return m_numElements;
	}

	template <U32 MaxCapacityT, typename KeyT>
	inline U32 HandleHashMapT<MaxCapacityT, KeyT>::getMaxCapacity() const
	{
		return m_maxCapacity;
	}

	template <U32 MaxCapacityT, typename KeyT>
	inline typename HandleHashMapT<MaxCapacityT, KeyT>::Iterator HandleHashMapT<MaxCapacityT, KeyT>::first() const
	{
		Iterator it;
		it.handle = kInvalidHandle;
		it.pos    = 0;
		it.num    = m_numElements;

		if (0 == it.num)
		{
			return it;
		}

		++it.num;
		next(it);
		return it;
	}

	template <U32 MaxCapacityT, typename KeyT>
	inline bool HandleHashMapT<MaxCapacityT, KeyT>::next(Iterator& _it) const
	{
		if (0 == _it.num)
		{
			return false;
		}

		for (
			;_it.pos < MaxCapacityT && kInvalidHandle == m_handle[_it.pos]
			; ++_it.pos
			);
		_it.handle = m_handle[_it.pos];
		++_it.pos;
		--_it.num;
		return true;
	}

	template <U32 MaxCapacityT, typename KeyT>
	inline U32 HandleHashMapT<MaxCapacityT, KeyT>::findIndex(KeyT _key) const
	{
		const KeyT hash = mix(_key);

		const U32 firstIdx = hash % MaxCapacityT;
		U32 idx = firstIdx;
		do
		{
			if (m_handle[idx] == kInvalidHandle)
			{
				return UINT32_MAX;
			}

			if (m_key[idx] == _key)
			{
				return idx;
			}

			idx = (idx + 1) % MaxCapacityT;

		} while (idx != firstIdx);

		return UINT32_MAX;
	}

	template <U32 MaxCapacityT, typename KeyT>
	inline void HandleHashMapT<MaxCapacityT, KeyT>::removeIndex(U32 _idx)
	{
		m_handle[_idx] = kInvalidHandle;
		--m_numElements;

		for (U32 idx = (_idx + 1) % MaxCapacityT
				; m_handle[idx] != kInvalidHandle
				; idx = (idx + 1) % MaxCapacityT)
		{
			if (m_handle[idx] != kInvalidHandle)
			{
				const KeyT key = m_key[idx];
				if (idx != findIndex(key) )
				{
					const U16 handle = m_handle[idx];
					m_handle[idx] = kInvalidHandle;
					--m_numElements;
					insert(key, handle);
				}
			}
		}
	}

	template <U32 MaxCapacityT, typename KeyT>
	inline U32 HandleHashMapT<MaxCapacityT, KeyT>::mix(U32 _x) const
	{
		const U32 tmp0   = uint32_mul(_x,   UINT32_C(2246822519) );
		const U32 tmp1   = uint32_rol(tmp0, 13);
		const U32 result = uint32_mul(tmp1, UINT32_C(2654435761) );
		return result;
	}

	template <U32 MaxCapacityT, typename KeyT>
	inline U64 HandleHashMapT<MaxCapacityT, KeyT>::mix(U64 _x) const
	{
		const U64 tmp0   = uint64_mul(_x,   UINT64_C(14029467366897019727) );
		const U64 tmp1   = uint64_rol(tmp0, 31);
		const U64 result = uint64_mul(tmp1, UINT64_C(11400714785074694791) );
		return result;
	}

	template <U16 MaxHandlesT, typename KeyT>
	inline HandleHashMapAllocT<MaxHandlesT, KeyT>::HandleHashMapAllocT()
	{
		reset();
	}

	template <U16 MaxHandlesT, typename KeyT>
	inline HandleHashMapAllocT<MaxHandlesT, KeyT>::~HandleHashMapAllocT()
	{
	}

	template <U16 MaxHandlesT, typename KeyT>
	inline U16 HandleHashMapAllocT<MaxHandlesT, KeyT>::alloc(KeyT _key)
	{
		U16 handle = m_alloc.alloc();
		if (kInvalidHandle == handle)
		{
			return kInvalidHandle;
		}

		bool ok = m_table.insert(_key, handle);
		if (!ok)
		{
			m_alloc.free(handle);
			return kInvalidHandle;
		}

		return handle;
	}

	template <U16 MaxHandlesT, typename KeyT>
	inline void HandleHashMapAllocT<MaxHandlesT, KeyT>::free(KeyT _key)
	{
		U16 handle = m_table.find(_key);
		if (kInvalidHandle == handle)
		{
			return;
		}

		m_table.removeByKey(_key);
		m_alloc.free(handle);
	}

	template <U16 MaxHandlesT, typename KeyT>
	inline void HandleHashMapAllocT<MaxHandlesT, KeyT>::free(U16 _handle)
	{
		m_table.removeByHandle(_handle);
		m_alloc.free(_handle);
	}

	template <U16 MaxHandlesT, typename KeyT>
	inline U16 HandleHashMapAllocT<MaxHandlesT, KeyT>::find(KeyT _key) const
	{
		return m_table.find(_key);
	}

	template <U16 MaxHandlesT, typename KeyT>
	inline const U16* HandleHashMapAllocT<MaxHandlesT, KeyT>::getHandles() const
	{
		return m_alloc.getHandles();
	}

	template <U16 MaxHandlesT, typename KeyT>
	inline U16 HandleHashMapAllocT<MaxHandlesT, KeyT>::getHandleAt(U16 _at) const
	{
		return m_alloc.getHandleAt(_at);
	}

	template <U16 MaxHandlesT, typename KeyT>
	inline U16 HandleHashMapAllocT<MaxHandlesT, KeyT>::getNumHandles() const
	{
		return m_alloc.getNumHandles();
	}

	template <U16 MaxHandlesT, typename KeyT>
	inline U16 HandleHashMapAllocT<MaxHandlesT, KeyT>::getMaxHandles() const
	{
		return m_alloc.getMaxHandles();
	}

	template <U16 MaxHandlesT, typename KeyT>
	inline bool HandleHashMapAllocT<MaxHandlesT, KeyT>::IsValid(U16 _handle) const
	{
		return m_alloc.IsValid(_handle);
	}

	template <U16 MaxHandlesT, typename KeyT>
	inline void HandleHashMapAllocT<MaxHandlesT, KeyT>::reset()
	{
		m_table.reset();
		m_alloc.reset();
	}

} // namespace bx
