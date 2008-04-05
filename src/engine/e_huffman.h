
enum
{
	MAX_SYMBOL_SIZE=8,
	MAX_NODES=1024*8
};

typedef struct
{
	int i;
} HUFFSYMBOL;

typedef struct HUFFNODE_t
{
	int frequency;
	
	int symbol_size;
	unsigned char symbol[MAX_SYMBOL_SIZE];
	
	int num_bits;
	unsigned bits;

	struct HUFFNODE_t *parent;
	struct HUFFNODE_t *zero;
	struct HUFFNODE_t *one;
} HUFFNODE;

typedef struct
{
	HUFFNODE nodes[MAX_NODES];
	HUFFNODE *start_node;
	int num_symbols;
	int num_nodes;
} HUFFSTATE;


void huffman_add_symbol(HUFFSTATE *huff, int frequency, int size, unsigned char *symbol);
void huffman_init(HUFFSTATE *huff);
void huffman_construct_tree(HUFFSTATE *huff);
int huffman_compress(HUFFSTATE *huff, const void *input, int input_size, void *output, int output_size);
int huffman_decompress(HUFFSTATE *huff, const void *input, int input_size, void *output, int output_size);

int huffman_test();
