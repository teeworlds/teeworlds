// copyright (c) 2010 magnus auvinen, see licence.txt for more info
#ifndef ENGINE_SHARED_COMPRESSION_H
#define ENGINE_SHARED_COMPRESSION_H
// variable int packing
class CVariableInt
{
public:
	static unsigned char *Pack(unsigned char *pDst, int i);
	static const unsigned char *Unpack(const unsigned char *pSrc, int *pInOut);
	static long Compress(const void *pSrc, int Size, void *pDst);
	static long Decompress(const void *pSrc, int Size, void *pDst);
};
#endif
