#pragma once
#include <stdint.h>
#include <base/tl/array.h>
#include <base/vmath.h>
#include <game/client/ui.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

template<typename T>
struct array2: public array<T>
{
	typedef array<T> ParentT;

	inline T& add(const T& Elt)
	{
		return this->base_ptr()[ParentT::add(Elt)];
	}

	T& add(const T* aElements, int EltCount)
	{
		for(int i = 0; i < EltCount; i++)
			ParentT::add(aElements[i]);
		return *(this->base_ptr()+this->size()-EltCount);
	}

	T& add_empty(int EltCount)
	{
		dbg_assert(EltCount > 0, "Add 0 or more");
		for(int i = 0; i < EltCount; i++)
			ParentT::add(T());
		return *(this->base_ptr()+this->size()-EltCount);
	}

	void clear()
	{
		// don't free buffer on clear
		this->num_elements = 0;
	}

	inline void remove_index_fast(int Index)
	{
		dbg_assert(Index >= 0 && Index < this->size(), "Index out of bounds");
		ParentT::remove_index_fast(Index);
	}

	// keeps order, way slower
	inline void remove_index(int Index)
	{
		dbg_assert(Index >= 0 && Index < this->size(), "Index out of bounds");
		ParentT::remove_index(Index);
	}

	inline T& operator[] (int Index)
	{
		dbg_assert(Index >= 0 && Index < ParentT::size(), "Index out of bounds");
		return ParentT::operator[](Index);
	}

	inline const T& operator[] (int Index) const
	{
		dbg_assert(Index >= 0 && Index < ParentT::size(), "Index out of bounds");
		return ParentT::operator[](Index);
	}
};

// Sparse array
// - Provides indirection between ID and data pointer so IDs are not invalidated
// - Data is packed
template<typename T, u32 MAX_ELEMENTS_>
struct CSparseArray
{
	enum
	{
		MAX_ELEMENTS = MAX_ELEMENTS_,
		INVALID_ID = 0xFFFF
	};

	u16 m_ID[MAX_ELEMENTS];
	u16 m_ReverseID[MAX_ELEMENTS];
	T m_Data[MAX_ELEMENTS];
	int m_Count;

	CSparseArray()
	{
		Clear();
	}

	u32 Push(const T& elt)
	{
		dbg_assert(m_Count < MAX_ELEMENTS, "Array is full");

		for(int i = 0; i < MAX_ELEMENTS; i++)
		{
			if(m_ID[i] == INVALID_ID)
			{
				const u16 dataID = m_Count++;
				m_ID[i] = dataID;
				m_ReverseID[dataID] = i;
				m_Data[dataID] = elt;
				return i;
			}
		}

		dbg_assert(0, "Failed to push element");
		return -1;
	}

	T& Set(u32 SlotID, const T& elt)
	{
		dbg_assert(SlotID < MAX_ELEMENTS, "ID out of bounds");

		if(m_ID[SlotID] == INVALID_ID) {
			dbg_assert(m_Count < MAX_ELEMENTS, "Array is full"); // should never happen
			const u16 dataID = m_Count++;
			m_ID[SlotID] = dataID;
			m_ReverseID[dataID] = SlotID;
		}
		return m_Data[m_ID[SlotID]] = elt;
	}

	T& Get(u32 SlotID)
	{
		dbg_assert(SlotID < MAX_ELEMENTS, "ID out of bounds");
		dbg_assert(m_ID[SlotID] != INVALID_ID, "Slot is empty");
		return m_Data[m_ID[SlotID]];
	}

	bool IsValid(u32 SlotID) const
	{
		if(SlotID >= MAX_ELEMENTS) return false;
		if(m_ID[SlotID] == INVALID_ID) return false;
#ifdef CONF_DEBUG
		const u16 dataID = m_ID[SlotID];
		return m_ReverseID[dataID] == SlotID;
#endif
		return true;
	}

	u32 GetIDFromData(const T& elt) const
	{
		int64 dataID = &elt - m_Data;
		dbg_assert(dataID >= 0 && dataID < MAX_ELEMENTS, "Element out of bounds");
		return m_ReverseID[dataID];
	}

	u32 GetDataID(u32 SlotID) const
	{
		dbg_assert(SlotID < MAX_ELEMENTS, "ID out of bounds");
		dbg_assert(m_ID[SlotID] != INVALID_ID, "Slot is empty");
		return m_ID[SlotID];
	}

	void RemoveByID(u32 SlotID)
	{
		dbg_assert(SlotID < MAX_ELEMENTS, "ID out of bounds");

		if(m_ID[SlotID] != INVALID_ID) {
			dbg_assert(m_Count > 0, "Array is empty"); // should never happen
			const u16 dataID = m_ID[SlotID];

			const int lastDataID = m_Count-1;
			const u16 lastID = m_ReverseID[lastDataID];
			m_ID[lastID] = dataID;
			tl_swap(m_ReverseID[dataID], m_ReverseID[lastDataID]);
			tl_swap(m_Data[dataID], m_Data[lastDataID]);
			m_Count--;

			m_ID[SlotID] = INVALID_ID;
		}
	}

	void RemoveKeepOrderByID(u32 SlotID)
	{
		dbg_assert(SlotID < MAX_ELEMENTS, "ID out of bounds");

		if(m_ID[SlotID] != INVALID_ID) {
			dbg_assert(m_Count > 0, "Array is empty"); // should never happen


			dbg_assert(0, "Implement me");
		}
	}

	void Clear()
	{
		memset(m_ID, 0xFF, sizeof(m_ID));
		m_Count = 0;
	}

	int Count() const
	{
		return m_Count;
	}

	/*
	inline T& operator [] (u32 ID) const
	{
		return Get(ID);
	}
	*/

	T* Data()
	{
		return m_Data;
	}
};

template<typename T, u32 COUNT_>
struct CPlainArray
{
	enum {
		CAPACITY = COUNT_
	};

	T data[CAPACITY];

	inline T& operator [] (u32 ID)
	{
#ifdef CONF_DEBUG
		dbg_assert(ID < CAPACITY, "Index out of bounds");
#endif
		return data[ID];
	}
};

#define ARR_COUNT(arr) (sizeof(arr)/sizeof(arr[0]))

inline float fract(float f)
{
	return f - (int)f;
}

inline bool IsInsideRect(vec2 Pos, CUIRect Rect)
{
	return (Pos.x >= Rect.x && Pos.x < (Rect.x+Rect.w) &&
			Pos.y >= Rect.y && Pos.y < (Rect.y+Rect.h));
}

inline bool DoRectIntersect(CUIRect Rect1, CUIRect Rect2)
{
	if(Rect1.x < Rect2.x + Rect2.w &&
	   Rect1.x + Rect1.w > Rect2.x &&
	   Rect1.y < Rect2.y + Rect2.h &&
	   Rect1.h + Rect2.y > Rect2.y)
	{
		return true;
	}
	return false;
}

// increase size and zero out / construct new items
template<typename T>
inline void ArraySetSizeAndZero(array<T>* pArray, int NewSize)
{
	const int OldElementCount = pArray->size();
	if(OldElementCount >= NewSize)
		return;

	pArray->set_size(NewSize);
	const int Diff = pArray->size() - OldElementCount;
	for(int i = 0; i < Diff; i++)
		(*pArray)[i+OldElementCount] = T();
}
