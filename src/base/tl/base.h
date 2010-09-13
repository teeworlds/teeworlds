// copyright (c) 2010 magnus auvinen, see licence.txt for more info
#ifndef TL_FILE_BASE_HPP
#define TL_FILE_BASE_HPP

#include <base/system.h>

inline void assert(bool statement)
{
	dbg_assert(statement, "assert!");
}

template<class T>
inline void swap(T &a, T &b)
{
	T c = b;
	b = a;
	a = c;
}

#endif
