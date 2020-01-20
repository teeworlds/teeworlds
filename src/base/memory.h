/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

/*
	Title: OS Abstraction: Memory
*/

#ifndef BASE_MEMORY_H
#define BASE_MEMORY_H

#include "detect.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
	Function: mem_alloc
		Allocates memory.

	Parameters:
		size - Size of the needed block.
		alignment - Alignment for the block.

	Returns:
		Returns a pointer to the newly allocated block. Returns a
		null pointer if the memory couldn't be allocated.

	Remarks:
		- Passing 0 to size will allocated the smallest amount possible
		and return a unique pointer.

	See Also:
		<mem_free>
*/
void *mem_alloc_debug(const char *filename, int line, unsigned size, unsigned alignment);
#define mem_alloc(s,a) mem_alloc_debug(__FILE__, __LINE__, (s), (a))

/*
	Function: mem_free
		Frees a block allocated through <mem_alloc>.

	Remarks:
		- In the debug version of the library the function will assert if
		a non-valid block is passed, like a null pointer or a block that
		isn't allocated.

	See Also:
		<mem_alloc>
*/
void mem_free(void *block);

/*
	Function: mem_copy
		Copies a a memory block.

	Parameters:
		dest - Destination.
		source - Source to copy.
		size - Size of the block to copy.

	Remarks:
		- This functions DOES NOT handles cases where source and
		destination is overlapping.

	See Also:
		<mem_move>
*/
void mem_copy(void *dest, const void *source, unsigned size);

/*
	Function: mem_move
		Copies a a memory block

	Parameters:
		dest - Destination
		source - Source to copy
		size - Size of the block to copy

	Remarks:
		- This functions handles cases where source and destination
		is overlapping

	See Also:
		<mem_copy>
*/
void mem_move(void *dest, const void *source, unsigned size);

/*
	Function: mem_zero
		Sets a complete memory block to 0

	Parameters:
		block - Pointer to the block to zero out
		size - Size of the block
*/
void mem_zero(void *block, unsigned size);

/*
	Function: mem_comp
		Compares two blocks of memory

	Parameters:
		a - First block of data
		b - Second block of data
		size - Size of the data to compare

	Returns:
		<0 - Block a is lesser then block b
		0 - Block a is equal to block b
		>0 - Block a is greater then block b
*/
int mem_comp(const void *a, const void *b, int size);

#ifdef __cplusplus
}
#endif

#endif
