/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_SHARED_COMPRESSION_H
#define ENGINE_SHARED_COMPRESSION_H

// variable int packing
class CVariableInt
{
public:
	enum
	{
		MAX_BYTES_PACKED = 5, // maximum number of bytes in a packed int
	};

	static unsigned char *Pack(unsigned char *pDst, int i, int DstSize);
	static const unsigned char *Unpack(const unsigned char *pSrc, int *pInOut, int SrcSize);

	static long Compress(const void *pSrc, int SrcSize, void *pDst, int DstSize);
	static long Decompress(const void *pSrc, int SrcSize, void *pDst, int DstSize);
};

#endif
