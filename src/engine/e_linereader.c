#include "e_linereader.h"

void linereader_init(LINEREADER *lr, IOHANDLE io)
{
	lr->buffer_max_size = 4*1024;
	lr->buffer_size = 0;
	lr->buffer_pos = 0;
	lr->io = io;
}

char *linereader_get(LINEREADER *lr)
{
	unsigned line_start = lr->buffer_pos;

	while(1)
	{
		if(lr->buffer_pos >= lr->buffer_size)
		{
			/* fetch more */

			/* move the remaining part to the front */
			unsigned read;
			unsigned left = lr->buffer_size - line_start;

			if(line_start > lr->buffer_size)
				left = 0;
			if(left)
				mem_move(lr->buffer, &lr->buffer[line_start], left);
			lr->buffer_pos = left;

			/* fill the buffer */
			read = io_read(lr->io, &lr->buffer[lr->buffer_pos], lr->buffer_max_size-lr->buffer_pos);
			lr->buffer_size = left + read;
			line_start = 0;

			if(!read)
			{
				if(left)
				{
					lr->buffer[left] = 0; /* return the last line */
					lr->buffer_pos = left;
					lr->buffer_size = left;
					return lr->buffer;
				}
				else
					return 0x0; /* we are done! */
			}
		}
		else
		{
			if(lr->buffer[lr->buffer_pos] == '\n' || lr->buffer[lr->buffer_pos] == '\r')
			{
				/* line found */
				lr->buffer[lr->buffer_pos] = 0;
				lr->buffer_pos++;
				return &lr->buffer[line_start];
			}
			else
				lr->buffer_pos++;
		}
	}
}
