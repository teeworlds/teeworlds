/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/system.h>

#include "ringbuffer.h"

CRingBufferBase::CItem *CRingBufferBase::NextBlock(CItem *pItem)
{
	if(pItem->m_pNext)
		return pItem->m_pNext;
	return m_pFirst;
}

CRingBufferBase::CItem *CRingBufferBase::PrevBlock(CItem *pItem)
{
	if(pItem->m_pPrev)
		return pItem->m_pPrev;
	return m_pLast;
}

CRingBufferBase::CItem *CRingBufferBase::MergeBack(CItem *pItem)
{
	// make sure that this block and previous block is free
	if(!pItem->m_Free || !pItem->m_pPrev || !pItem->m_pPrev->m_Free)
		return pItem;

	// merge the blocks
	pItem->m_pPrev->m_Size += pItem->m_Size;
	pItem->m_pPrev->m_pNext = pItem->m_pNext;

	// fixup pointers
	if(pItem->m_pNext)
		pItem->m_pNext->m_pPrev = pItem->m_pPrev;
	else
		m_pLast = pItem->m_pPrev;

	if(pItem == m_pProduce)
		m_pProduce = pItem->m_pPrev;

	if(pItem == m_pConsume)
		m_pConsume = pItem->m_pPrev;

	// return the current block
	return pItem->m_pPrev;
}

void CRingBufferBase::Init(void *pMemory, int Size, int Flags)
{
	mem_zero(pMemory, Size);
	m_Size = (Size)/sizeof(CItem)*sizeof(CItem);
	m_pFirst = (CItem *)pMemory;
	m_pFirst->m_Free = 1;
	m_pFirst->m_Size = m_Size;
	m_pLast = m_pFirst;
	m_pProduce = m_pFirst;
	m_pConsume = m_pFirst;
	m_Flags = Flags;

}

void *CRingBufferBase::Allocate(int Size)
{
	int WantedSize = (Size+sizeof(CItem)+sizeof(CItem)-1)/sizeof(CItem)*sizeof(CItem);
	CItem *pBlock = 0;

	// check if we even can fit this block
	if(WantedSize > m_Size)
		return 0;

	while(1)
	{
		// check for space
		if(m_pProduce->m_Free)
		{
			if(m_pProduce->m_Size >= WantedSize)
				pBlock = m_pProduce;
			else
			{
				// wrap around to try to find a block
				if(m_pFirst->m_Free && m_pFirst->m_Size >= WantedSize)
					pBlock = m_pFirst;
			}
		}

		if(pBlock)
			break;
		else
		{
			// we have no block, check our policy and see what todo
			if(m_Flags&FLAG_RECYCLE)
			{
				if(!PopFirst())
					return 0;
			}
			else
				return 0;
		}
	}

	// okey, we have our block

	// split the block if needed
	if(pBlock->m_Size > WantedSize+(int)sizeof(CItem))
	{
		CItem *pNewItem = (CItem *)((char *)pBlock + WantedSize);
		pNewItem->m_pPrev = pBlock;
		pNewItem->m_pNext = pBlock->m_pNext;
		if(pNewItem->m_pNext)
			pNewItem->m_pNext->m_pPrev = pNewItem;
		pBlock->m_pNext = pNewItem;

		pNewItem->m_Free = 1;
		pNewItem->m_Size = pBlock->m_Size - WantedSize;
		pBlock->m_Size = WantedSize;

		if(!pNewItem->m_pNext)
			m_pLast = pNewItem;
	}


	// set next block
	m_pProduce = NextBlock(pBlock);

	// set as used and return the item pointer
	pBlock->m_Free = 0;
	return (void *)(pBlock+1);
}

int CRingBufferBase::PopFirst()
{
	if(m_pConsume->m_Free)
		return 0;

	// set the free flag
	m_pConsume->m_Free = 1;

	// previous block is also free, merge them
	m_pConsume = MergeBack(m_pConsume);

	// advance the consume pointer
	m_pConsume = NextBlock(m_pConsume);
	while(m_pConsume->m_Free && m_pConsume != m_pProduce)
	{
		m_pConsume = MergeBack(m_pConsume);
		m_pConsume = NextBlock(m_pConsume);
	}

	// in the case that we have catched up with the produce pointer
	// we might stand on a free block so merge em
	MergeBack(m_pConsume);
	return 1;
}


void *CRingBufferBase::Prev(void *pCurrent)
{
	CItem *pItem = ((CItem *)pCurrent) - 1;

	while(1)
	{
		pItem = PrevBlock(pItem);
		if(pItem == m_pProduce)
			return 0;
		if(!pItem->m_Free)
			return pItem+1;
	}
}

void *CRingBufferBase::Next(void *pCurrent)
{
	CItem *pItem = ((CItem *)pCurrent) - 1;

	while(1)
	{
		pItem = NextBlock(pItem);
		if(pItem == m_pProduce)
			return 0;
		if(!pItem->m_Free)
			return pItem+1;
	}
}

void *CRingBufferBase::First()
{
	if(m_pConsume->m_Free)
		return 0;
	return (void *)(m_pConsume+1);
}

void *CRingBufferBase::Last()
{
	return Prev(m_pProduce+1);
}

