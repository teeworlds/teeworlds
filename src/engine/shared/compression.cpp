/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/system.h>

#include "compression.h"

// Format: ESDDDDDD EDDDDDDD EDD... Extended, Data, Sign
unsigned char *CVariableInt::Pack(unsigned char *pDst, int i)
{
	*pDst = (i>>25)&0x40; // set sign bit if i<0
	i = i^(i>>31); // if(i<0) i = ~i

	*pDst |= i&0x3F; // pack 6bit into dst
	i >>= 6; // discard 6 bits
	if(i)
	{
		*pDst |= 0x80; // set extend bit
		while(1)
		{
			pDst++;
			*pDst = i&(0x7F); // pack 7bit
			i >>= 7; // discard 7 bits
			*pDst |= (i!=0)<<7; // set extend bit (may branch)
			if(!i)
				break;
		}
	}

	pDst++;
	return pDst;
}

const unsigned char *CVariableInt::Unpack(const unsigned char *pSrc, int *pInOut)
{
	int Sign = (*pSrc>>6)&1;
	*pInOut = *pSrc&0x3F;

	do
	{
		if(!(*pSrc&0x80)) break;
		pSrc++;
		*pInOut |= (*pSrc&(0x7F))<<(6);

		if(!(*pSrc&0x80)) break;
		pSrc++;
		*pInOut |= (*pSrc&(0x7F))<<(6+7);

		if(!(*pSrc&0x80)) break;
		pSrc++;
		*pInOut |= (*pSrc&(0x7F))<<(6+7+7);

		if(!(*pSrc&0x80)) break;
		pSrc++;
		*pInOut |= (*pSrc&(0x7F))<<(6+7+7+7);
	} while(0);

	pSrc++;
	*pInOut ^= -Sign; // if(sign) *i = ~(*i)
	return pSrc;
}


long CVariableInt::Decompress(const void *pSrc_, int SrcSize, void *pDst_, int DstSize)
{
	const unsigned char *pSrc = (unsigned char *)pSrc_;
	const unsigned char *pEnd = pSrc + SrcSize;
	int *pDst = (int *)pDst_;
	int *pDstEnd = pDst + DstSize/4;
	while(pSrc < pEnd)
	{
		if(pDst >= pDstEnd)
			return -1;
		pSrc = CVariableInt::Unpack(pSrc, pDst);
		pDst++;
	}
	return (long)((unsigned char *)pDst-(unsigned char *)pDst_);
}

long CVariableInt::Compress(const void *pSrc_, int SrcSize, void *pDst_, int DstSize)
{
	int *pSrc = (int *)pSrc_;
	unsigned char *pDst = (unsigned char *)pDst_;
	unsigned char *pDstEnd = pDst + DstSize;
	SrcSize /= 4;
	while(SrcSize)
	{
		if(pDstEnd - pDst < 6)
			return -1;
		pDst = CVariableInt::Pack(pDst, *pSrc);
		SrcSize--;
		pSrc++;
	}
	return (long)(pDst-(unsigned char *)pDst_);
}

