
enum
{
	PACKER_BUFFER_SIZE=1024*2
};

typedef struct
{
	
	unsigned char buffer[PACKER_BUFFER_SIZE];
	unsigned char *current;
	unsigned char *end;
	int error;
} PACKER;

typedef struct
{
	const unsigned char *current;
	const unsigned char *start;
	const unsigned char *end;
	int error;
} UNPACKER;

void packer_reset(PACKER *p);
void packer_add_int(PACKER *p, int i);
void packer_add_string(PACKER *p, const char *str, int limit);
void packer_add_raw(PACKER *p, const unsigned char *data, int size);
int packer_size(PACKER *p);
const unsigned char *packer_data(PACKER *p);

void unpacker_reset(UNPACKER *p, const unsigned char *data, int size);
int unpacker_get_int(UNPACKER *p);
const char *unpacker_get_string(UNPACKER *p);
const unsigned char *unpacker_get_raw(UNPACKER *p, int size);
