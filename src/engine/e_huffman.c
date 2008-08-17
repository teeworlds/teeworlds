#include <stdlib.h> /* qsort */
#include <memory.h> /* memset */
#include "e_huffman.h"

typedef struct HUFFMAN_CONSTRUCT_NODE
{
	unsigned short node_id;
 	int frequency;
} HUFFMAN_CONSTRUCT_NODE;

static int sort_func(const void *a, const void *b)
{
	if((*(HUFFMAN_CONSTRUCT_NODE **)a)->frequency > (*(HUFFMAN_CONSTRUCT_NODE **)b)->frequency)
		return -1;
	if((*(HUFFMAN_CONSTRUCT_NODE **)a)->frequency < (*(HUFFMAN_CONSTRUCT_NODE **)b)->frequency)
		return 1;
	return 0;
}

static void huffman_setbits_r(HUFFMAN_STATE *huff, HUFFMAN_NODE *node, int bits, int depth)
{
	if(node->leafs[1] != 0xffff)
		huffman_setbits_r(huff, &huff->nodes[node->leafs[1]], bits|(1<<depth), depth+1);
	if(node->leafs[0] != 0xffff)
		huffman_setbits_r(huff, &huff->nodes[node->leafs[0]], bits, depth+1);
		
	if(node->num_bits)
	{
		node->bits = bits;
		node->num_bits = depth;
	}
}

static void huffman_construct_tree(HUFFMAN_STATE *huff, const unsigned *frequencies)
{
	HUFFMAN_CONSTRUCT_NODE nodes_left_storage[HUFFMAN_MAX_NODES];
	HUFFMAN_CONSTRUCT_NODE *nodes_left[HUFFMAN_MAX_NODES];
	int num_nodes_left = HUFFMAN_MAX_SYMBOLS;
	int i;

	/* add the symbols */
	for(i = 0; i < HUFFMAN_MAX_SYMBOLS; i++)
	{
		huff->nodes[i].num_bits = -1;
		huff->nodes[i].symbol = i;
		huff->nodes[i].leafs[0] = -1;
		huff->nodes[i].leafs[1] = -1;

		if(i == HUFFMAN_EOF_SYMBOL)
			nodes_left_storage[i].frequency = 1;
		else
			nodes_left_storage[i].frequency = frequencies[i];
		nodes_left_storage[i].node_id = i;
		nodes_left[i] = &nodes_left_storage[i];
	}

	huff->num_nodes = HUFFMAN_MAX_SYMBOLS;
	
	/* construct the table */
	while(num_nodes_left > 1)
	{
		qsort(nodes_left, num_nodes_left, sizeof(HUFFMAN_CONSTRUCT_NODE *), sort_func);
		
		huff->nodes[huff->num_nodes].num_bits = 0;
		huff->nodes[huff->num_nodes].leafs[0] = nodes_left[num_nodes_left-1]->node_id;
		huff->nodes[huff->num_nodes].leafs[1] = nodes_left[num_nodes_left-2]->node_id;
		nodes_left[num_nodes_left-2]->node_id = huff->num_nodes;
		nodes_left[num_nodes_left-2]->frequency = nodes_left[num_nodes_left-1]->frequency + nodes_left[num_nodes_left-2]->frequency;
		huff->num_nodes++;
		num_nodes_left--;
	}

	/* set start node */
	huff->start_node = &huff->nodes[huff->num_nodes-1];
	
	/* build symbol bits */
	huffman_setbits_r(huff, huff->start_node, 0, 0);
}

void huffman_init(HUFFMAN_STATE *huff, const unsigned *frequencies)
{
	int i;

	/* make sure to cleanout every thing */
	memset(huff, 0, sizeof(HUFFMAN_STATE));

	/* construct the tree */
	huffman_construct_tree(huff, frequencies);

	/* build decode LUT */
	for(i = 0; i < HUFFMAN_LUTSIZE; i++)
	{
		unsigned bits = i;
		int k;
		HUFFMAN_NODE *node = huff->start_node;
		for(k = 0; k < HUFFMAN_LUTBITS; k++)
		{
			node = &huff->nodes[node->leafs[bits&1]];
			bits >>= 1;

			if(!node)
				break;

			if(node->num_bits)
			{
				huff->decode_lut[i] = node;
				break;
			}
		}

		if(k == HUFFMAN_LUTBITS)
			huff->decode_lut[i] = node;
	}

}

