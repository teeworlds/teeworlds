#include <base/system.h>
#include <stdlib.h>
#include <string.h>
#include <engine/e_huffman.h>

void huffman_init(HUFFSTATE *huff)
{
	mem_zero(huff, sizeof(HUFFSTATE));
	huff->nodes[0].frequency = 1;
	huff->nodes[0].symbol_size = -1;
	huff->num_symbols++;
	huff->num_nodes++;
}

void huffman_add_symbol(HUFFSTATE *huff, int frequency, int size, unsigned char *symbol)
{
	huff->nodes[huff->num_nodes].frequency = frequency;
	huff->nodes[huff->num_nodes].symbol_size = size;
	mem_copy(huff->nodes[huff->num_nodes].symbol, symbol, size);
	huff->num_nodes++;
	huff->num_symbols++;
}


static int sort_func(const void *a, const void *b)
{
	if((*(HUFFNODE **)a)->frequency > (*(HUFFNODE **)b)->frequency)
		return -1;
	if((*(HUFFNODE **)a)->frequency < (*(HUFFNODE **)b)->frequency)
		return 1;
	return 0;
}

void huffman_setbits_r(HUFFNODE *node, int bits, int depth)
{
	if(node->one)
		huffman_setbits_r(node->one, (bits<<1)|1, depth+1);
	if(node->zero)
		huffman_setbits_r(node->zero, (bits<<1), depth+1);
		
	if(node->symbol_size)
	{
		node->bits = bits;
		node->num_bits = depth;
	}
}
	
void huffman_checktree_r(HUFFNODE *node, int bits, int depth)
{
	if(node->one)
		huffman_checktree_r(node->one, (bits<<1)|1, depth+1);
	if(node->zero)
		huffman_checktree_r(node->zero, (bits<<1), depth+1);
		
	if(node->symbol_size)
	{
		/*dbg_msg("", "%p %p %d %d %d", node->one, node->zero, node->symbol[0], node->bits, node->num_bits);*/
		
		if(node->bits != bits || node->num_bits != depth)
		{
			dbg_msg("", "crap!   %d %d=%d   %d!=%d", node->bits>>1, node->bits, bits, node->num_bits , depth);
			/*dbg_msg("", "%p %p %d", node->one, node->zero, node->symbol[0]);*/
		}
	}
}

void huffman_construct_tree(HUFFSTATE *huff)
{
	HUFFNODE *nodes_left[MAX_NODES];
	int num_nodes_left = huff->num_nodes;
	int i, k;

	for(i = 0; i < num_nodes_left; i++)
		nodes_left[i] = &huff->nodes[i];
	
	/* construct the table */
	while(num_nodes_left > 1)
	{
		qsort(nodes_left, num_nodes_left, sizeof(HUFFNODE *), sort_func);
		
		huff->nodes[huff->num_nodes].symbol_size = 0;
		huff->nodes[huff->num_nodes].frequency = nodes_left[num_nodes_left-1]->frequency + nodes_left[num_nodes_left-2]->frequency;
		huff->nodes[huff->num_nodes].zero = nodes_left[num_nodes_left-1];
		huff->nodes[huff->num_nodes].one = nodes_left[num_nodes_left-2];
		nodes_left[num_nodes_left-1]->parent = &huff->nodes[huff->num_nodes];
		nodes_left[num_nodes_left-2]->parent = &huff->nodes[huff->num_nodes];
		nodes_left[num_nodes_left-2] = &huff->nodes[huff->num_nodes];
		huff->num_nodes++;
		num_nodes_left--;
	}

	dbg_msg("", "%d", huff->num_nodes);
	for(i = 0; i < huff->num_nodes; i++)
	{
		if(huff->nodes[i].symbol_size && (huff->nodes[i].one || huff->nodes[i].zero))
			dbg_msg("", "tree strangeness");
			
		if(!huff->nodes[i].parent)
		{
			dbg_msg("", "root %p %p", huff->nodes[i].one, huff->nodes[i].zero);
			huff->start_node = &huff->nodes[i];
		}
	}
	
	huffman_setbits_r(huff->start_node, 0, 0);
	
	/*
	for(i = 0; i < huff->num_symbols; i++)
	{
		unsigned bits = 0;
		int num_bits = 0;
		HUFFNODE *n = &huff->nodes[i];
		HUFFNODE *p = n;
		HUFFNODE *c = n->parent;
		
		while(c)
		{
			num_bits++;
			if(c->one == p)
				bits |= 1;
			bits <<= 1;
			p = c;
			c = c->parent;
		}

		n->bits = bits;
		n->num_bits = num_bits;
	}*/
	
	huffman_checktree_r(huff->start_node, 0, 0);

	for(i = 0; i < huff->num_symbols; i++)
	{
		for(k = 0; k < huff->num_symbols; k++)
		{
			if(k == i)
				continue;
			if(huff->nodes[i].num_bits == huff->nodes[k].num_bits && huff->nodes[i].bits == huff->nodes[k].bits)
				dbg_msg("", "tree error %d %d %d", i, k, huff->nodes[i].num_bits);
		}
	}

}

typedef struct
{
	unsigned char *data;
	unsigned char current_bits;
	int num;
} HUFFBITIO;

int debug_count = 0;

static void bitio_init(HUFFBITIO *bitio, unsigned char *data)
{
	bitio->data = data;
	bitio->num = 0;
	bitio->current_bits = 0;
}

