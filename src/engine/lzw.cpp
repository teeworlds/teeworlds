#include <string.h>

// LZW Compressor
struct SYM
{
	unsigned char *data;
	int size;
	int next;
};

struct SYMBOLS
{
	SYM syms[512];
	int jumptable[256];
	int currentsym;
};

static SYMBOLS symbols;

// symbol info
inline int sym_size(int i) { return symbols.syms[i].size; }
inline unsigned char *sym_data(int i) { return symbols.syms[i].data; }

static void sym_index(int sym)
{
	int table = symbols.syms[sym].data[0];
	symbols.syms[sym].next = symbols.jumptable[table];
	symbols.jumptable[table] = sym;
}

static void sym_unindex(int sym)
{
	int table = symbols.syms[sym].data[0];
	int prev = -1;
	int current = symbols.jumptable[table];

	while(current != -1)
	{
		if(current == sym)
		{
			if(prev != -1)
				symbols.syms[prev].next = symbols.syms[current].next;
			else
				symbols.jumptable[table] = symbols.syms[current].next;
			break;
		}

		prev = current;
		current = symbols.syms[current].next;
	}
}

static int sym_add(unsigned char *sym, long len)
{
	int i = 256+symbols.currentsym;
	symbols.syms[i].data = sym;
	symbols.syms[i].size = len;
	symbols.currentsym = (symbols.currentsym+1)%255;
	return i;
}

static int sym_add_and_index(unsigned char *sym, long len)
{
	if(symbols.syms[256+symbols.currentsym].size)
		sym_unindex(256+symbols.currentsym);
	int s = sym_add(sym, len);
	sym_index( s);
	return s;
}

static void sym_init()
{
	static unsigned char table[256];
	for(int i = 0; i < 256; i++)
	{
		table[i] = i;
		symbols.syms[i].data = &table[i];
		symbols.syms[i].size = 1;
		symbols.jumptable[i] = -1;
	}

	for(int i = 0; i < 512; i++)
		symbols.syms[i].next = -1;

	/*
	// insert some symbols to start with
	static unsigned char zeros[8] = {0,0,0,0,0,0,0,0};
	//static unsigned char one1[4] = {0,0,0,1};
	//static unsigned char one2[4] = {1,0,0,0};
	sym_add_and_index(zeros, 2);
	sym_add_and_index(zeros, 3);
	sym_add_and_index(zeros, 4);
	sym_add_and_index(zeros, 5);
	sym_add_and_index(zeros, 6);
	sym_add_and_index(zeros, 7);
	sym_add_and_index(zeros, 8);
	
	//sym_add_and_index(one1, 4);
	//sym_add_and_index(one2, 4);*/

	symbols.currentsym = 0;
}

static int sym_find(unsigned char *data, int size, int avoid)
{
	int best = data[0];
	int bestlen = 1;
	int current = symbols.jumptable[data[0]];

	while(current != -1)
	{
		if(current != avoid && symbols.syms[current].size <= size && memcmp(data, symbols.syms[current].data, symbols.syms[current].size) == 0)
		{
			if(bestlen < symbols.syms[current].size)
			{
				bestlen = symbols.syms[current].size;
				best = current;
			}
		}

		current = symbols.syms[current].next;
	}		

	return best;
}

//
// compress
//
long lzw_compress(const void *src_, int size, void *dst_)
{
	unsigned char *src = (unsigned char *)src_;
	unsigned char *end = (unsigned char *)src_+size;
	unsigned char *dst = (unsigned char *)dst_;
	long left = (end-src);
	int lastsym = -1;

	// init symboltable
	sym_init();

	bool done = false;
	while(!done)
	{
		unsigned char *flagptr = dst;
		unsigned char flagbits = 0;
		int b = 0;

		dst++; // skip a byte where the flags are

		for(; b < 8; b++)
		{
			if(left <= 0) // check for EOF
			{
				// write EOF symbol
				flagbits |= 1<<b;
				*dst++ = 255;
				done = true;
				break;
			}

 			int sym = sym_find(src, left, lastsym);
			int symsize = sym_size( sym);

			if(sym&0x100)
				flagbits |= 1<<b; // add bit that says that its a symbol

			*dst++ = sym&0xff; // set symbol

			if(left > symsize+1) // create new symbol
				lastsym = sym_add_and_index(src, symsize+1);
			
			src += symsize; // advance src
			left -= symsize;
		}

		// write the flags
		*flagptr = flagbits;
	}

	return (long)(dst-(unsigned char*)dst_);
}

//
// decompress
//
long lzw_decompress(const void *src_, void *dst_)
{
	unsigned char *src = (unsigned char *)src_;
	unsigned char *dst = (unsigned char *)dst_;
	unsigned char *prevdst = 0;
	int prevsize = -1;
	int item;

	sym_init();

	while(1)
	{
		unsigned char flagbits = 0;
		flagbits = *src++; // read flags

		int b = 0;
		for(; b < 8; b++)
		{
			item = *src++;
			if(flagbits&(1<<b))
				item |= 256;

			if(item == 0x1ff) // EOF symbol
				return (dst-(unsigned char *)dst_);

			if(prevdst) // this one could be removed
				sym_add(prevdst, prevsize+1);
				
			memcpy(dst, sym_data(item), sym_size(item));
			prevdst = dst;
			prevsize = sym_size(item);
			dst += sym_size(item);
		}

	}

	return 0;
}
