/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#include <stdlib.h> /* rand() */
#include <base/system.h>

#include "e_packer.h"
#include "e_compression.h"
#include "e_engine.h"
#include "e_config.h"

/* useful for debugging */
#if 0
	#define packing_error(p) p->error = 1; dbg_break()
#else
	#define packing_error(p) p->error = 1
#endif

static int stress_get_int()
{
	static const int nasty[] = {-1, 0, 1, 66000, -66000, (-1<<31), 0x7fffffff};
	if(rand()&1)
		return rand();
	return nasty[rand()%6];
}

static const char *stress_get_string(int *size)
{
	static char noise[1024];
	int i;
	int s;
	s = (rand()%1024)-1;
	for(i = 0; i < s; i++)
		noise[i] = (rand()%254)+1;
	noise[s] = 0;
	if(size)
		*size = s;
	return noise;
}


static int stress_prob(float probability)
{
	if(!config.dbg_stress_network)
		return 0;
	if(rand()/(float)RAND_MAX < probability)
		return 1;
	return 0;
}


void packer_reset(PACKER *p)
{
	p->error = 0;
	p->current = p->buffer;
	p->end = p->current + PACKER_BUFFER_SIZE;
}

void packer_add_int(PACKER *p, int i)
{
	if(p->error)
		return;
		
	if(stress_prob(0.025f))
		i = stress_get_int();
	
	/* make sure that we have space enough */
	if(p->end - p->current < 6)
	{
		dbg_break();
		p->error = 1;
	}
	else
		p->current = vint_pack(p->current, i);
}

void packer_add_string(PACKER *p, const char *str, int limit)
{
	if(p->error)
		return;
		
	if(stress_prob(0.1f))
	{
		str = stress_get_string(0);
		limit = 0;
	}
	
	/* */
	if(limit > 0)
	{
		while(*str && limit != 0)
		{
			*p->current++ = *str++;
			limit--;
			
			if(p->current >= p->end)
			{
				packing_error(p);
				break;
			}
		}
		*p->current++ = 0;
	}
	else
	{
		while(*str)
		{
			*p->current++ = *str++;

			if(p->current >= p->end)
			{
				packing_error(p);
				break;
			}
		}
		*p->current++ = 0;
	}
}

void packer_add_raw(PACKER *p, const unsigned char *data, int size)
{
	if(p->error)
		return;
		
	if(p->current+size >= p->end)
	{
		packing_error(p);
		return;
	}
	
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
	if(p->error)
		return 0;
	if(p->current >= p->end)
	{
		packing_error(p);
		return 0;
	}
	
	p->current = vint_unpack(p->current, &i);
	if(p->current > p->end)
	{
		packing_error(p);
		return 0;
	}
	return i;
}

const char *unpacker_get_string(UNPACKER *p)
{
	char *ptr;
	if(p->error || p->current >= p->end)
		return "";
		
	ptr = (char *)p->current;
	while(*p->current) /* skip the string */
	{
		p->current++;
		if(p->current == p->end)
		{
			packing_error(p);
			return "";
		}
	}
	p->current++;
	
	/* sanitize all strings */
	str_sanitize(ptr);
	return ptr;
}

const unsigned char *unpacker_get_raw(UNPACKER *p, int size)
{
	const unsigned char *ptr = p->current;
	if(p->error)
		return 0;
	
	/* check for nasty sizes */
	if(size < 0 || p->current+size > p->end)
	{
		packing_error(p);
		return 0;
	}

	/* "unpack" the data */	
	p->current += size;
	return ptr;
}
