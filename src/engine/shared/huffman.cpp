/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/system.h>
#include "huffman.h"

struct CHuffmanConstructNode
{
	unsigned short m_NodeId;
 	int m_Frequency;
};

void CHuffman::Setbits_r(CNode *pNode, int Bits, unsigned Depth)
{
	if(pNode->m_aLeafs[1] != 0xffff)
		Setbits_r(&m_aNodes[pNode->m_aLeafs[1]], Bits|(1<<Depth), Depth+1);
	if(pNode->m_aLeafs[0] != 0xffff)
		Setbits_r(&m_aNodes[pNode->m_aLeafs[0]], Bits, Depth+1);

	if(pNode->m_NumBits)
	{
		pNode->m_Bits = Bits;
		pNode->m_NumBits = Depth;
	}
}

// TODO: this should be something faster, but it's enough for now
static void BubbleSort(CHuffmanConstructNode **ppList, int Size)
{
	int Changed = 1;
	CHuffmanConstructNode *pTemp;

	while(Changed)
	{
		Changed = 0;
		for(int i = 0; i < Size-1; i++)
		{
			if(ppList[i]->m_Frequency < ppList[i+1]->m_Frequency)
			{
				pTemp = ppList[i];
				ppList[i] = ppList[i+1];
				ppList[i+1] = pTemp;
				Changed = 1;
			}
		}
		Size--;
	}
}

void CHuffman::ConstructTree(const unsigned *pFrequencies)
{
	CHuffmanConstructNode aNodesLeftStorage[HUFFMAN_MAX_SYMBOLS];
	CHuffmanConstructNode *apNodesLeft[HUFFMAN_MAX_SYMBOLS];
	int NumNodesLeft = HUFFMAN_MAX_SYMBOLS;

	// add the symbols
	for(int i = 0; i < HUFFMAN_MAX_SYMBOLS; i++)
	{
		m_aNodes[i].m_NumBits = 0xFFFFFFFF;
		m_aNodes[i].m_Symbol = i;
		m_aNodes[i].m_aLeafs[0] = 0xffff;
		m_aNodes[i].m_aLeafs[1] = 0xffff;

		if(i == HUFFMAN_EOF_SYMBOL)
			aNodesLeftStorage[i].m_Frequency = 1;
		else
			aNodesLeftStorage[i].m_Frequency = pFrequencies[i];
		aNodesLeftStorage[i].m_NodeId = i;
		apNodesLeft[i] = &aNodesLeftStorage[i];

	}

	m_NumNodes = HUFFMAN_MAX_SYMBOLS;

	// construct the table
	while(NumNodesLeft > 1)
	{
		// we can't rely on stdlib's qsort for this, it can generate different results on different implementations
		BubbleSort(apNodesLeft, NumNodesLeft);

		m_aNodes[m_NumNodes].m_NumBits = 0;
		m_aNodes[m_NumNodes].m_aLeafs[0] = apNodesLeft[NumNodesLeft-1]->m_NodeId;
		m_aNodes[m_NumNodes].m_aLeafs[1] = apNodesLeft[NumNodesLeft-2]->m_NodeId;
		apNodesLeft[NumNodesLeft-2]->m_NodeId = m_NumNodes;
		apNodesLeft[NumNodesLeft-2]->m_Frequency = apNodesLeft[NumNodesLeft-1]->m_Frequency + apNodesLeft[NumNodesLeft-2]->m_Frequency;

		m_NumNodes++;
		NumNodesLeft--;
	}

	// set start node
	m_pStartNode = &m_aNodes[m_NumNodes-1];

	// build symbol bits
	Setbits_r(m_pStartNode, 0, 0);
}

void CHuffman::Init(const unsigned *pFrequencies)
{
	int i;

	// make sure to cleanout every thing
	mem_zero(this, sizeof(*this));

	// construct the tree
	ConstructTree(pFrequencies);

	// build decode LUT
	for(i = 0; i < HUFFMAN_LUTSIZE; i++)
	{
		unsigned Bits = i;
		int k;
		CNode *pNode = m_pStartNode;
		for(k = 0; k < HUFFMAN_LUTBITS; k++)
		{
			pNode = &m_aNodes[pNode->m_aLeafs[Bits&1]];
			Bits >>= 1;

			if(!pNode)
				break;

			if(pNode->m_NumBits)
			{
				m_apDecodeLut[i] = pNode;
				break;
			}
		}

		if(k == HUFFMAN_LUTBITS)
			m_apDecodeLut[i] = pNode;
	}

}

