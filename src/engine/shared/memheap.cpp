/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/system.h>
#include "memheap.h"


// allocates a new chunk to be used
void CHeap::NewChunk()
{
	// allocate memory
	char *pMem = (char*)mem_alloc(sizeof(CChunk)+CHUNK_SIZE, 1);
	if(!pMem)
		return;

	// the chunk structure is located in the begining of the chunk
	// init it and return the chunk
	CChunk *pChunk = (CChunk*)pMem;
	pChunk->m_pMemory = (char*)(pChunk+1);
	pChunk->m_pCurrent = pChunk->m_pMemory;
	pChunk->m_pEnd = pChunk->m_pMemory + CHUNK_SIZE;
	pChunk->m_pNext = m_pCurrent;

	m_pCurrent = pChunk;
}

//****************
void *CHeap::AllocateFromChunk(unsigned int Size)
{
	// check if we need can fit the allocation
	if(m_pCurrent->m_pCurrent + Size > m_pCurrent->m_pEnd)
		return (void*)0x0;

	// get memory and move the pointer forward
	char *pMem = m_pCurrent->m_pCurrent;
	m_pCurrent->m_pCurrent += Size;
	return pMem;
}

// creates a heap
CHeap::CHeap()
{
	m_pCurrent = 0x0;
	NewChunk();
}

CHeap::~CHeap()
{
	Clear();
}

void CHeap::Reset()
{
	Clear();
	NewChunk();
}

// destroys the heap
void CHeap::Clear()
{
	CChunk *pChunk = m_pCurrent;

	while(pChunk)
	{
		CChunk *pNext = pChunk->m_pNext;
		mem_free(pChunk);
		pChunk = pNext;
	}

	m_pCurrent = 0x0;
}

//
void *CHeap::Allocate(unsigned int Size)
{
	// try to allocate from current chunk
	char *pMem = (char *)AllocateFromChunk(Size);
	if(!pMem)
	{
		// allocate new chunk and add it to the heap
		NewChunk();

		// try to allocate again
		pMem = (char *)AllocateFromChunk(Size);
	}

	return pMem;
}
