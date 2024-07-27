/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_SHARED_HUFFMAN_H
#define ENGINE_SHARED_HUFFMAN_H

class CHuffman
{
	enum
	{
		HUFFMAN_EOF_SYMBOL = 256,

		HUFFMAN_MAX_SYMBOLS=HUFFMAN_EOF_SYMBOL+1,
		HUFFMAN_MAX_NODES=HUFFMAN_MAX_SYMBOLS*2-1,

		HUFFMAN_LUTBITS = 10,
		HUFFMAN_LUTSIZE = (1<<HUFFMAN_LUTBITS),
		HUFFMAN_LUTMASK = (HUFFMAN_LUTSIZE-1)
	};

	struct CNode
	{
		// symbol
		unsigned m_Bits;
		unsigned m_NumBits;

		// don't use pointers for this. shorts are smaller so we can fit more data into the cache
		unsigned short m_aLeafs[2];

		// what the symbol represents
		unsigned char m_Symbol;
	};

	static const unsigned ms_aFreqTable[HUFFMAN_MAX_SYMBOLS];

	CNode m_aNodes[HUFFMAN_MAX_NODES];
	CNode *m_apDecodeLut[HUFFMAN_LUTSIZE];
	CNode *m_pStartNode;
	int m_NumNodes;

	void Setbits_r(CNode *pNode, int Bits, unsigned Depth);
	void ConstructTree(const unsigned *pFrequencies);

public:
	/*
		Function: Init
			Inits the compressor/decompressor.

		Parameters:
			pFrequencies - A pointer to an array of 256 entries of the frequencies of the bytes

		Remarks:
			- Does no allocation whatsoever.
			- You don't have to call any cleanup functions when you are done with it.
	*/
	void Init(const unsigned *pFrequencies = ms_aFreqTable);

	/*
		Function: Compress
			Compresses a buffer and outputs a compressed buffer.

		Parameters:
			pInput - Buffer to compress
			InputSize - Size of the buffer to compress
			pOutput - Buffer to put the compressed data into
			OutputSize - Size of the output buffer

		Returns:
			Returns the size of the compressed data. Negative value on failure.
	*/
	int Compress(const void *pInput, int InputSize, void *pOutput, int OutputSize) const;

	/*
		Function: Decompress
			Decompresses a buffer

		Parameters:
			pInput - Buffer to decompress
			InputSize - Size of the buffer to decompress
			pOutput - Buffer to put the uncompressed data into
			OutputSize - Size of the output buffer

		Returns:
			Returns the size of the uncompressed data. Negative value on failure.
	*/
	int Decompress(const void *pInput, int InputSize, void *pOutput, int OutputSize) const;
};
#endif // ENGINE_SHARED_HUFFMAN_H
