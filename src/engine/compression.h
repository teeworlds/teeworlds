// lzw is no longer in use
long lzw_compress(const void *src, int size, void *dst);
long lzw_decompress(const void *src, void *dst);

unsigned char *vint_pack(unsigned char *dst, int i);
const unsigned char *vint_unpack(const unsigned char *src, int *inout);

long intpack_compress(const void *src, int size, void *dst);
long intpack_decompress(const void *src, int size, void *dst);

long zerobit_compress(const void *src, int size, void *dst);
long zerobit_decompress(const void *src, int size, void *dst);
