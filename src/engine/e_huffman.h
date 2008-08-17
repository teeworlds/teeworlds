#ifndef __HUFFMAN_HEADER__
#define __HUFFMAN_HEADER__

#ifdef __cplusplus
extern "C" {
#endif

enum
{
	HUFFMAN_EOF_SYMBOL = 256,

	HUFFMAN_MAX_SYMBOLS=HUFFMAN_EOF_SYMBOL+1,
	HUFFMAN_MAX_NODES=HUFFMAN_MAX_SYMBOLS*2-1,
	
	HUFFMAN_LUTBITS = 10,
	HUFFMAN_LUTSIZE = (1<<HUFFMAN_LUTBITS),
	HUFFMAN_LUTMASK = (HUFFMAN_LUTSIZE-1)
};

typedef struct HUFFMAN_NODE
{
	/* symbol */
	unsigned bits;
	unsigned num_bits;

	/* don't use pointers for this. shorts are smaller so we can fit more data into the cache */
	unsigned short leafs[2];

	/* what the symbol represents */
	unsigned char symbol;
} HUFFMAN_NODE;

typedef struct HUFFMAN_STATE
{
	HUFFMAN_NODE nodes[HUFFMAN_MAX_NODES];
	HUFFMAN_NODE *decode_lut[HUFFMAN_LUTSIZE];
	HUFFMAN_NODE *start_node;
	int num_nodes;
} HUFFMAN_STATE;

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
void huffman_init(HUFFMAN_STATE *huff, const unsigned *frequencies);

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
int huffman_compress(HUFFMAN_STATE *huff, const void *input, int input_size, void *output, int output_size);

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
int huffman_decompress(HUFFMAN_STATE *huff, const void *input, int input_size, void *output, int output_size);

#ifdef __cplusplus
}
#endif

#endif /* __HUFFMAN_HEADER__ */
