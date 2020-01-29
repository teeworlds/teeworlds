/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
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
		CHUNK_SIZE = 1024*64,
	};

	CChunk *m_pCurrent;


	void Clear();
	void NewChunk();
	void *AllocateFromChunk(unsigned int Size);

public:
	CHeap();
	~CHeap();
	void Reset();
	void *Allocate(unsigned int Size);
};
#endif
