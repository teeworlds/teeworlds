#include "e_system.h"

/* buffered stream for reading lines, should perhaps be something smaller */
typedef struct
{
	char buffer[4*1024];
	unsigned buffer_pos;
	unsigned buffer_size;
	unsigned buffer_max_size;
	IOHANDLE io;
} LINEREADER;

void linereader_init(LINEREADER *lr, IOHANDLE io);
char *linereader_get(LINEREADER *lr);
