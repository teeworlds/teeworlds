#include "hash.h"
#include "hash_ctxt.h"

#include "system.h"

static void digest_str(const unsigned char *digest, size_t digest_len, char *str, size_t max_len)
{
	unsigned i;
	if(max_len > digest_len * 2 + 1)
	{
		max_len = digest_len * 2 + 1;
	}
	str[max_len - 1] = 0;
	max_len -= 1;
	for(i = 0; i < max_len; i++)
	{
		static const char HEX[] = "0123456789abcdef";
		int index = i / 2;
		if(i % 2 == 0)
		{
			str[i] = HEX[digest[index] >> 4];
		}
		else
		{
			str[i] = HEX[digest[index] & 0xf];
		}
	}
}

SHA256_DIGEST sha256(const void *message, size_t message_len)
{
	SHA256_CTX ctxt;
	sha256_init(&ctxt);
	sha256_update(&ctxt, message, message_len);
	return sha256_finish(&ctxt);
}

void sha256_str(SHA256_DIGEST digest, char *str, size_t max_len)
{
	digest_str(digest.data, sizeof(digest.data), str, max_len);
}

int sha256_comp(SHA256_DIGEST digest1, SHA256_DIGEST digest2)
{
	return mem_comp(digest1.data, digest2.data, sizeof(digest1.data));
}

MD5_DIGEST md5(const void *message, size_t message_len)
{
	MD5_CTX ctxt;
	md5_init(&ctxt);
	md5_update(&ctxt, message, message_len);
	return md5_finish(&ctxt);
}

void md5_str(MD5_DIGEST digest, char *str, size_t max_len)
{
	digest_str(digest.data, sizeof(digest.data), str, max_len);
}

int md5_comp(MD5_DIGEST digest1, MD5_DIGEST digest2)
{
	return mem_comp(digest1.data, digest2.data, sizeof(digest1.data));
}
