/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef BASE_TL_STRING_H
#define BASE_TL_STRING_H

#include "base.h"
#include "allocator.h"

template<class ALLOCATOR >
class string_base : private ALLOCATOR
{
	char *str;
	int length;

	void reset()
	{
		str = 0; length = 0;
	}

	void free()
	{
		ALLOCATOR::free_array(str);
		reset();
	}

	void copy(const char *other_str, int other_length)
	{
		length = other_length;
		str = ALLOCATOR::alloc_array(length+1);
		mem_copy(str, other_str, length+1);
	}

	void copy(const string_base &other)
	{
		if(!other.str)
			return;
		copy(other.str, other.length);
	}

public:
	string_base() { reset(); }
	string_base(const char *other_str) { copy(other_str, str_length(other_str)); }
	string_base(const string_base &other) { reset(); copy(other); }
	~string_base() { free(); }

	string_base &operator = (const char *other)
	{
		free();
		if(other)
			copy(other, str_length(other));
		return *this;
	}

	string_base &operator = (const string_base &other)
	{
		free();
		copy(other);
		return *this;
	}

	bool operator < (const char *other_str) const { return str_comp(str, other_str) < 0; }
	operator const char *() const { return str; }

	const char *cstr() const { return str; }
};

/* normal allocated string */
typedef string_base<allocator_default<char> > string;

#endif // TL_FILE_STRING_HPP
