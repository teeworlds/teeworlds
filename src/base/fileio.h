/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

/*
	Title: OS Abstraction - File IO
*/

#ifndef BASE_FILEIO_H
#define BASE_FILEIO_H

#include "detect.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
	IOFLAG_READ = 1,
	IOFLAG_WRITE = 2,
	IOFLAG_RANDOM = 4,

	IOSEEK_START = 0,
	IOSEEK_CUR = 1,
	IOSEEK_END = 2
};

typedef struct IOINTERNAL *IOHANDLE;

/*
	Function: io_open
		Opens a file.

	Parameters:
		filename - File to open.
		flags - A set of flags. IOFLAG_READ, IOFLAG_WRITE, IOFLAG_RANDOM.

	Returns:
		Returns a handle to the file on success and 0 on failure.

*/
IOHANDLE io_open(const char *filename, int flags);

/*
	Function: io_read
		Reads data into a buffer from a file.

	Parameters:
		io - Handle to the file to read data from.
		buffer - Pointer to the buffer that will recive the data.
		size - Number of bytes to read from the file.

	Returns:
		Number of bytes read.

*/
unsigned io_read(IOHANDLE io, void *buffer, unsigned size);

/*
	Function: io_unread_byte
		"Unreads" a single byte, making it available for future read
		operations.

	Parameters:
		io - Handle to the file to unread the byte from.
		byte - Byte to unread.

	Returns:
		Returns 0 on success and 1 on failure.

*/
unsigned io_unread_byte(IOHANDLE io, unsigned char byte);

/*
	Function: io_skip
		Skips data in a file.

	Parameters:
		io - Handle to the file.
		size - Number of bytes to skip.

	Returns:
		Number of bytes skipped.
*/
unsigned io_skip(IOHANDLE io, int size);

/*
	Function: io_write
		Writes data from a buffer to file.

	Parameters:
		io - Handle to the file.
		buffer - Pointer to the data that should be written.
		size - Number of bytes to write.

	Returns:
		Number of bytes written.
*/
unsigned io_write(IOHANDLE io, const void *buffer, unsigned size);

/*
	Function: io_write_newline
		Writes newline to file.

	Parameters:
		io - Handle to the file.

	Returns:
		Number of bytes written.
*/
unsigned io_write_newline(IOHANDLE io);

/*
	Function: io_seek
		Seeks to a specified offset in the file.

	Parameters:
		io - Handle to the file.
		offset - Offset from pos to stop.
		origin - Position to start searching from.

	Returns:
		Returns 0 on success.
*/
int io_seek(IOHANDLE io, int offset, int origin);

/*
	Function: io_tell
		Gets the current position in the file.

	Parameters:
		io - Handle to the file.

	Returns:
		Returns the current position. -1L if an error occured.
*/
long int io_tell(IOHANDLE io);

/*
	Function: io_length
		Gets the total length of the file. Resetting cursor to the beginning

	Parameters:
		io - Handle to the file.

	Returns:
		Returns the total size. -1L if an error occured.
*/
long int io_length(IOHANDLE io);

/*
	Function: io_close
		Closes a file.

	Parameters:
		io - Handle to the file.

	Returns:
		Returns 0 on success.
*/
int io_close(IOHANDLE io);

/*
	Function: io_flush
		Empties all buffers and writes all pending data.

	Parameters:
		io - Handle to the file.

	Returns:
		Returns 0 on success.
*/
int io_flush(IOHANDLE io);


/*
	Function: io_stdin
		Returns an <IOHANDLE> to the standard input.
*/
IOHANDLE io_stdin();

/*
	Function: io_stdout
		Returns an <IOHANDLE> to the standard output.
*/
IOHANDLE io_stdout();

/*
	Function: io_stderr
		Returns an <IOHANDLE> to the standard error.
*/
IOHANDLE io_stderr();

#ifdef __cplusplus
}
#endif

#endif
