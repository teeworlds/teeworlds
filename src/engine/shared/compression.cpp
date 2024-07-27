/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/system.h>

#include "compression.h"

// Format: ESDDDDDD EDDDDDDD EDD... Extended, Data, Sign
unsigned char *CVariableInt::Pack(unsigned char *pDst, int i, int DstSize)
{
	if(DstSize <= 0)
		return 0;

	DstSize--;
	*pDst = 0;
	if(i < 0)
	{
		*pDst |= 0x40; // set sign bit
		i = ~i;
	}

	*pDst |= i & 0x3F; // pack 6bit into dst
	i >>= 6; // discard 6 bits
	while(i)
	{
		if(DstSize <= 0)
			return 0;
		*pDst |= 0x80; // set extend bit
		DstSize--;
		pDst++;
		*pDst = i & 0x7F; // pack 7bit
		i >>= 7; // discard 7 bits
	}

	pDst++;
	return pDst;
}

const unsigned char *CVariableInt::Unpack(const unsigned char *pSrc, int *pInOut, int SrcSize)
{
	if(SrcSize <= 0)
		return 0;

	const int Sign = (*pSrc >> 6) & 1;
	*pInOut = *pSrc & 0x3F;
	SrcSize--;

	const static int s_aMasks[] = {0x7F, 0x7F, 0x7F, 0x0F};
	const static int s_aShifts[] = {6, 6 + 7, 6 + 7 + 7, 6 + 7 + 7 + 7};

	for(unsigned i = 0; i < sizeof(s_aMasks) / sizeof(int); i++)
	{
		if(!(*pSrc & 0x80))
			break;
		if(SrcSize <= 0)
			return 0;
		SrcSize--;
		pSrc++;
		*pInOut |= (*pSrc & s_aMasks[i]) << s_aShifts[i];
	}

	pSrc++;
	*pInOut ^= -Sign; // if(sign) *i = ~(*i)
	return pSrc;
}

long CVariableInt::Decompress(const void *pSrc_, int SrcSize, void *pDst_, int DstSize)
{
	dbg_assert(DstSize % sizeof(int) == 0, "invalid bounds");

	const unsigned char *pSrc = (unsigned char *)pSrc_;
	const unsigned char *pSrcEnd = pSrc + SrcSize;
	int *pDst = (int *)pDst_;
	const int *pDstEnd = pDst + DstSize / sizeof(int);
	while(pSrc < pSrcEnd)
	{
		if(pDst >= pDstEnd)
			return -1;
		pSrc = CVariableInt::Unpack(pSrc, pDst, pSrcEnd - pSrc);
		if(!pSrc)
			return -1;
		pDst++;
	}
	return (long)((unsigned char *)pDst - (unsigned char *)pDst_);
}

long CVariableInt::Compress(const void *pSrc_, int SrcSize, void *pDst_, int DstSize)
{
	dbg_assert(SrcSize % sizeof(int) == 0, "invalid bounds");

	const int *pSrc = (int *)pSrc_;
	unsigned char *pDst = (unsigned char *)pDst_;
	const unsigned char *pDstEnd = pDst + DstSize;
	SrcSize /= sizeof(int);
	while(SrcSize)
	{
		pDst = CVariableInt::Pack(pDst, *pSrc, pDstEnd - pDst);
		if(!pDst)
			return -1;
		SrcSize--;
		pSrc++;
	}
	return (long)(pDst - (unsigned char *)pDst_);
}

