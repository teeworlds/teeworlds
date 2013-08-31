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


	/*
		Function: sort_range()
			sorts the whole array using the default sort method (shell sort)
	*/
	void sort_range()
	{
		sort(all());
	}

	/*
		Function: sort_range_quick()
			sorts the whole array using quick sort
	*/
	void sort_range_quick()
	{
		sort_quick(all());
	}

	/*
		Function: sort_range_heap()
			sorts the whole array using heap sort
	*/
	void sort_range_heap()
	{
		sort_heap(all());
	}

	/*
		Function: sort_range_merge()
			sorts the whole array using merge sort (stable)
	*/
	void sort_range_merge()
	{
		sort_merge(all());
	}

	/*
		Function: sort_range_shell()
			sorts the whole array using shell sort
	*/
	void sort_range_shell()
	{
		sort_shell(all());
	}

	/*
		Function: sort_range_bubble()
			sorts the whole array using bubble sort (stable)
	*/
	void sort_range_bubble()
	{
		sort_bubble(all());
	}

	/*
		Function: all
			Returns a sorted range that contains the whole array.
	*/
	range all() { return range(parent::list, parent::list+parent::num_elements); }
};

#endif // TL_FILE_SORTED_ARRAY_HPP
