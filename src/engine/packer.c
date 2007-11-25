/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */

#include "packer.h"
#include "compression.h"

void packer_reset(PACKER *p)
{
	p->error = 0;
	p->current = p->buffer;
	p->end = p->current + PACKER_BUFFER_SIZE;
}

void packer_add_int(PACKER *p, int i)
{
	p->current = vint_pack(p->current, i);
}

void packer_add_string(PACKER *p, const char *str, int limit)
{
	if(limit > 0)
	{
		while(*str && limit != 0)
		{
			*p->current++ = *str++;
			limit--;
		}
		*p->current++ = 0;
	}
	else
	{
		while(*str)
			*p->current++ = *str++;
		*p->current++ = 0;
	}
}

void packer_add_raw(PACKER *p, const unsigned char *data, int size)
{
	while(size)
	{
		*p->current++ = *data++;
		size--;
	}
}

int packer_size(PACKER *p)
{
	return (const unsigned char *)p->current-(const unsigned char *)p->buffer;
}

const unsigned char *packer_data(PACKER *p)
{
	return (const unsigned char *)p->buffer;
}

void unpacker_reset(UNPACKER *p, const unsigned char *data, int size)
{
	p->error = 0;
	p->start = data;
	p->end = p->start + size;
	p->current = p->start;
}

int unpacker_get_int(UNPACKER *p)
{
	int i;
	if(p->current >= p->end)
		return 0;
	p->current = vint_unpack(p->current, &i);
	return i;
}

const char *unpacker_get_string(UNPACKER *p)
{
	const char *ptr;
	if(p->current >= p->end)
		return "";
		
	ptr = (const char *)p->current;
	while(*p->current) /* skip the string */
		p->current++;
	p->current++;
	return ptr;
}

const unsigned char *unpacker_get_raw(UNPACKER *p, int size)
{
	const unsigned char *ptr = p->current;
	p->current += size;
	return ptr;
}
