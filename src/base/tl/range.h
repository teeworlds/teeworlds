/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef BASE_TL_RANGE_H
#define BASE_TL_RANGE_H

#include "base.h"

/*
	Group: Range concepts
*/

/*
	Concept: concept_empty

		template<class T>
		struct range
		{
			bool empty() const;
		};
*/
struct concept_empty
{
	template<typename T> static void check(T &t) { if(0) t.empty(); };
};

/*
	Concept: concept_index

		template<class T>
		struct range
		{
			T &index(size_t);
		};
*/
struct concept_index
{
	template<typename T> static void check(T &t) { if(0) t.index(0); };
};

/*
	Concept: concept_size

		template<class T>
		struct range
		{
			size_t size();
		};
*/
struct concept_size
{
	template<typename T> static void check(T &t) { if(0) t.size(); };
};

/*
	Concept: concept_slice

		template<class T>
		struct range
		{
			range slice(size_t start, size_t count);
		};
*/
struct concept_slice
{
	template<typename T> static void check(T &t) { if(0) t.slice(0, 0); };
};

/*
	Concept: concept_sorted

		template<class T>
		struct range
		{
			void sorted();
		};
*/
struct concept_sorted
{
	template<typename T> static void check(T &t) { if(0) t.sorted(); };
};

/*
	Concept: concept_forwarditeration
		Checks for the front and pop_front methods

		template<class T>
		struct range
		{
			void pop_front();
			T &front() const;
		};
*/
struct concept_forwarditeration
{
	template<typename T> static void check(T &t) { if(0) { t.front(); t.pop_front(); } };
};

/*
	Concept: concept_backwarditeration
		Checks for the back and pop_back methods

		template<class T>
		struct range
		{
			void pop_back();
			T &back() const;
		};
*/
struct concept_backwarditeration
{
	template<typename T> static void check(T &t) { if(0) { t.back(); t.pop_back(); } };
};


/*
	Group: Range classes
*/


/*
	Class: plain_range

	Concepts:
		<concept_empty>
		<concept_index>
		<concept_slice>
		<concept_forwardinteration>
		<concept_backwardinteration>
*/
template<class T>
class plain_range
{
public:
	typedef T type;
	plain_range()
	{
		begin = 0x0;
		end = 0x0;
	}

	plain_range(const plain_range &r)
	{
		*this = r;
	}

	plain_range(T *b, T *e)
	{
		begin = b;
		end = e;
	}

	bool empty() const { return begin >= end; }
	void pop_front() { assert(!empty()); begin++; }
	void pop_back() { assert(!empty()); end--; }
	T& front() { assert(!empty()); return *begin; }
	T& back() { assert(!empty()); return *(end-1); }
	T& index(unsigned i) { assert(i >= 0 && i < (unsigned)(end-begin)); return begin[i]; }
	unsigned size() const { return (unsigned)(end-begin); }
	plain_range slice(unsigned startindex, unsigned endindex)
	{
		return plain_range(begin+startindex, begin+endindex);
	}

protected:
	T *begin;
	T *end;
};

/*
	Class: plain_range_sorted

	Concepts:
		Same as <plain_range> but with these additions:
		<concept_sorted>
*/
template<class T>
class plain_range_sorted : public plain_range<T>
{
	typedef plain_range<T> parent;
public:
	/* sorted concept */
	void sorted() const { }

	plain_range_sorted()
	{}

	plain_range_sorted(const plain_range_sorted &r)
	{
		*this = r;
	}

	plain_range_sorted(T *b, T *e)
	: parent(b, e)
	{}

	plain_range_sorted slice(unsigned start, unsigned count)
	{
		return plain_range_sorted(parent::begin+start, parent::begin+start+count);
	}
};

template<class R>
class reverse_range
{
private:
	reverse_range() {}
public:
	typedef typename R::type type;

	reverse_range(R r)
	{
		range = r;
	}

	reverse_range(const reverse_range &other) { range = other.range; }


	bool empty() const { return range.empty(); }
	void pop_front() { range.pop_back(); }
	void pop_back() { range.pop_front(); }
	type& front() { return range.back(); }
	type& back() { return range.front(); }

	R range;
};

template<class R> reverse_range<R> reverse(R range) {
	return reverse_range<R>(range);
}
template<class R> R reverse(reverse_range<R> range) {
	return range.range;
}

#endif // TL_FILE_RANGE_HPP
