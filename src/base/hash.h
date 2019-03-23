#ifndef BASE_HASH_H
#define BASE_HASH_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

enum
{
	SHA256_DIGEST_LENGTH=256/8,
	SHA256_MAXSTRSIZE=2*SHA256_DIGEST_LENGTH+1,
};

typedef struct
{
	unsigned char data[SHA256_DIGEST_LENGTH];
} SHA256_DIGEST;

SHA256_DIGEST sha256(const void *message, size_t message_len);
void sha256_str(SHA256_DIGEST digest, char *str, size_t max_len);
int sha256_from_str(SHA256_DIGEST *out, const char *str);
int sha256_comp(SHA256_DIGEST digest1, SHA256_DIGEST digest2);

static const SHA256_DIGEST SHA256_ZEROED = {{0}};

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
inline bool operator==(const SHA256_DIGEST &that, const SHA256_DIGEST &other)
{
	return sha256_comp(that, other) == 0;
}
inline bool operator!=(const SHA256_DIGEST &that, const SHA256_DIGEST &other)
{
	return !(that == other);
}
#endif

#endif // BASE_HASH_H
