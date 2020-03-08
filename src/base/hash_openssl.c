#if defined(CONF_OPENSSL)
#include "hash_ctxt.h"

void sha256_init(SHA256_CTX *ctxt)
{
	SHA256_Init(ctxt);
}

void sha256_update(SHA256_CTX *ctxt, const void *data, size_t data_len)
{
	SHA256_Update(ctxt, data, data_len);
}

SHA256_DIGEST sha256_finish(SHA256_CTX *ctxt)
{
	SHA256_DIGEST result;
	SHA256_Final(result.data, ctxt);
	return result;
}

void md5_init(MD5_CTX *ctxt)
{
	MD5_Init(ctxt);
}

void md5_update(MD5_CTX *ctxt, const void *data, size_t data_len)
{
	MD5_Update(ctxt, data, data_len);
}

MD5_DIGEST md5_finish(MD5_CTX *ctxt)
{
	MD5_DIGEST result;
	MD5_Final(result.data, ctxt);
	return result;
}
#endif