//***************************************************************
int CHuffman::Compress(const void *pInput, int InputSize, void *pOutput, int OutputSize)
{
	// this macro loads a symbol for a byte into bits and bitcount
#define HUFFMAN_MACRO_LOADSYMBOL(Sym) \
	Bits |= m_aNodes[Sym].m_Bits << Bitcount; \
	Bitcount += m_aNodes[Sym].m_NumBits;

	// this macro writes the symbol stored in bits and bitcount to the dst pointer
#define HUFFMAN_MACRO_WRITE() \
	while(Bitcount >= 8) \
	{ \
		*pDst++ = (unsigned char)(Bits&0xff); \
		if(pDst == pDstEnd) \
			return -1; \
		Bits >>= 8; \
		Bitcount -= 8; \
	}

	// setup buffer pointers
	const unsigned char *pSrc = (const unsigned char *)pInput;
	const unsigned char *pSrcEnd = pSrc + InputSize;
	unsigned char *pDst = (unsigned char *)pOutput;
	unsigned char *pDstEnd = pDst + OutputSize;

	// symbol variables
	unsigned Bits = 0;
	unsigned Bitcount = 0;

	// make sure that we have data that we want to compress
	if(InputSize)
	{
		// {A} load the first symbol
		int Symbol = *pSrc++;

		while(pSrc != pSrcEnd)
		{
			// {B} load the symbol
			HUFFMAN_MACRO_LOADSYMBOL(Symbol)

			// {C} fetch next symbol, this is done here because it will reduce dependency in the code
			Symbol = *pSrc++;

			// {B} write the symbol loaded at
			HUFFMAN_MACRO_WRITE()
		}

		// write the last symbol loaded from {C} or {A} in the case of only 1 byte input buffer
		HUFFMAN_MACRO_LOADSYMBOL(Symbol)
		HUFFMAN_MACRO_WRITE()
	}

	// write EOF symbol
	HUFFMAN_MACRO_LOADSYMBOL(HUFFMAN_EOF_SYMBOL)
	HUFFMAN_MACRO_WRITE()

	// write out the last bits
	*pDst++ = Bits;

	// return the size of the output
	return (int)(pDst - (const unsigned char *)pOutput);

	// remove macros
#undef HUFFMAN_MACRO_LOADSYMBOL
#undef HUFFMAN_MACRO_WRITE
}

//***************************************************************
int CHuffman::Decompress(const void *pInput, int InputSize, void *pOutput, int OutputSize)
{
	// setup buffer pointers
	unsigned char *pDst = (unsigned char *)pOutput;
	unsigned char *pSrc = (unsigned char *)pInput;
	unsigned char *pDstEnd = pDst + OutputSize;
	unsigned char *pSrcEnd = pSrc + InputSize;

	unsigned Bits = 0;
	unsigned Bitcount = 0;

	CNode *pEof = &m_aNodes[HUFFMAN_EOF_SYMBOL];
	CNode *pNode = 0;

	while(1)
	{
		// {A} try to load a node now, this will reduce dependency at location {D}
		pNode = 0;
		if(Bitcount >= HUFFMAN_LUTBITS)
			pNode = m_apDecodeLut[Bits&HUFFMAN_LUTMASK];

		// {B} fill with new bits
		while(Bitcount < 24 && pSrc != pSrcEnd)
		{
			Bits |= (*pSrc++) << Bitcount;
			Bitcount += 8;
		}

		// {C} load symbol now if we didn't that earlier at location {A}
		if(!pNode)
			pNode = m_apDecodeLut[Bits&HUFFMAN_LUTMASK];

		if(!pNode)
			return -1;

		// {D} check if we hit a symbol already
		if(pNode->m_NumBits)
		{
			// remove the bits for that symbol
			Bits >>= pNode->m_NumBits;
			Bitcount -= pNode->m_NumBits;
		}
		else
		{
			// remove the bits that the lut checked up for us
			Bits >>= HUFFMAN_LUTBITS;
			Bitcount -= HUFFMAN_LUTBITS;

			// walk the tree bit by bit
			while(1)
			{
				// traverse tree
				pNode = &m_aNodes[pNode->m_aLeafs[Bits&1]];

				// remove bit
				Bitcount--;
				Bits >>= 1;

				// check if we hit a symbol
				if(pNode->m_NumBits)
					break;

				// no more bits, decoding error
				if(Bitcount == 0)
					return -1;
			}
		}

		// check for eof
		if(pNode == pEof)
			break;

		// output character
		if(pDst == pDstEnd)
			return -1;
		*pDst++ = pNode->m_Symbol;
	}

	// return the size of the decompressed buffer
	return (int)(pDst - (const unsigned char *)pOutput);
}
