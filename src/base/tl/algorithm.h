/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef BASE_TL_ALGORITHM_H
#define BASE_TL_ALGORITHM_H

#include "array.h"
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
	concept_forwarditeration::check(range);
	concept_backwarditeration::check(range);

	// slow bubblesort :/
	typename R::type *front = &range.front();
	typename R::type *back = &range.back();
	for(; back > front+1; back--)
	{
		//R section = range;
		typename R::type *prev = front;
		//section.pop_front();
		for(; prev < back; prev++)
		{
			typename R::type *cur = prev+1;
			if(*cur < *prev)
				swap(*cur, *prev);
			//prev = cur;
		}
	}
}

template<class R>
void sort_quick(R range)
{
	concept_forwarditeration::check(range);

	int i = 0;
	int left = 0;
	int right = 0;
	int start[300];
	int end[300];
	typename R::type *front = &range.front();
	typename R::type pivot;
	typename R::type *eleft;
	typename R::type *eright;

	start[0] = 0;
	end[0] = range.size();
	while(i >= 0)
	{
		left = start[i];
		right = end[i]-1;
		if(left < right)
		{
			pivot = *(front+left);
			eleft = front+left;
			eright = front+right;
			while(left < right)
			{
				while(!(*eright < pivot) && left < right)
				{
					right--;
					eright--;
				}
				if(left < right)
				{
					left++;
					*eleft = *eright;
					eleft++;
				}
				while(!(pivot < *eleft) && left < right)
				{
					left++;
					eleft++;
				}
				if(left < right)
				{
					right--;
					*eright = *eleft;
					eright--;
				}
			}

			*eleft = pivot;
			start[i+1] = left+1;
			end[i+1] = end[i];
			end[i++] = left;

			// make sure to handle the smaller partition first
			if(end[i]-start[i] > end[i-1]-start[i-1])
			{
				swap(start[i], start[i-1]);
				swap(end[i], end[i-1]);
			}
		}
		else
			i--;
	}
}

template<class R>
void sort_heap(R range)
{
	concept_index::check(range);
	concept_forwarditeration::check(range);
	concept_backwarditeration::check(range);

	// build heap
	int size = range.size();
	typename R::type *ipe; // for inner loop
	typename R::type *front = &range.front();
	typename R::type *back = &range.back();
	typename R::type *pe;
	typename R::type *re;
	for(pe = front+(size/2-1); pe >= front; pe--)
	{
		ipe = pe;
		while(1)
		{
			re = front+((ipe-front)*2+1);
			if(re > back)
				break;

			if(re+1 <= back)
			{
				if(*re < *(re+1))
					re++;
			}

			if(*re < *ipe)
				break;

			swap(*re, *ipe);
			ipe = re;
		}
	}

	// do the final sorting
	typename R::type *ire; // for inner loop
	for(re = front+size-1; re > front; re--)
	{
		pe = front;
		swap(*front, *re);
		while(1)
		{
			ire = front+((pe-front)*2+1);
			if(ire >= re)
				break;

			if(ire+1 < re)
			{
				if(*ire < *(ire+1))
					ire++;
			}

			if(*ire < *pe)
				break;

			swap(*ire, *pe);
			pe = ire;
		}
	}
}

template<class R>
void sort_merge(R range)
{
	concept_forwarditeration::check(range);
	concept_backwarditeration::check(range);

	array<typename R::type> temp;
	int size = range.size();
	temp.set_size(size/2);
	array<typename R::type>::range rtemp = temp.all();
	typename R::type *front = &range.front();
	typename R::type *tfront = &rtemp.front();
	typename R::type *back = &range.back();
	for(int s = 1; s < size; s += s)
	{
		for(typename R::type *m = back-1-s; m >= front; m -= s+s)
		{
			typename R::type *j;
			if(m-s+1 < front)
				j = front;
			else
				j = m-s+1;
			typename R::type *b = tfront;
			while(j <= m)
			{
				*b = *j;
				b++;
				j++;
			}

			typename R::type *k;
			if(m-s+1 < front)
				k = front;
			else
				k = m-s+1;

			b = tfront;
			while(k < j && j <= m+s)
			{
				if(!(*j < *b))
				{
					*k = *b;
					k++;
					b++;
				}
				else
				{
					*k = *j;
					j++;
					k++;
				}
			}

			while(k < j)
			{
				*k = *b;
				b++;
				k++;
			}
		}
	}
}

template<class R>
void sort_shell(R range)
{
	concept_forwarditeration::check(range);
	concept_backwarditeration::check(range);
	
	const int incs[] = {
		1, 3, 7, 21, 48, 112,
		336, 861, 1968, 4592, 13776,
		33936, 86961, 198768, 463792, 1391376,
		3402672, 8382192, 21479367, 49095696, 114556624,
		343669872, 52913488, 2085837936};
	typename R::type *front = &range.front();
	typename R::type *back = &range.back();
	for(int l = sizeof(incs)/sizeof(incs[0]); l > 0;)
	{
		const int m = incs[--l];
		for(typename R::type *i = front+m; i < back; i++)
		{
			typename R::type *j = i-m;
			if(*i < *j)
			{
				typename R::type temp = *i;
				do
				{
					*(j+m) = *j;
					j -= m;
				} while(j >= front && temp < *j);
				*(j+m) = temp;
			}
		}
	}
}

template<class R>
void sort(R range)
{
	sort_shell(range);
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
