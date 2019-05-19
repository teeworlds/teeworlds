#ifndef BASE_HASH_CTXT_H
#define BASE_HASH_CTXT_H

#include "hash.h"
#include <stdint.h>

#if defined(CONF_OPENSSL)
#include <openssl/sha.h>
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
#endif

void sha256_init(SHA256_CTX *ctxt);
void sha256_update(SHA256_CTX *ctxt, const void *data, size_t data_len);
SHA256_DIGEST sha256_finish(SHA256_CTX *ctxt);

#ifdef __cplusplus
}
#endif

#endif // BASE_HASH_CTXT_H
