#include "system.h"
#include <string.h>

/* Format: ESDDDDDD EDDDDDDD EDD...  Extended, Data, Sign */
unsigned char *vint_pack(unsigned char *dst, int i) 
{ 
	*dst = (i>>25)&0x40; /* set sign bit if i<0 */
	i = i^(i>>31); /* if(i<0) i = ~i */

	*dst |= i&0x3F; /* pack 6bit into dst */
	i >>= 6; /* discard 6 bits */
	if(i)
	{
		*dst |= 0x80; /* set extend bit */
		while(1)
		{
			dst++;
			*dst = i&(0x7F); /* pack 7bit */
			i >>= 7; /* discard 7 bits */
			*dst |= (i!=0)<<7; /* set extend bit (may branch) */
			if(!i)
				break;
		}
	}

	dst++;
	return dst; 
} 
 
const unsigned char *vint_unpack(const unsigned char *src, int *i)
{ 
	int sign = (*src>>6)&1; 
	*i = *src&0x3F; 

	do
	{ 
		if(!(*src&0x80)) break;
		src++;
		*i |= (*src&(0x7F))<<(6);

		if(!(*src&0x80)) break;
		src++;
		*i |= (*src&(0x7F))<<(6+7);

		if(!(*src&0x80)) break;
		src++;
		*i |= (*src&(0x7F))<<(6+7+7);

		if(!(*src&0x80)) break;
		src++;
		*i |= (*src&(0x7F))<<(6+7+7+7);
	} while(0);

	src++;
	*i ^= -sign; /* if(sign) *i = ~(*i) */
	return src; 
} 


long intpack_decompress(const void *src_, int size, void *dst_)
{
	const unsigned char *src = (unsigned char *)src_;
	const unsigned char *end = src + size;
	int *dst = (int *)dst_;
	while(src < end)
	{
		src = vint_unpack(src, dst);
		dst++;
	}
	return (long)((unsigned char *)dst-(unsigned char *)dst_);
}

long intpack_compress(const void *src_, int size, void *dst_)
{
	int *src = (int *)src_;
	unsigned char *dst = (unsigned char *)dst_;
	size /= 4;
	while(size)
	{
		dst = vint_pack(dst, *src);
		size--;
		src++;
	}
	return (long)(dst-(unsigned char *)dst_);
}


/* */
long zerobit_compress(const void *src_, int size, void *dst_)
{
	unsigned char *src = (unsigned char *)src_;
	unsigned char *dst = (unsigned char *)dst_;
	
	while(size)
	{
		unsigned char bit = 0x80;
		unsigned char mask = 0;
		int dst_move = 1;
		int chunk = size < 8 ? size : 8;
		int b;
		size -= chunk;
		
		for(b = 0; b < chunk; b++, bit>>=1)
		{
			if(*src)
			{
				dst[dst_move] = *src;
				mask |= bit;
				dst_move++;
			}
			
			src++;
		}
		
		*dst = mask;
		dst += dst_move;
	}
	
	return (long)(dst-(unsigned char *)dst_);
}

long zerobit_decompress(const void *src_, int size, void *dst_)
{
	unsigned char *src = (unsigned char *)src_;
	unsigned char *dst = (unsigned char *)dst_;
	unsigned char *end = src + size;
	
	while(src != end)
	{
		unsigned char bit = 0x80;
		unsigned char mask = *src++;
		int b;
		
		for(b = 0; b < 8; b++, bit>>=1)
		{
			if(mask&bit)
				*dst++ = *src++;
			else
				*dst++ = 0;
		}
	}
	
	long l = (long)(dst-(unsigned char *)dst_);
	return l;
}