/*****************************************************************/
int huffman_compress(HUFFMAN_STATE *huff, const void *input, int input_size, void *output, int output_size)
{
	/* this macro loads a symbol for a byte into bits and bitcount */
#define HUFFMAN_MACRO_LOADSYMBOL(sym) \
	bits |= huff->nodes[sym].bits << bitcount; \
	bitcount += huff->nodes[sym].num_bits;

	/* this macro writes the symbol stored in bits and bitcount to the dst pointer */
#define HUFFMAN_MACRO_WRITE() \
	while(bitcount >= 8) \
	{ \
		*dst++ = (unsigned char)(bits&0xff); \
		if(dst == dst_end) \
			return -1; \
		bits >>= 8; \
		bitcount -= 8; \
	}

	/* setup buffer pointers */
	const unsigned char *src = (const unsigned char *)input;
	const unsigned char *src_end = src + input_size;
	unsigned char *dst = (unsigned char *)output;
	unsigned char *dst_end = dst + output_size;

	/* symbol variables */
	unsigned bits = 0;
	unsigned bitcount = 0;

	/* make sure that we have data that we want to compress */
	if(input_size)
	{
		/* {A} load the first symbol */
		int symbol = *src++;

		while(src != src_end)
		{
			/* {B} load the symbol */
			HUFFMAN_MACRO_LOADSYMBOL(symbol)

			/* {C} fetch next symbol, this is done here because it will reduce dependency in the code */
			symbol = *src++;

			/* {B} write the symbol loaded at */
			HUFFMAN_MACRO_WRITE()
		}

		/* write the last symbol loaded from {C} or {A} in the case of only 1 byte input buffer */
		HUFFMAN_MACRO_LOADSYMBOL(symbol)
		HUFFMAN_MACRO_WRITE()
	}

	/* write EOF symbol */
	HUFFMAN_MACRO_LOADSYMBOL(HUFFMAN_EOF_SYMBOL)
	HUFFMAN_MACRO_WRITE()

	/* write out the last bits */
	*dst++ = bits;

	/* return the size of the output */
	return (int)(dst - (const unsigned char *)output);

	/* remove macros */
#undef HUFFMAN_MACRO_LOADSYMBOL
#undef HUFFMAN_MACRO_WRITE
}

/*****************************************************************/
int huffman_decompress(HUFFMAN_STATE *huff, const void *input, int input_size, void *output, int output_size)
{
	/* setup buffer pointers */
	unsigned char *dst = (unsigned char *)output;
	unsigned char *src = (unsigned char *)input;
	unsigned char *dst_end = dst + output_size;
	unsigned char *src_end = src + input_size;

	unsigned bits = 0;
	unsigned bitcount = 0;

	HUFFMAN_NODE *eof = &huff->nodes[HUFFMAN_EOF_SYMBOL];
	HUFFMAN_NODE *node = 0;

	while(1)
	{
		/* {A} try to load a node now, this will reduce dependency at location {D} */
		node = 0;
		if(bitcount >= HUFFMAN_LUTBITS)
			node = huff->decode_lut[bits&HUFFMAN_LUTMASK];

		/* {B} fill with new bits */
		while(bitcount < 24 && src != src_end)
		{
			bits |= (*src++) << bitcount;
			bitcount += 8;
		}

		/* {C} load symbol now if we didn't that earlier at location {A} */
		if(!node)
			node = huff->decode_lut[bits&HUFFMAN_LUTMASK];

		/* {D} check if we hit a symbol already */
		if(node->num_bits)
		{
			/* remove the bits for that symbol */
			bits >>= node->num_bits;
			bitcount -= node->num_bits;
		}
		else
		{
			/* remove the bits that the lut checked up for us */
			bits >>= HUFFMAN_LUTBITS;
			bitcount -= HUFFMAN_LUTBITS;

			/* walk the tree bit by bit */
			while(1)
			{
				/* traverse tree */
				node = &huff->nodes[node->leafs[bits&1]];

				/* remove bit */
				bitcount--;
				bits >>= 1;

				/* check if we hit a symbol */
				if(node->num_bits)
					break;

				/* no more bits, decoding error */
				if(bitcount == 0)
					return -1;
			}
		}

		/* check for eof */
		if(node == eof)
			break;

		/* output character */
		if(dst == dst_end)
			return -1;
		*dst++ = node->symbol;
	}

	/* return the size of the decompressed buffer */
	return (int)(dst - (const unsigned char *)output);
}
