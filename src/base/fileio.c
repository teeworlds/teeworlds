/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <stdlib.h>
#include <stdio.h>

#include "strings.h"
#include "fileio.h"

#if defined(CONF_FAMILY_UNIX)
	#include <unistd.h>
#elif defined(CONF_FAMILY_WINDOWS)
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
#else
	#error NOT IMPLEMENTED
#endif

#if defined(__cplusplus)
extern "C" {
#endif

IOHANDLE io_stdin() { return (IOHANDLE)stdin; }
IOHANDLE io_stdout() { return (IOHANDLE)stdout; }
IOHANDLE io_stderr() { return (IOHANDLE)stderr; }

IOHANDLE io_open(const char *filename, int flags)
{
	if(flags == IOFLAG_READ)
	{
	#if defined(CONF_FAMILY_WINDOWS)
		// check for filename case sensitive
		WIN32_FIND_DATA finddata;
		HANDLE handle;
		int length;

		length = str_length(filename);
		if(!filename || !length || filename[length-1] == '\\')
			return 0x0;
		handle = FindFirstFile(filename, &finddata);
		if(handle == INVALID_HANDLE_VALUE)
			return 0x0;
		else if(str_comp(filename+length-str_length(finddata.cFileName), finddata.cFileName) != 0)
		{
			FindClose(handle);
			return 0x0;
		}
		FindClose(handle);
	#endif
		return (IOHANDLE)fopen(filename, "rb");
	}
	if(flags == IOFLAG_WRITE)
		return (IOHANDLE)fopen(filename, "wb");
	return 0x0;
}

unsigned io_read(IOHANDLE io, void *buffer, unsigned size)
{
	return fread(buffer, 1, size, (FILE*)io);
}

unsigned io_unread_byte(IOHANDLE io, unsigned char byte)
{
	return ungetc(byte, (FILE*)io) == EOF;
}

unsigned io_skip(IOHANDLE io, int size)
{
	fseek((FILE*)io, size, SEEK_CUR);
	return size;
}

int io_seek(IOHANDLE io, int offset, int origin)
{
	int real_origin;

	switch(origin)
	{
	case IOSEEK_START:
		real_origin = SEEK_SET;
		break;
	case IOSEEK_CUR:
		real_origin = SEEK_CUR;
		break;
	case IOSEEK_END:
		real_origin = SEEK_END;
		break;
	default:
		return -1;
	}

	return fseek((FILE*)io, offset, real_origin);
}

long int io_tell(IOHANDLE io)
{
	return ftell((FILE*)io);
}

long int io_length(IOHANDLE io)
{
	long int length;
	io_seek(io, 0, IOSEEK_END);
	length = io_tell(io);
	io_seek(io, 0, IOSEEK_START);
	return length;
}

unsigned io_write(IOHANDLE io, const void *buffer, unsigned size)
{
	return fwrite(buffer, 1, size, (FILE*)io);
}

unsigned io_write_newline(IOHANDLE io)
{
#if defined(CONF_FAMILY_WINDOWS)
	return fwrite("\r\n", 1, 2, (FILE*)io);
#else
	return fwrite("\n", 1, 1, (FILE*)io);
#endif
}

int io_close(IOHANDLE io)
{
	fclose((FILE*)io);
	return 0;
}

int io_flush(IOHANDLE io)
{
	fflush((FILE*)io);
	return 0;
}

#if defined(__cplusplus)
}
#endif
