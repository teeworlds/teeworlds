// copyright (c) 2010 magnus auvinen, see licence.txt for more info
#ifndef ENGINE_SHARED_MEMHEAP_H
#define ENGINE_SHARED_MEMHEAP_H
class CHeap
{
	struct CChunk
	{
		char *m_pMemory;
		char *m_pCurrent;
		char *m_pEnd;
		CChunk *m_pNext;
	};
	
	enum
	{
		// how large each chunk should be
		CHUNK_SIZE = 1025*64,
	};
	
	CChunk *m_pCurrent;
	
	
	void Clear();
	void NewChunk();
	void *AllocateFromChunk(unsigned int Size);
	
public:
	CHeap();
	~CHeap();
	void Reset();
	void *Allocate(unsigned Size);
};
#endif
