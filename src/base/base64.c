/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "base64.h"
#include "system.h"

static const char base64_encode_table[64] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

char *base64_encode(const unsigned char *data, unsigned len)
{
	unsigned result_index = 0;
	unsigned result_len = 4 * ((len + 2) / 3) + 1;
	char *result = mem_alloc(result_len);

	for(unsigned x = 0; x < len; x += 3) 
	{
		unsigned n = ((unsigned)data[x]) << 16;
		if((x + 1) < len)
			n += ((unsigned)data[x + 1]) << 8;
		if((x + 2) < len)
			n += data[x + 2];

		result[result_index++] = base64_encode_table[(n >> 18) & 0x3F];
		result[result_index++] = base64_encode_table[(n >> 12) & 0x3F];

		if((x + 1) < len)
			result[result_index++] = base64_encode_table[(n >> 6) & 0x3F];
		if((x + 2) < len)
			result[result_index++] = base64_encode_table[n & 0x3F];
	}

	for(int padding = len % 3; padding > 0 && padding < 3; padding++) 
		result[result_index++] = '=';

	dbg_assert(result_index < result_len, "too small base64 encode size");
	result[result_index] = '\0';
	return result;
}

static const unsigned char base64_decode_table[256] = {
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 64, 64, 63, 52, 53,
	54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64, 64, 64, 64, 64,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9,
	10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 64, 64, 26, 27, 28,
	29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64
};

unsigned base64_decode(unsigned char **data, const char *str)
{
	unsigned char iter = 0;
	unsigned buf = 0;
	unsigned str_index = 0;
	unsigned str_len = str_length(str);
	const unsigned char *str_unsigned = (const unsigned char *)str; // so indices for the decoding table are positive
	unsigned data_index = 0;
	unsigned data_len = (str_len * 3) / 4 + 1;
	*data = mem_alloc(data_len);

	while(str_index < str_len)
	{
		const unsigned char chr_value = base64_decode_table[str_unsigned[str_index++]];
		if(chr_value == 64) // skip whitespace, padding and invalid characters
			continue;

		buf = buf << 6 | chr_value;
		iter++;
		if(iter == 4)
		{
			(*data)[data_index++] = (buf >> 16) & 0xFF;
			(*data)[data_index++] = (buf >> 8) & 0xFF;
			(*data)[data_index++] = buf & 0xFF;
			buf = 0;
			iter = 0;
		}
	}

	if(iter == 3)
	{
		(*data)[data_index++] = (buf >> 10) & 0xFF;
		(*data)[data_index++] = (buf >> 2) & 0xFF;
	}
	else if(iter == 2)
	{
		(*data)[data_index++] = (buf >> 4) & 0xFF;
	}

	dbg_assert(data_index < data_len, "too small base64 decode size");
	(*data)[data_index] = '\0'; // zero-terminate for more convenient usage with strings
	return data_index; // the zero-terminator is not included in the total size
}
