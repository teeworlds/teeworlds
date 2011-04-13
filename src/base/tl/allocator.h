/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef BASE_TL_ALLOCATOR_H
#define BASE_TL_ALLOCATOR_H

template <class T>
class allocator_default
{
public:
	static T *alloc() { return new T; }
	static void free(T *p) { delete p; }

	static T *alloc_array(int size) { return new T [size]; }
	static void free_array(T *p) { delete [] p; }
};

#endif // TL_FILE_ALLOCATOR_HPP
