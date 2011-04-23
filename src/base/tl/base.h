/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef BASE_TL_BASE_H
#define BASE_TL_BASE_H

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
