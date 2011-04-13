/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef BASE_TL_SORTED_ARRAY_H
#define BASE_TL_SORTED_ARRAY_H

#include "algorithm.h"
#include "array.h"

template <class T, class ALLOCATOR = allocator_default<T> >
class sorted_array : public array<T, ALLOCATOR>
{
	typedef array<T, ALLOCATOR> parent;

	// insert and size is not allowed
	int insert(const T& item, typename parent::range r) { dbg_break(); return 0; }
	int set_size(int new_size) { dbg_break(); return 0; }

public:
	typedef plain_range_sorted<T> range;

	int add(const T& item)
	{
		return parent::insert(item, partition_binary(all(), item));
	}

	int add_unsorted(const T& item)
	{
		return parent::add(item);
	}

	void sort_range()
	{
		sort(all());
	}


	/*
		Function: all
			Returns a sorted range that contains the whole array.
	*/
	range all() { return range(parent::list, parent::list+parent::num_elements); }
};

#endif // TL_FILE_SORTED_ARRAY_HPP