static void bitio_flush(HUFFBITIO *bitio)
{
	if(bitio->num == 8)
		*bitio->data = bitio->current_bits << (8-bitio->num);
	else
		*bitio->data = bitio->current_bits;
	bitio->data++;
	bitio->num = 0;
	bitio->current_bits = 0;
}

static void bitio_write(HUFFBITIO *bitio, int bit)
{
	bitio->current_bits = (bitio->current_bits<<1)|bit;
	bitio->num++;
	if(bitio->num == 8)
		bitio_flush(bitio);
		
	if(debug_count)
	{
		debug_count--;
		dbg_msg("", "out %d", bit);
	}
}

static int bitio_read(HUFFBITIO *bitio)
{
	int bit;
	
	if(!bitio->num)
	{
		bitio->current_bits = *bitio->data;
		bitio->data++;
		bitio->num = 8;
	}
	
	bitio->num--;
	bit = (bitio->current_bits>>bitio->num)&1;

	if(debug_count)
	{
		debug_count--;
		dbg_msg("", "in %d", bit);
	}
	return bit;
}

int huffman_compress(HUFFSTATE *huff, const void *input, int input_size, void *output, int output_size)
{
	const unsigned char *src = (const unsigned char *)input;
	int ret = 0;
	int quit = 0;
	HUFFBITIO io;
	
	bitio_init(&io, (unsigned char *)output);
	
	while(!quit)
	{
		int i;
		int best_match = -1;
		int best_match_size = 0;
		
		if(input_size)
		{
			for(i = 0; i < huff->num_symbols; i++)
			{
				if(huff->nodes[i].symbol_size <= input_size && huff->nodes[i].symbol_size > best_match_size)
				{
					if(memcmp(src, huff->nodes[i].symbol, huff->nodes[i].symbol_size) == 0)
					{
						best_match = i;
						best_match_size = huff->nodes[i].symbol_size;
					}
				}
			}
		}
		else
		{
			best_match = 0;
			best_match_size = 0;
			quit = 1;
		}
		
		
		if(best_match == -1)
		{
			dbg_msg("huffman", "couldn't find symbol! %d left", input_size);
			return -1;
		}
		
		i = huff->nodes[best_match].num_bits;
		for(i = huff->nodes[best_match].num_bits-1; i >= 0; i--)
			bitio_write(&io, (huff->nodes[best_match].bits>>i)&1);

		if(debug_count)		
			dbg_msg("", "--");
		
		ret += huff->nodes[best_match].num_bits;
		input_size -= best_match_size;
		src += best_match_size;
	}
	
	bitio_flush(&io);
	return ret;
}

int huffman_decompress(HUFFSTATE *huff, const void *input, int input_size, void *output, int output_size)
{
	unsigned char *dst = (unsigned char *)output;
	HUFFBITIO io;
	int size = 0;
	
	bitio_init(&io, (unsigned char *)input);
	
	while(size < 1401)
	{
		HUFFNODE *node = huff->start_node;
		int i;
		
		while(node)
		{
			if(node->symbol_size)
				break;
				
			if(bitio_read(&io))
				node = node->one;
			else
				node = node->zero;
		}

		if(debug_count)		
			dbg_msg("", "-- %d %d", node->bits, node->num_bits);
		
		/* check for eof */
		if(node == &huff->nodes[0])
			break;
		
		for(i = 0; i < node->symbol_size; i++)
		{
			*dst++ = node->symbol[i];
			size++;
		}
	}
	
	return size;
}

unsigned char test_data[1024*64];
int test_data_size;

unsigned char compressed_data[1024*64];
int compressed_data_size;

unsigned char output_data[1024*64];
int output_data_size;

HUFFSTATE state;

int huffman_test()
{
	huffman_init(&state);
	
	dbg_msg("", "test test");
	
	/* bitio testing */
	{
		char bits[] = {1, 1, 0, 1, 0, 0, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1,1,1,1,1,1,1,0};
		unsigned char buf[64];
		int i;
		HUFFBITIO io;
		
		bitio_init(&io, buf);
		for(i = 0; i < sizeof(bits); i++)
			bitio_write(&io, bits[i]);
			
		bitio_flush(&io);
		bitio_init(&io, buf);
		for(i = 0; i < sizeof(bits); i++)
		{
			if(bitio_read(&io) != bits[i])
				dbg_msg("", "bitio failed at %d", i);
		}
	}
	
	/* read test data */
	{
		IOHANDLE io = io_open("license.txt", IOFLAG_READ);
		test_data_size = io_read(io, test_data, sizeof(test_data));
		io_close(io);
	}
	
	/* add symbols */
	{
		int counts[256] = {0};
		int i;
		for(i = 0; i < test_data_size; i++)
			counts[test_data[i]]++;
		
		for(i = 0; i < 256; i++)
		{
			unsigned char symbol = (unsigned char )i;
			if(counts[i])
				huffman_add_symbol(&state, counts[i], 1, &symbol);
		}
	}
	
	huffman_construct_tree(&state);
	/*debug_count = 20;*/
	compressed_data_size = huffman_compress(&state, test_data, test_data_size, compressed_data, sizeof(compressed_data));
	/*debug_count = 20;*/
	output_data_size = huffman_decompress(&state, compressed_data, compressed_data_size, output_data, sizeof(output_data));
	
	dbg_msg("huffman", "%d -> %d -> %d", test_data_size, compressed_data_size/8, output_data_size);
	
	/* write test data */
	{
		IOHANDLE io = io_open("out.txt", IOFLAG_WRITE);
		io_write(io, output_data, output_data_size);
		io_close(io);
	}	
	
	return 0;
}
