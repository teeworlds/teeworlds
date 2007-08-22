
/* variable int packing */
unsigned char *vint_pack(unsigned char *dst, int i);
const unsigned char *vint_unpack(const unsigned char *src, int *inout);
long intpack_compress(const void *src, int size, void *dst);
long intpack_decompress(const void *src, int size, void *dst);

/* zerobit packing */
long zerobit_compress(const void *src, int size, void *dst);
long zerobit_decompress(const void *src, int size, void *dst);
