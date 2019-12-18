#ifndef BASE_HASH_CTXT_H
#define BASE_HASH_CTXT_H

#include "hash.h"
#include <stdint.h>

#if defined(CONF_OPENSSL)
#include <openssl/md5.h>
#include <openssl/sha.h>
#else
#include <engine/external/md5/md5.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONF_OPENSSL)
// SHA256_CTX is defined in <openssl/sha.h>
#else
typedef struct
{
    uint64_t length;
    uint32_t state[8];
    uint32_t curlen;
    unsigned char buf[64];
} SHA256_CTX;
typedef md5_state_t MD5_CTX;
#endif

void sha256_init(SHA256_CTX *ctxt);
void sha256_update(SHA256_CTX *ctxt, const void *data, size_t data_len);
SHA256_DIGEST sha256_finish(SHA256_CTX *ctxt);

void md5_init(MD5_CTX *ctxt);
void md5_update(MD5_CTX *ctxt, const void *data, size_t data_len);
MD5_DIGEST md5_finish(MD5_CTX *ctxt);

#ifdef __cplusplus
}
#endif

#endif // BASE_HASH_CTXT_H
