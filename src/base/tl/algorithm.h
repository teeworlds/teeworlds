/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef BASE_TL_ALGORITHM_H
#define BASE_TL_ALGORITHM_H

#include "range.h"


/*
	insert 4
		v
	1 2 3 4 5 6

*/


template<class R, class T>
R partition_linear(R range, T value)
{
	concept_empty::check(range);
	concept_forwarditeration::check(range);
	concept_sorted::check(range);

	for(; !range.empty(); range.pop_front())
	{
		if(!(range.front() < value))
			return range;
	}
	return range;
}


template<class R, class T>
R partition_binary(R range, T value)
{
	concept_empty::check(range);
	concept_index::check(range);
	concept_size::check(range);
	concept_slice::check(range);
	concept_sorted::check(range);

	if(range.empty())
		return range;
	if(range.back() < value)
		return R();

	while(range.size() > 1)
	{
		unsigned pivot = (range.size()-1)/2;
		if(range.index(pivot) < value)
			range = range.slice(pivot+1, range.size()-1);
		else
			range = range.slice(0, pivot+1);
	}
	return range;
}

template<class R, class T>
R find_linear(R range, T value)
{
	concept_empty::check(range);
	concept_forwarditeration::check(range);
	for(; !range.empty(); range.pop_front())
		if(value == range.front())
			break;
	return range;
}

template<class R, class T>
R find_binary(R range, T value)
{
	range = partition_linear(range, value);
	if(range.empty()) return range;
	if(range.front() == value) return range;
	return R();
}


template<class R>
void sort_bubble(R range)
{
	concept_empty::check(range);
	concept_forwarditeration::check(range);
	concept_backwarditeration::check(range);

	// slow bubblesort :/
	for(; !range.empty(); range.pop_back())
	{
		R section = range;
		typename R::type *prev = &section.front();
		section.pop_front();
		for(; !section.empty(); section.pop_front())
		{
			typename R::type *cur = &section.front();
			if(*cur < *prev)
				swap(*cur, *prev);
			prev = cur;
		}
	}
}

/*
template<class R>
void sort_quick(R range)
{
	concept_index::check(range);
}*/


template<class R>
void sort(R range)
{
	sort_bubble(range);
}


template<class R>
bool sort_verify(R range)
{
	concept_empty::check(range);
	concept_forwarditeration::check(range);

	typename R::type *prev = &range.front();
	range.pop_front();
	for(; !range.empty(); range.pop_front())
	{
		typename R::type *cur = &range.front();

		if(*cur < *prev)
			return false;
		prev = cur;
	}

	return true;
}

#endif // TL_FILE_ALGORITHMS_HPP
