/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_SHARED_COMPRESSION_H
#define ENGINE_SHARED_COMPRESSION_H
// variable int packing
class CVariableInt
{
public:
	static unsigned char *Pack(unsigned char *pDst, int i);
	static const unsigned char *Unpack(const unsigned char *pSrc, int *pInOut);
	static long Compress(const void *pSrc, int Size, void *pDst, int DstSize);
	static long Decompress(const void *pSrc, int Size, void *pDst, int DstSize);
};
#endif
