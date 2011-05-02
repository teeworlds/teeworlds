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

	CNode m_aNodes[HUFFMAN_MAX_NODES];
	CNode *m_apDecodeLut[HUFFMAN_LUTSIZE];
	CNode *m_pStartNode;
	int m_NumNodes;

	void Setbits_r(CNode *pNode, int Bits, unsigned Depth);
	void ConstructTree(const unsigned *pFrequencies);

public:
	/*
		Function: huffman_init
			Inits the compressor/decompressor.

		Parameters:
			huff - Pointer to the state to init
			frequencies - A pointer to an array of 256 entries of the frequencies of the bytes

		Remarks:
			- Does no allocation what so ever.
			- You don't have to call any cleanup functions when you are done with it
	*/
	void Init(const unsigned *pFrequencies);

	/*
		Function: huffman_compress
			Compresses a buffer and outputs a compressed buffer.

		Parameters:
			huff - Pointer to the huffman state
			input - Buffer to compress
			input_size - Size of the buffer to compress
			output - Buffer to put the compressed data into
			output_size - Size of the output buffer

		Returns:
			Returns the size of the compressed data. Negative value on failure.
	*/
	int Compress(const void *pInput, int InputSize, void *pOutput, int OutputSize);

	/*
		Function: huffman_decompress
			Decompresses a buffer

		Parameters:
			huff - Pointer to the huffman state
			input - Buffer to decompress
			input_size - Size of the buffer to decompress
			output - Buffer to put the uncompressed data into
			output_size - Size of the output buffer

		Returns:
			Returns the size of the uncompressed data. Negative value on failure.
	*/
	int Decompress(const void *pInput, int InputSize, void *pOutput, int OutputSize);

};
#endif // __HUFFMAN_HEADER__
