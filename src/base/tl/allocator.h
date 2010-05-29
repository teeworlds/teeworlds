#ifndef TL_FILE_ALLOCATOR_HPP
#define TL_FILE_ALLOCATOR_HPP

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
