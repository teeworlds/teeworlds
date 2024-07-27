/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include "system.h"

#include <sys/types.h>
#include <sys/stat.h>

#if defined(CONF_FAMILY_UNIX)
	#include <sys/time.h>
	#include <unistd.h>

	/* unix net includes */
	#include <sys/socket.h>
	#include <sys/ioctl.h>
	#include <errno.h>
	#include <netdb.h>
	#include <netinet/in.h>
	#include <fcntl.h>
	#include <pthread.h>
	#include <arpa/inet.h>

	#include <dirent.h>

	#if defined(CONF_PLATFORM_MACOS)
		#include <Carbon/Carbon.h>
	#endif

#elif defined(CONF_FAMILY_WINDOWS)
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
	#include <winsock2.h>
	#include <ws2tcpip.h>
	#include <fcntl.h>
	#include <direct.h>
	#include <errno.h>
	#include <process.h>
	#include <wincrypt.h>
	#include <share.h>
	#include <shellapi.h>
#else
	#error NOT IMPLEMENTED
#endif

#if defined(CONF_ARCH_IA32) || defined(CONF_ARCH_AMD64)
	#include <immintrin.h> //_mm_pause
#endif

#if defined(CONF_PLATFORM_SOLARIS)
	#include <sys/filio.h>
#endif

#if defined(__cplusplus)
extern "C" {
#endif

IOHANDLE io_stdin() { return (IOHANDLE)stdin; }
IOHANDLE io_stdout() { return (IOHANDLE)stdout; }
IOHANDLE io_stderr() { return (IOHANDLE)stderr; }

typedef struct
{
	DBG_LOGGER logger;
	DBG_LOGGER_FINISH finish;
	void *user;
} DBG_LOGGER_DATA;

static DBG_LOGGER_DATA loggers[16];
static int num_loggers = 0;

static NETSTATS network_stats = {0};

static NETSOCKET invalid_socket = {NETTYPE_INVALID, -1, -1};

static void dbg_logger_finish(void)
{
	int i;
	for(i = 0; i < num_loggers; i++)
	{
		if(loggers[i].finish)
		{
			loggers[i].finish(loggers[i].user);
		}
	}
}

void dbg_logger(DBG_LOGGER logger, DBG_LOGGER_FINISH finish, void *user)
{
	DBG_LOGGER_DATA data;
	if(num_loggers == 0)
	{
		atexit(dbg_logger_finish);
	}
	data.logger = logger;
	data.finish = finish;
	data.user = user;
	loggers[num_loggers] = data;
	num_loggers++;
}

void dbg_assert_imp(const char *filename, int line, int test, const char *msg)
{
	if(!test)
	{
		dbg_msg("assert", "%s(%d): %s", filename, line, msg);
		dbg_break();
	}
}

void dbg_break()
{
#ifdef __GNUC__
	__builtin_trap();
#else
	abort();
#endif
}

void dbg_msg(const char *sys, const char *fmt, ...)
{
	va_list args;
	char str[1024*4];
	char *msg;
	int i, len;

	char timestr[80];
	str_timestamp_format(timestr, sizeof(timestr), FORMAT_SPACE);

	str_format(str, sizeof(str), "[%s][%s]: ", timestr, sys);

	len = str_length(str);
	msg = (char *)str + len;

	va_start(args, fmt);
#if defined(CONF_FAMILY_WINDOWS) && !defined(__GNUC__)
	_vsprintf_p(msg, sizeof(str)-len, fmt, args);
#else
	vsnprintf(msg, sizeof(str)-len, fmt, args);
#endif
	va_end(args);

	for(i = 0; i < num_loggers; i++)
		loggers[i].logger(str, loggers[i].user);
}

#if defined(CONF_FAMILY_WINDOWS)
static void logger_win_console(const char *line, void *user)
{
	#define MAX_LENGTH 1024
	#define MAX_LENGTH_ERROR (MAX_LENGTH+32)

	static const int UNICODE_REPLACEMENT_CHAR = 0xfffd;

	static const char *STR_TOO_LONG = "(str too long)";
	static const char *INVALID_UTF8 = "(invalid utf8)";

	wchar_t wline[MAX_LENGTH_ERROR];
	size_t len = 0;

	const char *read = line;
	const char *error = STR_TOO_LONG;
	while(len < MAX_LENGTH)
	{
		// Read a character. This also advances the read pointer
		int glyph = str_utf8_decode(&read);
		if(glyph < 0)
		{
			// If there was an error decoding the UTF-8 sequence,
			// emit a replacement character. Since the
			// str_utf8_decode function will not work after such
			// an error, end the string here.
			glyph = UNICODE_REPLACEMENT_CHAR;
			error = INVALID_UTF8;
			wline[len] = glyph;
			break;
		}
		else if(glyph == 0)
		{
			// A character code of 0 signals the end of the string.
			error = 0;
			break;
		}
		else if(glyph > 0xffff)
		{
			// Since the windows console does not really support
			// UTF-16, don't mind doing actual UTF-16 encoding,
			// but rather emit a replacement character.
			glyph = UNICODE_REPLACEMENT_CHAR;
		}
		else if(glyph == 0x2022)
		{
			// The 'bullet' character might get converted to a 'beep',
			// so it will be replaced by the 'bullet operator'.
			glyph = 0x2219;
		}

		// Again, since the windows console does not really support
		// UTF-16, but rather something along the lines of UCS-2,
		// simply put the character into the output.
		wline[len++] = glyph;
	}

	if(error)
	{
		read = error;
		while(1)
		{
			// Errors are simple ascii, no need for UTF-8
			// decoding
			char character = *read;
			if(character == 0)
				break;

			dbg_assert(len < MAX_LENGTH_ERROR, "str too short for error");
			wline[len++] = (unsigned char)character;
			read++;
		}
	}

	// Terminate the line
	dbg_assert(len < MAX_LENGTH_ERROR, "str too short for \\r");
	wline[len++] = '\r';
	dbg_assert(len < MAX_LENGTH_ERROR, "str too short for \\n");
	wline[len++] = '\n';

	// Ignore any error that might occur
	WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), wline, len, 0, 0);

	#undef MAX_LENGTH
	#undef MAX_LENGTH_ERROR
}
#endif

static void logger_stdout(const char *line, void *user)
{
	printf("%s\n", line);
	fflush(stdout);
}

#if defined(CONF_FAMILY_WINDOWS)
static void logger_win_debugger(const char *line, void *user)
{
	WCHAR wBuffer[512];
	MultiByteToWideChar(CP_UTF8, 0, line, -1, wBuffer, sizeof(wBuffer) / sizeof(WCHAR));
	OutputDebugStringW(wBuffer);
	OutputDebugStringW(L"\n");
}
#endif

static void logger_file(const char *line, void *user)
{
	ASYNCIO *logfile = (ASYNCIO *)user;
	aio_lock(logfile);
	aio_write_unlocked(logfile, line, strlen(line));
	aio_write_newline_unlocked(logfile);
	aio_unlock(logfile);
}

static void logger_stdout_finish(void *user)
{
	ASYNCIO *logfile = (ASYNCIO *)user;
	aio_wait(logfile);
	aio_free(logfile);
}

static void logger_file_finish(void *user)
{
	ASYNCIO *logfile = (ASYNCIO *)user;
	aio_close(logfile);
	logger_stdout_finish(user);
}

void dbg_logger_stdout()
{
#if defined(CONF_FAMILY_WINDOWS)
	if(GetFileType(GetStdHandle(STD_OUTPUT_HANDLE)) == FILE_TYPE_CHAR)
	{
		dbg_logger(logger_win_console, 0, 0);
		return;
	}
#endif
	dbg_logger(logger_stdout, 0, 0);
}

void dbg_logger_debugger()
{
#if defined(CONF_FAMILY_WINDOWS)
	dbg_logger(logger_win_debugger, 0, 0);
#endif
}

void dbg_logger_file(IOHANDLE logfile)
{
	dbg_logger(logger_file, logger_file_finish, aio_new(logfile));
}

#if defined(CONF_FAMILY_WINDOWS)
static DWORD old_console_mode;

void dbg_console_init()
{
	HANDLE handle;
	DWORD console_mode;

	handle = GetStdHandle(STD_INPUT_HANDLE);
	GetConsoleMode(handle, &old_console_mode);
	console_mode = old_console_mode & (~ENABLE_QUICK_EDIT_MODE | ENABLE_EXTENDED_FLAGS);
	SetConsoleMode(handle, console_mode);
}
void dbg_console_cleanup()
{
	SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), old_console_mode);
}

void dbg_console_hide()
{
	FreeConsole();
}
#endif
/* */

void *mem_alloc(unsigned size)
{
	return malloc(size);
}

void mem_free(void *p)
{
	free(p);
}

void mem_copy(void *dest, const void *source, unsigned size)
{
	memcpy(dest, source, size);
}

void mem_move(void *dest, const void *source, unsigned size)
{
	memmove(dest, source, size);
}

void mem_zero(void *block,unsigned size)
{
	memset(block, 0, size);
}

IOHANDLE io_open_impl(const char *filename, int flags)
{
	dbg_assert(flags == (IOFLAG_READ | IOFLAG_SKIP_BOM) || flags == IOFLAG_READ || flags == IOFLAG_WRITE || flags == IOFLAG_APPEND, "flags must be read, read+skipbom, write or append");
#if defined(CONF_FAMILY_WINDOWS)
	if((flags & IOFLAG_READ) != 0)
	{
		// check for filename case sensitive
		WIN32_FIND_DATAW finddata;
		HANDLE handle;
		WCHAR wBuffer[IO_MAX_PATH_LENGTH];
		char buffer[IO_MAX_PATH_LENGTH];

		int length = str_length(filename);
		if(!filename || !length || filename[length-1] == '\\')
			return 0x0;
		MultiByteToWideChar(CP_UTF8, 0, filename, -1, wBuffer, sizeof(wBuffer) / sizeof(WCHAR));
		handle = FindFirstFileW(wBuffer, &finddata);
		if(handle == INVALID_HANDLE_VALUE)
			return 0x0;
		WideCharToMultiByte(CP_UTF8, 0, finddata.cFileName, -1, buffer, sizeof(buffer), NULL, NULL);
		if(str_comp(filename+length-str_length(buffer), buffer) != 0)
		{
			FindClose(handle);
			return 0x0;
		}
		FindClose(handle);
		return (IOHANDLE)_wfsopen(wBuffer, L"rb", _SH_DENYNO);
	}
	if(flags == IOFLAG_WRITE)
	{
		WCHAR wBuffer[IO_MAX_PATH_LENGTH];
		MultiByteToWideChar(CP_UTF8, 0, filename, -1, wBuffer, sizeof(wBuffer) / sizeof(WCHAR));
		return (IOHANDLE)_wfsopen(wBuffer, L"wb", _SH_DENYNO);
	}
	if(flags == IOFLAG_APPEND)
	{
		WCHAR wBuffer[IO_MAX_PATH_LENGTH];
		MultiByteToWideChar(CP_UTF8, 0, filename, -1, wBuffer, sizeof(wBuffer) / sizeof(WCHAR));
		return (IOHANDLE)_wfsopen(wBuffer, L"ab", _SH_DENYNO);
	}
	return 0x0;
#else
	if((flags & IOFLAG_READ) != 0)
		return (IOHANDLE)fopen(filename, "rb");
	if(flags == IOFLAG_WRITE)
		return (IOHANDLE)fopen(filename, "wb");
	if(flags == IOFLAG_APPEND)
		return (IOHANDLE)fopen(filename, "ab");
	return 0x0;
#endif
}

IOHANDLE io_open(const char *filename, int flags)
{
	IOHANDLE result = io_open_impl(filename, flags);
	unsigned char buf[3];
	if((flags & IOFLAG_SKIP_BOM) == 0 || !result)
	{
		return result;
	}
	if(io_read(result, buf, sizeof(buf)) != 3 || buf[0] != 0xef || buf[1] != 0xbb || buf[2] != 0xbf)
	{
		io_seek(result, 0, IOSEEK_START);
	}
	return result;
}

unsigned io_read(IOHANDLE io, void *buffer, unsigned size)
{
	return fread(buffer, 1, size, (FILE*)io);
}

void io_read_all(IOHANDLE io, void **result, unsigned *result_len)
{
	unsigned len = (unsigned)io_length(io);
	char *buffer = mem_alloc(len + 1);
	unsigned read = io_read(io, buffer, len + 1); // +1 to check if the file size is larger than expected
	if(read < len)
	{
		buffer = realloc(buffer, read + 1);
		len = read;
	}
	else if(read > len)
	{
		unsigned cap = 2 * read;
		len = read;
		buffer = realloc(buffer, cap);
		while((read = io_read(io, buffer + len, cap - len)) != 0)
		{
			len += read;
			if(len == cap)
			{
				cap *= 2;
				buffer = realloc(buffer, cap);
			}
		}
		buffer = realloc(buffer, len + 1);
	}
	buffer[len] = 0;
	*result = buffer;
	*result_len = len;
}

char *io_read_all_str(IOHANDLE io)
{
	void *buffer;
	unsigned len;
	io_read_all(io, &buffer, &len);
	if(mem_has_null(buffer, len))
	{
		mem_free(buffer);
		return 0x0;
	}
	return buffer;
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

int io_error(IOHANDLE io)
{
	return ferror((FILE*)io);
}

#define ASYNC_BUFSIZE 8 * 1024
#define ASYNC_LOCAL_BUFSIZE 64 * 1024

struct ASYNCIO
{
	LOCK lock;
	IOHANDLE io;
	SEMAPHORE sphore;
	void *thread;

	unsigned char *buffer;
	unsigned int buffer_size;
	unsigned int read_pos;
	unsigned int write_pos;

	int error;
	unsigned char finish;
	unsigned char refcount;
};

enum
{
	ASYNCIO_RUNNING,
	ASYNCIO_CLOSE,
	ASYNCIO_EXIT,
};

struct BUFFERS
{
	unsigned char *buf1;
	unsigned int len1;
	unsigned char *buf2;
	unsigned int len2;
};

static void buffer_ptrs(ASYNCIO *aio, struct BUFFERS *buffers)
{
	mem_zero(buffers, sizeof(*buffers));
	if(aio->read_pos < aio->write_pos)
	{
		buffers->buf1 = aio->buffer + aio->read_pos;
		buffers->len1 = aio->write_pos - aio->read_pos;
	}
	else if(aio->read_pos > aio->write_pos)
	{
		buffers->buf1 = aio->buffer + aio->read_pos;
		buffers->len1 = aio->buffer_size - aio->read_pos;
		buffers->buf2 = aio->buffer;
		buffers->len2 = aio->write_pos;
	}
}

static void aio_handle_free_and_unlock(ASYNCIO *aio)
{
	int do_free;
	aio->refcount--;

	do_free = aio->refcount == 0;
	lock_unlock(aio->lock);
	if(do_free)
	{
		free(aio->buffer);
		sphore_destroy(&aio->sphore);
		lock_destroy(aio->lock);
		free(aio);
	}
}

static void aio_thread(void *user)
{
	ASYNCIO *aio = user;

	lock_wait(aio->lock);
	while(1)
	{
		struct BUFFERS buffers;
		int result_io_error;
		unsigned char local_buffer[ASYNC_LOCAL_BUFSIZE];
		unsigned int local_buffer_len = 0;

		if(aio->read_pos == aio->write_pos)
		{
			if(aio->finish != ASYNCIO_RUNNING)
			{
				if(aio->finish == ASYNCIO_CLOSE)
				{
					io_close(aio->io);
				}
				aio_handle_free_and_unlock(aio);
				break;
			}
			lock_unlock(aio->lock);
			sphore_wait(&aio->sphore);
			lock_wait(aio->lock);
			continue;
		}

		buffer_ptrs(aio, &buffers);
		if(buffers.buf1)
		{
			if(buffers.len1 > sizeof(local_buffer) - local_buffer_len)
			{
				buffers.len1 = sizeof(local_buffer) - local_buffer_len;
			}
			mem_copy(local_buffer + local_buffer_len, buffers.buf1, buffers.len1);
			local_buffer_len += buffers.len1;
			if(buffers.buf2)
			{
				if(buffers.len2 > sizeof(local_buffer) - local_buffer_len)
				{
					buffers.len2 = sizeof(local_buffer) - local_buffer_len;
				}
				mem_copy(local_buffer + local_buffer_len, buffers.buf2, buffers.len2);
				local_buffer_len += buffers.len2;
			}
		}
		aio->read_pos = (aio->read_pos + buffers.len1 + buffers.len2) % aio->buffer_size;
		lock_unlock(aio->lock);

		io_write(aio->io, local_buffer, local_buffer_len);
		io_flush(aio->io);
		result_io_error = io_error(aio->io);

		lock_wait(aio->lock);
		aio->error = result_io_error;
	}
}

ASYNCIO *aio_new(IOHANDLE io)
{
	ASYNCIO *aio = malloc(sizeof(*aio));
	if(!aio)
	{
		return 0;
	}
	aio->io = io;
	aio->lock = lock_create();
	sphore_init(&aio->sphore);
	aio->thread = 0;

	aio->buffer = malloc(ASYNC_BUFSIZE);
	if(!aio->buffer)
	{
		sphore_destroy(&aio->sphore);
		lock_destroy(aio->lock);
		free(aio);
		return 0;
	}
	aio->buffer_size = ASYNC_BUFSIZE;
	aio->read_pos = 0;
	aio->write_pos = 0;
	aio->error = 0;
	aio->finish = ASYNCIO_RUNNING;
	aio->refcount = 2;

	aio->thread = thread_init(aio_thread, aio);
	if(!aio->thread)
	{
		free(aio->buffer);
		sphore_destroy(&aio->sphore);
		lock_destroy(aio->lock);
		free(aio);
		return 0;
	}
	return aio;
}

static unsigned int buffer_len(ASYNCIO *aio)
{
	if(aio->write_pos >= aio->read_pos)
	{
		return aio->write_pos - aio->read_pos;
	}
	else
	{
		return aio->buffer_size + aio->write_pos - aio->read_pos;
	}
}

static unsigned int next_buffer_size(unsigned int cur_size, unsigned int need_size)
{
	while(cur_size < need_size)
	{
		cur_size *= 2;
	}
	return cur_size;
}

void aio_lock(ASYNCIO *aio)
{
	lock_wait(aio->lock);
}

void aio_unlock(ASYNCIO *aio)
{
	lock_unlock(aio->lock);
	sphore_signal(&aio->sphore);
}

void aio_write_unlocked(ASYNCIO *aio, const void *buffer, unsigned size)
{
	unsigned int remaining;
	remaining = aio->buffer_size - buffer_len(aio);

	// Don't allow full queue to distinguish between empty and full queue.
	if(size < remaining)
	{
		unsigned int remaining_contiguous = aio->buffer_size - aio->write_pos;
		if(size > remaining_contiguous)
		{
			mem_copy(aio->buffer + aio->write_pos, buffer, remaining_contiguous);
			size -= remaining_contiguous;
			buffer = ((unsigned char *)buffer) + remaining_contiguous;
			aio->write_pos = 0;
		}
		mem_copy(aio->buffer + aio->write_pos, buffer, size);
		aio->write_pos = (aio->write_pos + size) % aio->buffer_size;
	}
	else
	{
		// Add 1 so the new buffer isn't completely filled.
		unsigned int new_written = buffer_len(aio) + size + 1;
		unsigned int next_size = next_buffer_size(aio->buffer_size, new_written);
		unsigned int next_len = 0;
		unsigned char *next_buffer = malloc(next_size);

		struct BUFFERS buffers;
		buffer_ptrs(aio, &buffers);
		if(buffers.buf1)
		{
			mem_copy(next_buffer + next_len, buffers.buf1, buffers.len1);
			next_len += buffers.len1;
			if(buffers.buf2)
			{
				mem_copy(next_buffer + next_len, buffers.buf2, buffers.len2);
				next_len += buffers.len2;
			}
		}
		mem_copy(next_buffer + next_len, buffer, size);
		next_len += size;

		free(aio->buffer);
		aio->buffer = next_buffer;
		aio->buffer_size = next_size;
		aio->read_pos = 0;
		aio->write_pos = next_len;
	}
}

void aio_write(ASYNCIO *aio, const void *buffer, unsigned size)
{
	aio_lock(aio);
	aio_write_unlocked(aio, buffer, size);
	aio_unlock(aio);
}

void aio_write_newline_unlocked(ASYNCIO *aio)
{
#if defined(CONF_FAMILY_WINDOWS)
	aio_write_unlocked(aio, "\r\n", 2);
#else
	aio_write_unlocked(aio, "\n", 1);
#endif
}

void aio_write_newline(ASYNCIO *aio)
{
	aio_lock(aio);
	aio_write_newline_unlocked(aio);
	aio_unlock(aio);
}

int aio_error(ASYNCIO *aio)
{
	int result;
	lock_wait(aio->lock);
	result = aio->error;
	lock_unlock(aio->lock);
	return result;
}

void aio_free(ASYNCIO *aio)
{
	lock_wait(aio->lock);
	if(aio->thread)
	{
		thread_detach(aio->thread);
		aio->thread = 0;
	}
	aio_handle_free_and_unlock(aio);
}

void aio_close(ASYNCIO *aio)
{
	lock_wait(aio->lock);
	aio->finish = ASYNCIO_CLOSE;
	lock_unlock(aio->lock);
	sphore_signal(&aio->sphore);
}

void aio_wait(ASYNCIO *aio)
{
	void *thread;
	lock_wait(aio->lock);
	thread = aio->thread;
	aio->thread = 0;
	if(aio->finish == ASYNCIO_RUNNING)
	{
		aio->finish = ASYNCIO_EXIT;
	}
	lock_unlock(aio->lock);
	sphore_signal(&aio->sphore);
	thread_wait(thread);
}

struct THREAD_RUN
{
	void (*threadfunc)(void *);
	void *u;
};

#if defined(CONF_FAMILY_UNIX)
static void *thread_run(void *user)
#elif defined(CONF_FAMILY_WINDOWS)
static unsigned long __stdcall thread_run(void *user)
#else
#error not implemented
#endif
{
	struct THREAD_RUN *data = user;
	void (*threadfunc)(void *) = data->threadfunc;
	void *u = data->u;
	free(data);
	threadfunc(u);
	return 0;
}

void *thread_init(void (*threadfunc)(void *), void *u)
{
	struct THREAD_RUN *data = malloc(sizeof(*data));
	data->threadfunc = threadfunc;
	data->u = u;
#if defined(CONF_FAMILY_UNIX)
	{
		pthread_t id;
		pthread_attr_t attr;
		pthread_attr_init(&attr);
#if defined(CONF_PLATFORM_MACOS)
		pthread_attr_set_qos_class_np(&attr, QOS_CLASS_USER_INTERACTIVE, 0);
#endif
		if(pthread_create(&id, &attr, thread_run, data) != 0)
		{
			return 0;
		}
		return (void*)id;
	}
#elif defined(CONF_FAMILY_WINDOWS)
	return CreateThread(NULL, 0, thread_run, data, 0, NULL);
#else
	#error not implemented
#endif
}

void thread_wait(void *thread)
{
#if defined(CONF_FAMILY_UNIX)
	pthread_join((pthread_t)thread, NULL);
#elif defined(CONF_FAMILY_WINDOWS)
	WaitForSingleObject((HANDLE)thread, INFINITE);
#else
	#error not implemented
#endif
}

void thread_destroy(void *thread)
{
#if defined(CONF_FAMILY_WINDOWS)
	CloseHandle((HANDLE)thread);
#endif
}

void thread_yield()
{
#if defined(CONF_FAMILY_UNIX)
	sched_yield();
#elif defined(CONF_FAMILY_WINDOWS)
	Sleep(0);
#else
	#error not implemented
#endif
}

void thread_sleep(int milliseconds)
{
#if defined(CONF_FAMILY_UNIX)
	usleep(milliseconds*1000);
#elif defined(CONF_FAMILY_WINDOWS)
	Sleep(milliseconds);
#else
	#error not implemented
#endif
}

void thread_detach(void *thread)
{
#if defined(CONF_FAMILY_UNIX)
	pthread_detach((pthread_t)(thread));
#elif defined(CONF_FAMILY_WINDOWS)
	CloseHandle(thread);
#else
	#error not implemented
#endif
}

void cpu_relax()
{
#if defined(CONF_ARCH_IA32) || defined(CONF_ARCH_AMD64)
	_mm_pause();
#else
	(void) 0;
#endif
}



#if defined(CONF_FAMILY_UNIX)
typedef pthread_mutex_t LOCKINTERNAL;
#elif defined(CONF_FAMILY_WINDOWS)
typedef CRITICAL_SECTION LOCKINTERNAL;
#else
	#error not implemented on this platform
#endif

LOCK lock_create()
{
	LOCKINTERNAL *lock = (LOCKINTERNAL*)mem_alloc(sizeof(LOCKINTERNAL));

#if defined(CONF_FAMILY_UNIX)
	pthread_mutex_init(lock, 0x0);
#elif defined(CONF_FAMILY_WINDOWS)
	InitializeCriticalSection((LPCRITICAL_SECTION)lock);
#else
	#error not implemented on this platform
#endif
	return (LOCK)lock;
}

void lock_destroy(LOCK lock)
{
#if defined(CONF_FAMILY_UNIX)
	pthread_mutex_destroy((LOCKINTERNAL *)lock);
#elif defined(CONF_FAMILY_WINDOWS)
	DeleteCriticalSection((LPCRITICAL_SECTION)lock);
#else
	#error not implemented on this platform
#endif
	mem_free(lock);
}

int lock_trylock(LOCK lock)
{
#if defined(CONF_FAMILY_UNIX)
	return pthread_mutex_trylock((LOCKINTERNAL *)lock);
#elif defined(CONF_FAMILY_WINDOWS)
	return !TryEnterCriticalSection((LPCRITICAL_SECTION)lock);
#else
	#error not implemented on this platform
#endif
}

void lock_wait(LOCK lock)
{
#if defined(CONF_FAMILY_UNIX)
	pthread_mutex_lock((LOCKINTERNAL *)lock);
#elif defined(CONF_FAMILY_WINDOWS)
	EnterCriticalSection((LPCRITICAL_SECTION)lock);
#else
	#error not implemented on this platform
#endif
}

void lock_unlock(LOCK lock)
{
#if defined(CONF_FAMILY_UNIX)
	pthread_mutex_unlock((LOCKINTERNAL *)lock);
#elif defined(CONF_FAMILY_WINDOWS)
	LeaveCriticalSection((LPCRITICAL_SECTION)lock);
#else
	#error not implemented on this platform
#endif
}

#if defined(CONF_FAMILY_UNIX) && !defined(CONF_PLATFORM_MACOS) // this should be CONF_POSIX_SEM but bam can't run C programs
void sphore_init(SEMAPHORE *sem) { sem_init(sem, 0, 0); }
void sphore_wait(SEMAPHORE *sem) { sem_wait(sem); }
void sphore_signal(SEMAPHORE *sem) { sem_post(sem); }
void sphore_destroy(SEMAPHORE *sem) { sem_destroy(sem); }
#elif defined(CONF_FAMILY_WINDOWS)
void sphore_init(SEMAPHORE *sem) { *sem = CreateSemaphore(0, 0, 10000, 0); }
void sphore_wait(SEMAPHORE *sem) { WaitForSingleObject((HANDLE)*sem, INFINITE); }
void sphore_signal(SEMAPHORE *sem) { ReleaseSemaphore((HANDLE)*sem, 1, NULL); }
void sphore_destroy(SEMAPHORE *sem) { CloseHandle((HANDLE)*sem); }
#else
typedef struct SEMINTERNAL
{
	int count;
	int waiters;
	LOCK c_lock;
	pthread_cond_t c_nzcond;
} SEMINTERNAL;

void sphore_init(SEMAPHORE *sem)
{
	*sem = mem_alloc(sizeof(**sem));

	(*sem)->count = 0;
	(*sem)->waiters = 0;

	(*sem)->c_lock = lock_create();
	pthread_cond_init(&(*sem)->c_nzcond, 0);
}

void sphore_wait(SEMAPHORE *sem)
{
	int result = 0;

	lock_wait((*sem)->c_lock);

	(*sem)->waiters++;
	while((*sem)->count == 0)
		result = pthread_cond_wait(&(*sem)->c_nzcond, (LOCKINTERNAL *)(*sem)->c_lock);

	(*sem)->waiters--;

	if(!result)
		(*sem)->count--;

	lock_unlock((*sem)->c_lock);
}

void sphore_signal(SEMAPHORE *sem)
{
	lock_wait((*sem)->c_lock);

	if((*sem)->waiters)
		pthread_cond_signal(&(*sem)->c_nzcond);

	(*sem)->count++;
	lock_unlock((*sem)->c_lock);
}

void sphore_destroy(SEMAPHORE *sem)
{
	pthread_cond_destroy(&(*sem)->c_nzcond);
	lock_destroy((*sem)->c_lock);
	mem_free(*sem);
}

#endif


/* -----  time ----- */
int64 time_get()
{
#if defined(CONF_FAMILY_UNIX)
	struct timeval val;
	gettimeofday(&val, NULL);
	return (int64)val.tv_sec*(int64)1000000+(int64)val.tv_usec;
#elif defined(CONF_FAMILY_WINDOWS)
	static int64 last = 0;
	int64 t;
	QueryPerformanceCounter((PLARGE_INTEGER)&t);
	if(t<last) /* for some reason, QPC can return values in the past */
		return last;
	last = t;
	return t;
#else
	#error not implemented
#endif
}

int64 time_freq()
{
#if defined(CONF_FAMILY_UNIX)
	return 1000000;
#elif defined(CONF_FAMILY_WINDOWS)
	int64 t;
	QueryPerformanceFrequency((PLARGE_INTEGER)&t);
	return t;
#else
	#error not implemented
#endif
}

/* -----  network ----- */
static void netaddr_to_sockaddr_in(const NETADDR *src, struct sockaddr_in *dest)
{
	mem_zero(dest, sizeof(struct sockaddr_in));
	if(src->type != NETTYPE_IPV4)
	{
		dbg_msg("system", "couldn't convert NETADDR of type %d to ipv4", src->type);
		return;
	}

	dest->sin_family = AF_INET;
	dest->sin_port = htons(src->port);
	mem_copy(&dest->sin_addr.s_addr, src->ip, 4);
}

static void netaddr_to_sockaddr_in6(const NETADDR *src, struct sockaddr_in6 *dest)
{
	mem_zero(dest, sizeof(struct sockaddr_in6));
	if(src->type != NETTYPE_IPV6)
	{
		dbg_msg("system", "couldn't convert NETADDR of type %d to ipv6", src->type);
		return;
	}

	dest->sin6_family = AF_INET6;
	dest->sin6_port = htons(src->port);
	mem_copy(&dest->sin6_addr.s6_addr, src->ip, 16);
}

static void sockaddr_to_netaddr(const struct sockaddr *src, NETADDR *dst)
{
	if(src->sa_family == AF_INET)
	{
		mem_zero(dst, sizeof(NETADDR));
		dst->type = NETTYPE_IPV4;
		dst->port = htons(((struct sockaddr_in*)src)->sin_port);
		mem_copy(dst->ip, &((struct sockaddr_in*)src)->sin_addr.s_addr, 4);
	}
	else if(src->sa_family == AF_INET6)
	{
		mem_zero(dst, sizeof(NETADDR));
		dst->type = NETTYPE_IPV6;
		dst->port = htons(((struct sockaddr_in6*)src)->sin6_port);
		mem_copy(dst->ip, &((struct sockaddr_in6*)src)->sin6_addr.s6_addr, 16);
	}
	else
	{
		mem_zero(dst, sizeof(struct sockaddr));
		dbg_msg("system", "couldn't convert sockaddr of family %d", src->sa_family);
	}
}

int net_addr_comp(const NETADDR *a, const NETADDR *b, int check_port)
{
	if(a->type == b->type && mem_comp(a->ip, b->ip, a->type == NETTYPE_IPV4 ? NETADDR_SIZE_IPV4 : NETADDR_SIZE_IPV6) == 0 && (!check_port || a->port == b->port))
		return 0;
	return -1;
}

void net_addr_str(const NETADDR *addr, char *string, int max_length, int add_port)
{
	if(addr->type == NETTYPE_IPV4)
	{
		if(add_port != 0)
			str_format(string, max_length, "%d.%d.%d.%d:%d", addr->ip[0], addr->ip[1], addr->ip[2], addr->ip[3], addr->port);
		else
			str_format(string, max_length, "%d.%d.%d.%d", addr->ip[0], addr->ip[1], addr->ip[2], addr->ip[3]);
	}
	else if(addr->type == NETTYPE_IPV6)
	{
		if(add_port != 0)
			str_format(string, max_length, "[%x:%x:%x:%x:%x:%x:%x:%x]:%d",
				(addr->ip[0]<<8)|addr->ip[1], (addr->ip[2]<<8)|addr->ip[3], (addr->ip[4]<<8)|addr->ip[5], (addr->ip[6]<<8)|addr->ip[7],
				(addr->ip[8]<<8)|addr->ip[9], (addr->ip[10]<<8)|addr->ip[11], (addr->ip[12]<<8)|addr->ip[13], (addr->ip[14]<<8)|addr->ip[15],
				addr->port);
		else
			str_format(string, max_length, "[%x:%x:%x:%x:%x:%x:%x:%x]",
				(addr->ip[0]<<8)|addr->ip[1], (addr->ip[2]<<8)|addr->ip[3], (addr->ip[4]<<8)|addr->ip[5], (addr->ip[6]<<8)|addr->ip[7],
				(addr->ip[8]<<8)|addr->ip[9], (addr->ip[10]<<8)|addr->ip[11], (addr->ip[12]<<8)|addr->ip[13], (addr->ip[14]<<8)|addr->ip[15]);
	}
	else
		str_format(string, max_length, "unknown type %d", addr->type);
}

static int priv_net_extract(const char *hostname, char *host, int max_host, int *port)
{
	int i;

	*port = 0;
	host[0] = 0;

	if(hostname[0] == '[')
	{
		// ipv6 mode
		for(i = 1; i < max_host && hostname[i] && hostname[i] != ']'; i++)
			host[i-1] = hostname[i];
		host[i-1] = 0;
		if(hostname[i] != ']') // malformatted
			return -1;

		i++;
		if(hostname[i] == ':')
			*port = atol(hostname+i+1);
	}
	else
	{
		// generic mode (ipv4, hostname etc)
		for(i = 0; i < max_host-1 && hostname[i] && hostname[i] != ':'; i++)
			host[i] = hostname[i];
		host[i] = 0;

		if(hostname[i] == ':')
			*port = atol(hostname+i+1);
	}

	return 0;
}

int net_host_lookup(const char *hostname, NETADDR *addr, int types)
{
	struct addrinfo hints;
	struct addrinfo *result;
	int e;
	char host[256];
	int port = 0;

	if(priv_net_extract(hostname, host, sizeof(host), &port))
		return -1;
	/*
	dbg_msg("host lookup", "host='%s' port=%d %d", host, port, types);
	*/

	mem_zero(&hints, sizeof(hints));

	hints.ai_family = AF_UNSPEC;

	if(types == NETTYPE_IPV4)
		hints.ai_family = AF_INET;
	else if(types == NETTYPE_IPV6)
		hints.ai_family = AF_INET6;

	e = getaddrinfo(host, NULL, &hints, &result);
	if(e != 0 || !result)
		return -1;

	sockaddr_to_netaddr(result->ai_addr, addr);
	freeaddrinfo(result);
	addr->port = port;
	return 0;
}

static int parse_int(int *out, const char **str)
{
	int i = 0;
	*out = 0;
	if(**str < '0' || **str > '9')
		return -1;

	i = **str - '0';
	(*str)++;

	while(1)
	{
		if(**str < '0' || **str > '9')
		{
			*out = i;
			return 0;
		}

		i = (i*10) + (**str - '0');
		(*str)++;
	}

	return 0;
}

static int parse_char(char c, const char **str)
{
	if(**str != c) return -1;
	(*str)++;
	return 0;
}

static int parse_uint8(unsigned char *out, const char **str)
{
	int i;
	if(parse_int(&i, str) != 0) return -1;
	if(i < 0 || i > 0xff) return -1;
	*out = i;
	return 0;
}

static int parse_uint16(unsigned short *out, const char **str)
{
	int i;
	if(parse_int(&i, str) != 0) return -1;
	if(i < 0 || i > 0xffff) return -1;
	*out = i;
	return 0;
}

int net_addr_from_str(NETADDR *addr, const char *string)
{
	const char *str = string;
	mem_zero(addr, sizeof(NETADDR));

	if(str[0] == '[')
	{
		/* ipv6 */
		struct sockaddr_in6 sa6;
		char buf[128];
		int i;
		str++;
		for(i = 0; i < 127 && str[i] && str[i] != ']'; i++)
			buf[i] = str[i];
		buf[i] = 0;
		str += i;
#if defined(CONF_FAMILY_WINDOWS)
		{
			int size;
			sa6.sin6_family = AF_INET6;
			size = (int)sizeof(sa6);
			if(WSAStringToAddressA(buf, AF_INET6, NULL, (struct sockaddr *)&sa6, &size) != 0)
				return -1;
		}
#else
		sa6.sin6_family = AF_INET6;

		if(inet_pton(AF_INET6, buf, &sa6.sin6_addr) != 1)
			return -1;
#endif
		sockaddr_to_netaddr((struct sockaddr *)&sa6, addr);

		if(*str == ']')
		{
			str++;
			if(*str == ':')
			{
				str++;
				if(parse_uint16(&addr->port, &str))
					return -1;
			}
		}
		else
			return -1;

		return 0;
	}
	else
	{
		/* ipv4 */
		if(parse_uint8(&addr->ip[0], &str)) return -1;
		if(parse_char('.', &str)) return -1;
		if(parse_uint8(&addr->ip[1], &str)) return -1;
		if(parse_char('.', &str)) return -1;
		if(parse_uint8(&addr->ip[2], &str)) return -1;
		if(parse_char('.', &str)) return -1;
		if(parse_uint8(&addr->ip[3], &str)) return -1;
		if(*str == ':')
		{
			str++;
			if(parse_uint16(&addr->port, &str)) return -1;
		}

		addr->type = NETTYPE_IPV4;
	}

	return 0;
}

static void priv_net_close_socket(int sock)
{
#if defined(CONF_FAMILY_WINDOWS)
	closesocket(sock);
#else
	close(sock);
#endif
}

static int priv_net_close_all_sockets(NETSOCKET sock)
{
	/* close down ipv4 */
	if(sock.ipv4sock >= 0)
	{
		priv_net_close_socket(sock.ipv4sock);
		sock.ipv4sock = -1;
		sock.type &= ~NETTYPE_IPV4;
	}

	/* close down ipv6 */
	if(sock.ipv6sock >= 0)
	{
		priv_net_close_socket(sock.ipv6sock);
		sock.ipv6sock = -1;
		sock.type &= ~NETTYPE_IPV6;
	}
	return 0;
}

static int priv_net_create_socket(int domain, int type, struct sockaddr *addr, int sockaddrlen, int use_random_port)
{
	int sock, e;

	/* create socket */
	sock = socket(domain, type, 0);
	if(sock < 0)
	{
#if defined(CONF_FAMILY_WINDOWS)
		char buf[128];
		WCHAR wBuffer[128];
		int error = WSAGetLastError();
		if(FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, error, 0, wBuffer, sizeof(wBuffer) / sizeof(WCHAR), 0) == 0)
			wBuffer[0] = 0;
		WideCharToMultiByte(CP_UTF8, 0, wBuffer, -1, buf, sizeof(buf), NULL, NULL);
		dbg_msg("net", "failed to create socket with domain %d and type %d (%d '%s')", domain, type, error, buf);
#else
		dbg_msg("net", "failed to create socket with domain %d and type %d (%d '%s')", domain, type, errno, strerror(errno));
#endif
		return -1;
	}

	/* set to IPv6 only if thats what we are creating */
#if defined(IPV6_V6ONLY)	/* windows sdk 6.1 and higher */
	if(domain == AF_INET6)
	{
		int ipv6only = 1;
		setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, (const char*)&ipv6only, sizeof(ipv6only));
	}
#endif

	/* bind the socket */
	while(1)
	{
		/* pick random port */
		if(use_random_port)
		{
			int port = htons(rand()%16384+49152);	/* 49152 to 65535 */
			if(domain == AF_INET)
				((struct sockaddr_in *)(addr))->sin_port = port;
			else
				((struct sockaddr_in6 *)(addr))->sin6_port = port;
		}

		e = bind(sock, addr, sockaddrlen);
		if(e == 0)
			break;
		else
		{
#if defined(CONF_FAMILY_WINDOWS)
			char buf[128];
			WCHAR wBuffer[128];
			int error = WSAGetLastError();
			if(error == WSAEADDRINUSE && use_random_port)
				continue;
			if(FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, error, 0, wBuffer, sizeof(wBuffer) / sizeof(WCHAR), 0) == 0)
				wBuffer[0] = 0;
			WideCharToMultiByte(CP_UTF8, 0, wBuffer, -1, buf, sizeof(buf), NULL, NULL);
			dbg_msg("net", "failed to bind socket with domain %d and type %d (%d '%s')", domain, type, error, buf);
#else
			if(errno == EADDRINUSE && use_random_port)
				continue;
			dbg_msg("net", "failed to bind socket with domain %d and type %d (%d '%s')", domain, type, errno, strerror(errno));
#endif
			priv_net_close_socket(sock);
			return -1;
		}
	}

	/* return the newly created socket */
	return sock;
}

NETSOCKET net_udp_create(NETADDR bindaddr, int use_random_port)
{
	NETSOCKET sock = invalid_socket;
	NETADDR tmpbindaddr = bindaddr;
	int broadcast = 1;
	int recvsize = 65536;

	if(bindaddr.type&NETTYPE_IPV4)
	{
		struct sockaddr_in addr;
		int socket = -1;

		/* bind, we should check for error */
		tmpbindaddr.type = NETTYPE_IPV4;
		netaddr_to_sockaddr_in(&tmpbindaddr, &addr);
		socket = priv_net_create_socket(AF_INET, SOCK_DGRAM, (struct sockaddr *)&addr, sizeof(addr), use_random_port);
		if(socket >= 0)
		{
			sock.type |= NETTYPE_IPV4;
			sock.ipv4sock = socket;

			/* set broadcast */
			setsockopt(socket, SOL_SOCKET, SO_BROADCAST, (const char*)&broadcast, sizeof(broadcast));

			/* set receive buffer size */
			setsockopt(socket, SOL_SOCKET, SO_RCVBUF, (char*)&recvsize, sizeof(recvsize));
		}
	}

	if(bindaddr.type&NETTYPE_IPV6)
	{
		struct sockaddr_in6 addr;
		int socket = -1;

		/* bind, we should check for error */
		tmpbindaddr.type = NETTYPE_IPV6;
		netaddr_to_sockaddr_in6(&tmpbindaddr, &addr);
		socket = priv_net_create_socket(AF_INET6, SOCK_DGRAM, (struct sockaddr *)&addr, sizeof(addr), use_random_port);
		if(socket >= 0)
		{
			sock.type |= NETTYPE_IPV6;
			sock.ipv6sock = socket;

			/* set broadcast */
			setsockopt(socket, SOL_SOCKET, SO_BROADCAST, (const char*)&broadcast, sizeof(broadcast));

			/* set receive buffer size */
			setsockopt(socket, SOL_SOCKET, SO_RCVBUF, (char*)&recvsize, sizeof(recvsize));
		}
	}

	/* set non-blocking */
	net_set_non_blocking(sock);

	/* return */
	return sock;
}

int net_udp_send(NETSOCKET sock, const NETADDR *addr, const void *data, int size)
{
	int d = -1;

	if(addr->type&NETTYPE_IPV4)
	{
		if(sock.ipv4sock >= 0)
		{
			struct sockaddr_in sa;
			if(addr->type&NETTYPE_LINK_BROADCAST)
			{
				mem_zero(&sa, sizeof(sa));
				sa.sin_port = htons(addr->port);
				sa.sin_family = AF_INET;
				sa.sin_addr.s_addr = INADDR_BROADCAST;
			}
			else
				netaddr_to_sockaddr_in(addr, &sa);

			d = sendto((int)sock.ipv4sock, (const char*)data, size, 0, (struct sockaddr *)&sa, sizeof(sa));
		}
		else
			dbg_msg("net", "can't send ipv4 traffic to this socket");
	}

	if(addr->type&NETTYPE_IPV6)
	{
		if(sock.ipv6sock >= 0)
		{
			struct sockaddr_in6 sa;
			if(addr->type&NETTYPE_LINK_BROADCAST)
			{
				mem_zero(&sa, sizeof(sa));
				sa.sin6_port = htons(addr->port);
				sa.sin6_family = AF_INET6;
				sa.sin6_addr.s6_addr[0] = 0xff; /* multicast */
				sa.sin6_addr.s6_addr[1] = 0x02; /* link local scope */
				sa.sin6_addr.s6_addr[15] = 1; /* all nodes */
			}
			else
				netaddr_to_sockaddr_in6(addr, &sa);

			d = sendto((int)sock.ipv6sock, (const char*)data, size, 0, (struct sockaddr *)&sa, sizeof(sa));
		}
		else
			dbg_msg("net", "can't send ipv6 traffic to this socket");
	}
	/*
	else
		dbg_msg("net", "can't send to network of type %d", addr->type);
		*/

	/*if(d < 0)
	{
		char addrstr[256];
		net_addr_str(addr, addrstr, sizeof(addrstr));

		dbg_msg("net", "sendto error (%d '%s')", errno, strerror(errno));
		dbg_msg("net", "\tsock = %d %x", sock, sock);
		dbg_msg("net", "\tsize = %d %x", size, size);
		dbg_msg("net", "\taddr = %s", addrstr);

	}*/
	network_stats.sent_bytes += size;
	network_stats.sent_packets++;
	return d;
}

int net_udp_recv(NETSOCKET sock, NETADDR *addr, void *data, int maxsize)
{
	char sockaddrbuf[128];
	socklen_t fromlen;// = sizeof(sockaddrbuf);
	int bytes = 0;

	if(sock.ipv4sock >= 0)
	{
		fromlen = sizeof(struct sockaddr_in);
		bytes = recvfrom(sock.ipv4sock, (char*)data, maxsize, 0, (struct sockaddr *)&sockaddrbuf, &fromlen);
	}

	if(bytes <= 0 && sock.ipv6sock >= 0)
	{
		fromlen = sizeof(struct sockaddr_in6);
		bytes = recvfrom(sock.ipv6sock, (char*)data, maxsize, 0, (struct sockaddr *)&sockaddrbuf, &fromlen);
	}

	if(bytes > 0)
	{
		sockaddr_to_netaddr((struct sockaddr *)&sockaddrbuf, addr);
		network_stats.recv_bytes += bytes;
		network_stats.recv_packets++;
		return bytes;
	}
	else if(bytes == 0)
		return 0;
	return -1; /* error */
}

int net_udp_close(NETSOCKET sock)
{
	return priv_net_close_all_sockets(sock);
}

NETSOCKET net_tcp_create(NETADDR bindaddr)
{
	NETSOCKET sock = invalid_socket;
	NETADDR tmpbindaddr = bindaddr;

	if(bindaddr.type&NETTYPE_IPV4)
	{
		struct sockaddr_in addr;
		int socket = -1;

		/* bind, we should check for error */
		tmpbindaddr.type = NETTYPE_IPV4;
		netaddr_to_sockaddr_in(&tmpbindaddr, &addr);
		socket = priv_net_create_socket(AF_INET, SOCK_STREAM, (struct sockaddr *)&addr, sizeof(addr), 0);
		if(socket >= 0)
		{
			sock.type |= NETTYPE_IPV4;
			sock.ipv4sock = socket;
		}
	}

	if(bindaddr.type&NETTYPE_IPV6)
	{
		struct sockaddr_in6 addr;
		int socket = -1;

		/* bind, we should check for error */
		tmpbindaddr.type = NETTYPE_IPV6;
		netaddr_to_sockaddr_in6(&tmpbindaddr, &addr);
		socket = priv_net_create_socket(AF_INET6, SOCK_STREAM, (struct sockaddr *)&addr, sizeof(addr), 0);
		if(socket >= 0)
		{
			sock.type |= NETTYPE_IPV6;
			sock.ipv6sock = socket;
		}
	}

	/* return */
	return sock;
}

int net_tcp_set_linger(NETSOCKET sock, int state)
{
	struct linger linger_state;
	linger_state.l_onoff = state;
	linger_state.l_linger = 0;

	if(sock.ipv4sock >= 0)
	{
		/*	set linger	*/
		setsockopt(sock.ipv4sock, SOL_SOCKET, SO_LINGER, (const char*)&linger_state, sizeof(linger_state));
	}

	if(sock.ipv6sock >= 0)
	{
		/*	set linger	*/
		setsockopt(sock.ipv6sock, SOL_SOCKET, SO_LINGER, (const char*)&linger_state, sizeof(linger_state));
	}

	return 0;
}

int net_set_non_blocking(NETSOCKET sock)
{
	unsigned long mode = 1;
	if(sock.ipv4sock >= 0)
	{
#if defined(CONF_FAMILY_WINDOWS)
		ioctlsocket(sock.ipv4sock, FIONBIO, (unsigned long *)&mode);
#else
		ioctl(sock.ipv4sock, FIONBIO, (unsigned long *)&mode);
#endif
	}

	if(sock.ipv6sock >= 0)
	{
#if defined(CONF_FAMILY_WINDOWS)
		ioctlsocket(sock.ipv6sock, FIONBIO, (unsigned long *)&mode);
#else
		ioctl(sock.ipv6sock, FIONBIO, (unsigned long *)&mode);
#endif
	}

	return 0;
}

int net_set_blocking(NETSOCKET sock)
{
	unsigned long mode = 0;
	if(sock.ipv4sock >= 0)
	{
#if defined(CONF_FAMILY_WINDOWS)
		ioctlsocket(sock.ipv4sock, FIONBIO, (unsigned long *)&mode);
#else
		ioctl(sock.ipv4sock, FIONBIO, (unsigned long *)&mode);
#endif
	}

	if(sock.ipv6sock >= 0)
	{
#if defined(CONF_FAMILY_WINDOWS)
		ioctlsocket(sock.ipv6sock, FIONBIO, (unsigned long *)&mode);
#else
		ioctl(sock.ipv6sock, FIONBIO, (unsigned long *)&mode);
#endif
	}

	return 0;
}

int net_tcp_listen(NETSOCKET sock, int backlog)
{
	int err = -1;
	if(sock.ipv4sock >= 0)
		err = listen(sock.ipv4sock, backlog);
	if(sock.ipv6sock >= 0)
		err = listen(sock.ipv6sock, backlog);
	return err;
}

int net_tcp_accept(NETSOCKET sock, NETSOCKET *new_sock, NETADDR *a)
{
	int s;
	socklen_t sockaddr_len;

	*new_sock = invalid_socket;

	if(sock.ipv4sock >= 0)
	{
		struct sockaddr_in addr;
		sockaddr_len = sizeof(addr);

		s = accept(sock.ipv4sock, (struct sockaddr *)&addr, &sockaddr_len);

		if (s != -1)
		{
			sockaddr_to_netaddr((const struct sockaddr *)&addr, a);
			new_sock->type = NETTYPE_IPV4;
			new_sock->ipv4sock = s;
			return s;
		}
	}

	if(sock.ipv6sock >= 0)
	{
		struct sockaddr_in6 addr;
		sockaddr_len = sizeof(addr);

		s = accept(sock.ipv6sock, (struct sockaddr *)&addr, &sockaddr_len);

		if (s != -1)
		{
			sockaddr_to_netaddr((const struct sockaddr *)&addr, a);
			new_sock->type = NETTYPE_IPV6;
			new_sock->ipv6sock = s;
			return s;
		}
	}

	return -1;
}

int net_tcp_connect(NETSOCKET sock, const NETADDR *a)
{
	if(a->type&NETTYPE_IPV4)
	{
		struct sockaddr_in addr;
		netaddr_to_sockaddr_in(a, &addr);
		return connect(sock.ipv4sock, (struct sockaddr *)&addr, sizeof(addr));
	}

	if(a->type&NETTYPE_IPV6)
	{
		struct sockaddr_in6 addr;
		netaddr_to_sockaddr_in6(a, &addr);
		return connect(sock.ipv6sock, (struct sockaddr *)&addr, sizeof(addr));
	}

	return -1;
}

int net_tcp_connect_non_blocking(NETSOCKET sock, NETADDR bindaddr)
{
	int res = 0;

	net_set_non_blocking(sock);
	res = net_tcp_connect(sock, &bindaddr);
	net_set_blocking(sock);

	return res;
}

int net_tcp_send(NETSOCKET sock, const void *data, int size)
{
	int bytes = -1;

	if(sock.ipv4sock >= 0)
		bytes = send((int)sock.ipv4sock, (const char*)data, size, 0);
	if(sock.ipv6sock >= 0)
		bytes = send((int)sock.ipv6sock, (const char*)data, size, 0);

	return bytes;
}

int net_tcp_recv(NETSOCKET sock, void *data, int maxsize)
{
	int bytes = -1;

	if(sock.ipv4sock >= 0)
		bytes = recv((int)sock.ipv4sock, (char*)data, maxsize, 0);
	if(sock.ipv6sock >= 0)
		bytes = recv((int)sock.ipv6sock, (char*)data, maxsize, 0);

	return bytes;
}

int net_tcp_close(NETSOCKET sock)
{
	return priv_net_close_all_sockets(sock);
}

int net_errno()
{
#if defined(CONF_FAMILY_WINDOWS)
	return WSAGetLastError();
#else
	return errno;
#endif
}

int net_would_block()
{
#if defined(CONF_FAMILY_WINDOWS)
	return net_errno() == WSAEWOULDBLOCK;
#else
	return net_errno() == EWOULDBLOCK;
#endif
}

void net_invalidate_socket(NETSOCKET *socket)
{
	*socket = invalid_socket;
}

int net_init()
{
#if defined(CONF_FAMILY_WINDOWS)
	WSADATA wsaData;
	int err = WSAStartup(MAKEWORD(1, 1), &wsaData);
	dbg_assert(err == 0, "network initialization failed.");
	return err==0?0:1;
#endif

	return 0;
}

#if defined (CONF_FAMILY_WINDOWS)
static inline time_t filetime_to_unixtime(LPFILETIME filetime)
{
	time_t t;
	ULARGE_INTEGER li;
	li.LowPart = filetime->dwLowDateTime;
	li.HighPart = filetime->dwHighDateTime;

	li.QuadPart /= 10000000; // 100ns to 1s
	li.QuadPart -= 11644473600LL; // Windows epoch is in the past

	t = li.QuadPart;
	return t == (time_t)li.QuadPart ? t : (time_t)-1;
}
#endif

void fs_listdir(const char *dir, FS_LISTDIR_CALLBACK cb, int type, void *user)
{
#if defined(CONF_FAMILY_WINDOWS)
	WIN32_FIND_DATAW finddata;
	HANDLE handle;
	char buffer[IO_MAX_PATH_LENGTH];
	char buffer2[IO_MAX_PATH_LENGTH];
	WCHAR wBuffer[IO_MAX_PATH_LENGTH];
	int length;

	str_format(buffer, sizeof(buffer), "%s/*", dir);
	MultiByteToWideChar(CP_UTF8, 0, buffer, -1, wBuffer, sizeof(wBuffer) / sizeof(WCHAR));

	handle = FindFirstFileW(wBuffer, &finddata);
	if(handle == INVALID_HANDLE_VALUE)
		return;

	str_format(buffer, sizeof(buffer), "%s/", dir);
	length = str_length(buffer);

	/* add all the entries */
	do
	{
		WideCharToMultiByte(CP_UTF8, 0, finddata.cFileName, -1, buffer2, sizeof(buffer2), NULL, NULL);
		str_copy(buffer+length, buffer2, (int)sizeof(buffer)-length);
		if(cb(buffer2, (finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0, type, user))
			break;
	}
	while(FindNextFileW(handle, &finddata));

	FindClose(handle);
#else
	struct dirent *entry;
	char buffer[IO_MAX_PATH_LENGTH];
	int length;
	DIR *d = opendir(dir);

	if(!d)
		return;

	str_format(buffer, sizeof(buffer), "%s/", dir);
	length = str_length(buffer);

	while((entry = readdir(d)) != NULL)
	{
		str_copy(buffer+length, entry->d_name, (int)sizeof(buffer)-length);
		if(cb(entry->d_name, entry->d_type == DT_UNKNOWN ? fs_is_dir(buffer) : entry->d_type == DT_DIR, type, user))
			break;
	}

	/* close the directory and return */
	closedir(d);
#endif
}

void fs_listdir_fileinfo(const char *dir, FS_LISTDIR_CALLBACK_FILEINFO cb, int type, void *user)
{
#if defined(CONF_FAMILY_WINDOWS)
	WIN32_FIND_DATAW finddata;
	HANDLE handle;
	char buffer[IO_MAX_PATH_LENGTH];
	char buffer2[IO_MAX_PATH_LENGTH];
	WCHAR wBuffer[IO_MAX_PATH_LENGTH];
	int length;

	str_format(buffer, sizeof(buffer), "%s/*", dir);
	MultiByteToWideChar(CP_UTF8, 0, buffer, -1, wBuffer, sizeof(wBuffer) / sizeof(WCHAR));

	handle = FindFirstFileW(wBuffer, &finddata);
	if(handle == INVALID_HANDLE_VALUE)
		return;

	str_format(buffer, sizeof(buffer), "%s/", dir);
	length = str_length(buffer);

	/* add all the entries */
	do
	{
		CFsFileInfo info;
		WideCharToMultiByte(CP_UTF8, 0, finddata.cFileName, -1, buffer2, sizeof(buffer2), NULL, NULL);
		str_copy(buffer+length, buffer2, (int)sizeof(buffer)-length);

		info.m_pName = buffer2;
		info.m_TimeCreated = filetime_to_unixtime(&finddata.ftCreationTime);
		info.m_TimeModified = filetime_to_unixtime(&finddata.ftLastWriteTime);

		if(cb(&info, (finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0, type, user))
			break;
	}
	while(FindNextFileW(handle, &finddata));

	FindClose(handle);
#else
	struct dirent *entry;
	time_t created = -1, modified = -1;
	char buffer[IO_MAX_PATH_LENGTH];
	int length;
	DIR *d = opendir(dir);

	if(!d)
		return;

	str_format(buffer, sizeof(buffer), "%s/", dir);
	length = str_length(buffer);

	while((entry = readdir(d)) != NULL)
	{
		CFsFileInfo info;

		str_copy(buffer+length, entry->d_name, (int)sizeof(buffer)-length);
		fs_file_time(buffer, &created, &modified);

		info.m_pName = entry->d_name;
		info.m_TimeCreated = created;
		info.m_TimeModified = modified;

		if(cb(&info, entry->d_type == DT_UNKNOWN ? fs_is_dir(buffer) : entry->d_type == DT_DIR, type, user))
			break;
	}

	/* close the directory and return */
	closedir(d);
#endif
}

int fs_storage_path(const char *appname, char *path, int max)
{
#if defined(CONF_FAMILY_WINDOWS)
	WCHAR *home = _wgetenv(L"APPDATA");
	char buffer[IO_MAX_PATH_LENGTH];
	if(!home)
		return -1;
	WideCharToMultiByte(CP_UTF8, 0, home, -1, buffer, sizeof(buffer), NULL, NULL);
	str_format(path, max, "%s/%s", buffer, appname);
	return 0;
#else
	char *home = getenv("HOME");
	int i;
	char *xdgdatahome = getenv("XDG_DATA_HOME");
	char xdgpath[max];

	if(!home)
		return -1;

#if defined(CONF_PLATFORM_MACOS)
	str_format(path, max, "%s/Library/Application Support/%s", home, appname);
	return 0;
#endif

	/* old folder location */
	str_format(path, max, "%s/.%s", home, appname);
	for(i = str_length(home)+2; path[i]; i++)
		path[i] = tolower(path[i]);

	if(!xdgdatahome)
	{
		/* use default location */
		str_format(xdgpath, max, "%s/.local/share/%s", home, appname);
		for(i = str_length(home)+14; xdgpath[i]; i++)
			xdgpath[i] = tolower(xdgpath[i]);
	}
	else
	{
		str_format(xdgpath, max, "%s/%s", xdgdatahome, appname);
		for(i = str_length(xdgdatahome)+1; xdgpath[i]; i++)
			xdgpath[i] = tolower(xdgpath[i]);
	}

	/* check for old location / backward compatibility */
	if(fs_is_dir(path))
	{
		/* use old folder path */
		/* for backward compatibility */
		return 0;
	}

	str_format(path, max, "%s", xdgpath);

	return 0;
#endif
}

int fs_makedir(const char *path)
{
#if defined(CONF_FAMILY_WINDOWS)
	WCHAR wBuffer[IO_MAX_PATH_LENGTH];
	MultiByteToWideChar(CP_UTF8, 0, path, -1, wBuffer, sizeof(wBuffer) / sizeof(WCHAR));
	if(_wmkdir(wBuffer) == 0)
		return 0;
	if(errno == EEXIST)
		return 0;
	return -1;
#else
	if(mkdir(path, 0755) == 0)
		return 0;
	if(errno == EEXIST)
		return 0;
	return -1;
#endif
}

int fs_makedir_recursive(const char *path)
{
	char buffer[2048];
	int len;
	int i;
	str_copy(buffer, path, sizeof(buffer));
	len = str_length(buffer);
	// ignore a leading slash
	for(i = 1; i < len; i++)
	{
		char b = buffer[i];
		if(b == '/' || (b == '\\' && buffer[i-1] != ':'))
		{
			buffer[i] = 0;
			if(fs_makedir(buffer) < 0)
			{
				return -1;
			}
			buffer[i] = b;

		}
	}
	return fs_makedir(path);
}

int fs_is_dir(const char *path)
{
#if defined(CONF_FAMILY_WINDOWS)
	WCHAR wPath[IO_MAX_PATH_LENGTH];
	DWORD attributes;
	MultiByteToWideChar(CP_UTF8, 0, path, -1, wPath, sizeof(wPath) / sizeof(WCHAR));
	attributes = GetFileAttributesW(wPath);
	return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) ? 1 : 0;
#else
	struct stat sb;
	if(stat(path, &sb) == -1)
		return 0;
	return S_ISDIR(sb.st_mode) ? 1 : 0;
#endif
}

int fs_chdir(const char *path)
{
	if(fs_is_dir(path))
	{
#if defined(CONF_FAMILY_WINDOWS)
		WCHAR wBuffer[IO_MAX_PATH_LENGTH];
		MultiByteToWideChar(CP_UTF8, 0, path, -1, wBuffer, sizeof(wBuffer) / sizeof(WCHAR));
		if(_wchdir(wBuffer))
			return 1;
		else
			return 0;
#else
		if(chdir(path))
			return 1;
		else
			return 0;
#endif
	}
	else
		return 1;
}

char *fs_getcwd(char *buffer, int buffer_size)
{
#if defined(CONF_FAMILY_WINDOWS)
	WCHAR wBuffer[IO_MAX_PATH_LENGTH];
#endif
	dbg_assert(buffer != 0, "buffer invalid");
	dbg_assert(buffer_size > 0, "buffer_size invalid");
#if defined(CONF_FAMILY_WINDOWS)
	if(_wgetcwd(wBuffer, buffer_size) == 0)
	{
		buffer[0] = '\0';
		return 0;
	}
	WideCharToMultiByte(CP_UTF8, 0, wBuffer, -1, buffer, buffer_size, NULL, NULL);
	return buffer;
#else
	if(getcwd(buffer, buffer_size) == 0)
	{
		buffer[0] = '\0';
		return 0;
	}
	return buffer;
#endif
}

int fs_parent_dir(char *path)
{
	char *parent = 0;
	for(; *path; ++path)
	{
		if(*path == '/' || *path == '\\')
			parent = path;
	}

	if(parent)
	{
		*parent = 0;
		return 0;
	}
	return 1;
}

int fs_remove(const char *filename)
{
#if defined(CONF_FAMILY_WINDOWS)
	WCHAR wFilename[IO_MAX_PATH_LENGTH];
	MultiByteToWideChar(CP_UTF8, 0, filename, -1, wFilename, sizeof(wFilename) / sizeof(WCHAR));
	if(DeleteFileW(wFilename) == 0)
		return 1;
	return 0;
#else
	if(remove(filename))
		return 1;
	return 0;
#endif
}

int fs_rename(const char *oldname, const char *newname)
{
#if defined(CONF_FAMILY_WINDOWS)
	WCHAR wOldname[IO_MAX_PATH_LENGTH];
	WCHAR wNewname[IO_MAX_PATH_LENGTH];
	MultiByteToWideChar(CP_UTF8, 0, oldname, -1, wOldname, sizeof(wOldname) / sizeof(WCHAR));
	MultiByteToWideChar(CP_UTF8, 0, newname, -1, wNewname, sizeof(wNewname) / sizeof(WCHAR));
	if(MoveFileExW(wOldname, wNewname, MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED) == 0)
		return 1;
	return 0;
#else
	if(rename(oldname, newname))
		return 1;
	return 0;
#endif
}

int fs_read(const char *name, void **result, unsigned *result_len)
{
	IOHANDLE file = io_open(name, IOFLAG_READ);
	*result = 0;
	*result_len = 0;
	if(!file)
	{
		return 1;
	}
	io_read_all(file, result, result_len);
	io_close(file);
	return 0;
}

char *fs_read_str(const char *name)
{
	IOHANDLE file = io_open(name, IOFLAG_READ | IOFLAG_SKIP_BOM);
	char *result;
	if(!file)
	{
		return 0;
	}
	result = io_read_all_str(file);
	io_close(file);
	return result;
}

int fs_file_time(const char *name, time_t *created, time_t *modified)
{
#if defined(CONF_FAMILY_WINDOWS)
	WIN32_FIND_DATAW finddata;
	HANDLE handle;
	WCHAR wBuffer[IO_MAX_PATH_LENGTH];

	MultiByteToWideChar(CP_UTF8, 0, name, -1, wBuffer, sizeof(wBuffer) / sizeof(WCHAR));
	handle = FindFirstFileW(wBuffer, &finddata);
	if(handle == INVALID_HANDLE_VALUE)
		return 1;

	*created = filetime_to_unixtime(&finddata.ftCreationTime);
	*modified = filetime_to_unixtime(&finddata.ftLastWriteTime);
	FindClose(handle);
#elif defined(CONF_FAMILY_UNIX)
	struct stat sb;
	if(stat(name, &sb))
		return 1;

	*created = sb.st_ctime;
	*modified = sb.st_mtime;
#else
	#error not implemented
#endif

	return 0;
}

void swap_endian(void *data, unsigned elem_size, unsigned num)
{
	char *src = (char*) data;
	char *dst = src + (elem_size - 1);

	while(num)
	{
		unsigned n = elem_size>>1;
		char tmp;
		while(n)
		{
			tmp = *src;
			*src = *dst;
			*dst = tmp;

			src++;
			dst--;
			n--;
		}

		src = src + (elem_size>>1);
		dst = src + (elem_size - 1);
		num--;
	}
}

int net_socket_read_wait(NETSOCKET sock, int time)
{
	struct timeval tv;
	fd_set readfds;
	int sockid;

	tv.tv_sec = 0;
	tv.tv_usec = 1000*time;
	sockid = 0;

	FD_ZERO(&readfds);
	if(sock.ipv4sock >= 0)
	{
		FD_SET(sock.ipv4sock, &readfds);
		sockid = sock.ipv4sock;
	}
	if(sock.ipv6sock >= 0)
	{
		FD_SET(sock.ipv6sock, &readfds);
		if(sock.ipv6sock > sockid)
			sockid = sock.ipv6sock;
	}

	/* don't care about writefds and exceptfds */
	select(sockid+1, &readfds, NULL, NULL, &tv);

	if(sock.ipv4sock >= 0 && FD_ISSET(sock.ipv4sock, &readfds))
		return 1;

	if(sock.ipv6sock >= 0 && FD_ISSET(sock.ipv6sock, &readfds))
		return 1;

	return 0;
}

int time_timestamp()
{
	return time(0);
}

int time_houroftheday()
{
	time_t time_data;
	struct tm *time_info;

	time(&time_data);
	time_info = localtime(&time_data);
	return time_info->tm_hour;
}

int time_season()
{
	time_t time_data;
	struct tm *time_info;

	time(&time_data);
	time_info = localtime(&time_data);

	if((time_info->tm_mon == 11 && time_info->tm_mday == 31) || (time_info->tm_mon == 0 && time_info->tm_mday == 1))
	{
		return SEASON_NEWYEAR;
	}

	switch(time_info->tm_mon)
	{
		case 11:
		case 0:
		case 1:
			return SEASON_WINTER;
		case 2:
		case 3:
		case 4:
			return SEASON_SPRING;
		case 5:
		case 6:
		case 7:
			return SEASON_SUMMER;
		case 8:
		case 9:
		case 10:
			return SEASON_AUTUMN;
	}
	return SEASON_SPRING; // should never happen
}

int time_isxmasday()
{
	time_t time_data;
	struct tm *time_info;

	time(&time_data);
	time_info = localtime(&time_data);
	if(time_info->tm_mon == 11 && time_info->tm_mday >= 24 && time_info->tm_mday <= 26)
		return 1;
	return 0;
}

int time_iseasterday()
{
	time_t time_data_now, time_data;
	struct tm *time_info;
	int Y, a, b, c, d, e, f, g, h, i, k, L, m, month, day, day_offset;

	time(&time_data_now);
	time_info = localtime(&time_data_now);

	// compute Easter day (Sunday) using https://en.wikipedia.org/w/index.php?title=Computus&oldid=890710285#Anonymous_Gregorian_algorithm
	Y = time_info->tm_year + 1900;
	a = Y % 19;
	b = Y / 100;
	c = Y % 100;
	d = b / 4;
	e = b % 4;
	f = (b + 8) / 25;
	g = (b - f + 1) / 3;
	h = (19 * a + b - d - g + 15) % 30;
	i = c / 4;
	k = c % 4;
	L = (32 + 2 * e + 2 * i - h - k) % 7;
	m = (a + 11 * h + 22 * L) / 451;
	month = (h + L - 7 * m + 114) / 31;
	day = ((h + L - 7 * m + 114) % 31) + 1;

	// (now-1d ≤ easter ≤ now+2d) <=> (easter-2d ≤ now ≤ easter+1d) <=> (Good Friday ≤ now ≤ Easter Monday)
	for(day_offset = -1; day_offset <= 2; day_offset++)
	{
		time_data = time_data_now + day_offset*(60*60*24);
		time_info = localtime(&time_data);

		if(time_info->tm_mon == month-1 && time_info->tm_mday == day)
			return 1;
	}
	return 0;
}

void str_append(char *dst, const char *src, int dst_size)
{
	int s;
	int i = 0;
	dbg_assert(dst_size > 0, "dst_size invalid");
	s = str_length(dst);
	while(s < dst_size)
	{
		dst[s] = src[i];
		if(!src[i]) /* check for null termination */
			break;
		s++;
		i++;
	}

	dst[dst_size-1] = 0; /* assure null termination */
}

void str_copy(char *dst, const char *src, int dst_size)
{
	dbg_assert(dst_size > 0, "dst_size invalid");
	dst[0] = '\0';
	strncat(dst, src, dst_size - 1);
}

void str_truncate(char *dst, int dst_size, const char *src, int truncation_len)
{
	int size = dst_size;
	if(truncation_len < size)
	{
		size = truncation_len + 1;
	}
	str_copy(dst, src, size);
}

int str_length(const char *str)
{
	return (int)strlen(str);
}

void str_format(char *buffer, int buffer_size, const char *format, ...)
{
	va_list ap;
	dbg_assert(buffer_size > 0, "buffer_size invalid");
	va_start(ap, format);

#if defined(CONF_FAMILY_WINDOWS) && !defined(__GNUC__)
	_vsprintf_p(buffer, buffer_size, format, ap);
#else
	vsnprintf(buffer, buffer_size, format, ap);
#endif

	va_end(ap);

	buffer[buffer_size-1] = 0; /* assure null termination */
}



/* makes sure that the string only contains the characters between 32 and 127 */
void str_sanitize_strong(char *str_in)
{
	unsigned char *str = (unsigned char *)str_in;
	while(*str)
	{
		*str &= 0x7f;
		if(*str < 32)
			*str = 32;
		str++;
	}
}

/* makes sure that the string only contains the characters between 32 and 255 */
void str_sanitize_cc(char *str_in)
{
	unsigned char *str = (unsigned char *)str_in;
	while(*str)
	{
		if(*str < 32)
			*str = ' ';
		str++;
	}
}

/* check if the string contains '..' (parent directory) paths */
int str_path_unsafe(const char *str)
{
	// State machine. 0 means that we're at the beginning
	// of a new directory/filename, and a positive number represents the number of
	// dots ('.') we found. -1 means we encountered a different character
	// since the last path separator (or the beginning of the string).
	int parse_counter = 0;
	while(*str)
	{
		if(*str == '\\' || *str == '/')
		{
			// A path separator. Check how many dots we found since
			// the last path separator.
			//
			// Two dots => ".." contained in the path. Return an
			// error.
			if(parse_counter == 2)
				return -1;
			else
				parse_counter = 0;
		}
		else if(parse_counter >= 0)
		{
			// If we have not encountered a non-dot character since
			// the last path separator, count the dots.
			if(*str == '.')
				parse_counter++;
			else
				parse_counter = -1;
		}

		++str;
	}
	// If there's a ".." at the end, fail too.
	if(parse_counter == 2)
		return -1;
	return 0;
}

/* makes sure that the string only contains the characters between 32 and 255 + \r\n\t */
void str_sanitize(char *str_in)
{
	unsigned char *str = (unsigned char *)str_in;
	while(*str)
	{
		if(*str < 32 && !(*str == '\r') && !(*str == '\n') && !(*str == '\t'))
			*str = ' ';
		str++;
	}
}

/* removes all forbidden windows/unix characters in filenames*/
char* str_sanitize_filename(char* aName)
{
	char *str = (char *)aName;
	while(*str)
	{
		// replace forbidden characters with a whispace
		if(*str == '/' || *str == '<' || *str == '>' || *str == ':' || *str == '"'
			|| *str == '\\' || *str == '|' || *str == '?' || *str == '*')
 			*str = ' ';
		str++;
	}
	str_clean_whitespaces(aName);
	return aName;
}

/* removes leading and trailing spaces and limits the use of multiple spaces */
void str_clean_whitespaces(char *str_in)
{
	char *read = str_in;
	char *write = str_in;

	/* skip initial whitespace */
	while(*read == ' ')
		read++;

	/* end of read string is detected in the loop */
	while(1)
	{
		/* skip whitespace */
		int found_whitespace = 0;
		for(; *read == ' '; read++)
			found_whitespace = 1;
		/* if not at the end of the string, put a found whitespace here */
		if(*read)
		{
			if(found_whitespace)
				*write++ = ' ';
			*write++ = *read++;
		}
		else
		{
			*write = 0;
			break;
		}
	}
}

/* removes leading and trailing spaces */
void str_clean_whitespaces_simple(char *str_in)
{
	char *read = str_in;
	char *write = str_in;

	/* skip initial whitespace */
	while(*read == ' ')
		read++;

	/* end of read string is detected in the loop */
	while(1)
	{
		/* skip whitespace */
		int found_whitespace = 0;
		for(; *read == ' ' && !found_whitespace; read++)
			found_whitespace = 1;
		/* if not at the end of the string, put a found whitespace here */
		if(*read)
		{
			if(found_whitespace)
				*write++ = ' ';
			*write++ = *read++;
		}
		else
		{
			*write = 0;
			break;
		}
	}
}

char *str_skip_to_whitespace(char *str)
{
	while(*str && (*str != ' ' && *str != '\t' && *str != '\n'))
		str++;
	return str;
}

const char *str_skip_to_whitespace_const(const char *str)
{
	while(*str && (*str != ' ' && *str != '\t' && *str != '\n'))
		str++;
	return str;
}

char *str_skip_whitespaces(char *str)
{
	while(*str && (*str == ' ' || *str == '\t' || *str == '\n' || *str == '\r'))
		str++;
	return str;
}

const char *str_skip_whitespaces_const(const char *str)
{
	while(*str && (*str == ' ' || *str == '\t' || *str == '\n' || *str == '\r'))
		str++;
	return str;
}

/* case */
int str_comp_nocase(const char *a, const char *b)
{
#if defined(CONF_FAMILY_WINDOWS) && !defined(__GNUC__)
	return _stricmp(a,b);
#else
	return strcasecmp(a,b);
#endif
}

int str_comp_nocase_num(const char *a, const char *b, const int num)
{
#if defined(CONF_FAMILY_WINDOWS) && !defined(__GNUC__)
	return _strnicmp(a, b, num);
#else
	return strncasecmp(a, b, num);
#endif
}

int str_comp(const char *a, const char *b)
{
	return strcmp(a, b);
}

int str_comp_num(const char *a, const char *b, const int num)
{
	return strncmp(a, b, num);
}

int str_comp_filenames(const char *a, const char *b)
{
	int result;

	for(; *a && *b; ++a, ++b)
	{
		if(*a >= '0' && *a <= '9' && *b >= '0' && *b <= '9')
		{
			result = 0;
			do
			{
				if(!result)
					result = *a - *b;
				++a;
				++b;
			} while(*a >= '0' && *a <= '9' && *b >= '0' && *b <= '9');

			if(*a >= '0' && *a <= '9')
				return 1;
			else if(*b >= '0' && *b <= '9')
				return -1;
			else if(result || *a == '\0' || *b == '\0')
				return result;
		}

		result = tolower(*a) - tolower(*b);
		if(result)
			return result;
	}
	return *a - *b;
}

const char *str_startswith_nocase(const char *str, const char *prefix)
{
	int prefixl = str_length(prefix);
	if(str_comp_nocase_num(str, prefix, prefixl) == 0)
	{
		return str + prefixl;
	}
	else
	{
		return 0;
	}
}

const char *str_startswith(const char *str, const char *prefix)
{
	int prefixl = str_length(prefix);
	if(str_comp_num(str, prefix, prefixl) == 0)
	{
		return str + prefixl;
	}
	else
	{
		return 0;
	}
}

const char *str_endswith_nocase(const char *str, const char *suffix)
{
	int strl = str_length(str);
	int suffixl = str_length(suffix);
	const char *strsuffix;
	if(strl < suffixl)
	{
		return 0;
	}
	strsuffix = str + strl - suffixl;
	if(str_comp_nocase(strsuffix, suffix) == 0)
	{
		return strsuffix;
	}
	else
	{
		return 0;
	}
}

const char *str_endswith(const char *str, const char *suffix)
{
	int strl = str_length(str);
	int suffixl = str_length(suffix);
	const char *strsuffix;
	if(strl < suffixl)
	{
		return 0;
	}
	strsuffix = str + strl - suffixl;
	if(str_comp(strsuffix, suffix) == 0)
	{
		return strsuffix;
	}
	else
	{
		return 0;
	}
}

const char *str_find_nocase(const char *haystack, const char *needle)
{
	while(*haystack) /* native implementation */
	{
		const char *a = haystack;
		const char *b = needle;
		while(*a && *b && tolower(*a) == tolower(*b))
		{
			a++;
			b++;
		}
		if(!(*b))
			return haystack;
		haystack++;
	}

	return 0;
}


const char *str_find(const char *haystack, const char *needle)
{
	while(*haystack) /* native implementation */
	{
		const char *a = haystack;
		const char *b = needle;
		while(*a && *b && *a == *b)
		{
			a++;
			b++;
		}
		if(!(*b))
			return haystack;
		haystack++;
	}

	return 0;
}

void str_hex(char *dst, int dst_size, const void *data, int data_size)
{
	static const char hex[] = "0123456789ABCDEF";
	int data_index;
	int dst_index;
	for(data_index = 0, dst_index = 0; data_index < data_size && dst_index < dst_size - 3; data_index++)
	{
		dst[data_index * 3] = hex[((const unsigned char *)data)[data_index] >> 4];
		dst[data_index * 3 + 1] = hex[((const unsigned char *)data)[data_index] & 0xf];
		dst[data_index * 3 + 2] = ' ';
		dst_index += 3;
	}
	dst[dst_index] = '\0';
}

int str_is_number(const char *str)
{
	while(*str)
	{
		if(!(*str >= '0' && *str <= '9'))
			return -1;
		str++;
	}
	return 0;
}
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
#endif
void str_timestamp_ex(time_t time_data, char *buffer, int buffer_size, const char *format)
{
	struct tm *time_info;
	dbg_assert(buffer_size > 0, "buffer_size invalid");
	time_info = localtime(&time_data);
	strftime(buffer, buffer_size, format, time_info);
	buffer[buffer_size-1] = 0;	/* assure null termination */
}

void str_timestamp_format(char *buffer, int buffer_size, const char *format)
{
	time_t time_data;
	time(&time_data);
	str_timestamp_ex(time_data, buffer, buffer_size, format);
}

void str_timestamp(char *buffer, int buffer_size)
{
	str_timestamp_format(buffer, buffer_size, FORMAT_NOSPACE);
}
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

int str_span(const char *str, const char *set)
{
	return strcspn(str, set);
}

int mem_comp(const void *a, const void *b, int size)
{
	return memcmp(a,b,size);
}

int mem_has_null(const void *block, unsigned size)
{
	const unsigned char *bytes = block;
	unsigned i;
	for(i = 0; i < size; i++)
	{
		if(bytes[i] == 0)
		{
			return 1;
		}
	}
	return 0;
}

void net_stats(NETSTATS *stats_inout)
{
	*stats_inout = network_stats;
}

int str_isspace(char c) { return c == ' ' || c == '\n' || c == '\t'; }

char str_uppercase(char c)
{
	if(c >= 'a' && c <= 'z')
		return 'A' + (c-'a');
	return c;
}

int str_toint(const char *str) { return atoi(str); }
float str_tofloat(const char *str) { return atof(str); }

int str_utf8_is_whitespace(int code)
{
	// check if unicode is not empty
	if(code > 0x20 && code != 0xA0 && code != 0x034F && (code < 0x2000 || code > 0x200F) && (code < 0x2028 || code > 0x202F) &&
		(code < 0x205F || code > 0x2064) && (code < 0x206A || code > 0x206F) && code != 0x3000  && (code < 0xFE00 || code > 0xFE0F) &&
		code != 0xFEFF && (code < 0xFFF9 || code > 0xFFFC))
	{
		return 0;
	}
	return 1;
}

const char *str_utf8_skip_whitespaces(const char *str)
{
	const char *str_old;
	int code;

	while(*str)
	{
		str_old = str;
		code = str_utf8_decode(&str);

		if(!str_utf8_is_whitespace(code))
		{
			return str_old;
		}
	}

	return str;
}

void str_utf8_trim_whitespaces_right(char *str)
{
	int cursor = str_length(str);
	const char *last = str + cursor;
	while(str_utf8_is_whitespace(str_utf8_decode(&last)))
	{
		str[cursor] = 0;
		cursor = str_utf8_rewind(str, cursor);
		last = str + cursor;
		if(cursor == 0)
		{
			break;
		}
	}
}

static int str_utf8_isstart(char c)
{
	if((c&0xC0) == 0x80) /* 10xxxxxx */
		return 0;
	return 1;
}

int str_utf8_rewind(const char *str, int cursor)
{
	while(cursor)
	{
		cursor--;
		if(str_utf8_isstart(*(str + cursor)))
			break;
	}
	return cursor;
}

int str_utf8_forward(const char *str, int cursor)
{
	const char *buf = str + cursor;
	if(!buf[0])
		return cursor;

	if((*buf&0x80) == 0x0)  /* 0xxxxxxx */
		return cursor+1;
	else if((*buf&0xE0) == 0xC0) /* 110xxxxx */
	{
		if(!buf[1]) return cursor+1;
		return cursor+2;
	}
	else  if((*buf & 0xF0) == 0xE0)	/* 1110xxxx */
	{
		if(!buf[1]) return cursor+1;
		if(!buf[2]) return cursor+2;
		return cursor+3;
	}
	else if((*buf & 0xF8) == 0xF0)	/* 11110xxx */
	{
		if(!buf[1]) return cursor+1;
		if(!buf[2]) return cursor+2;
		if(!buf[3]) return cursor+3;
		return cursor+4;
	}

	/* invalid */
	return cursor+1;
}

int str_utf8_encode(char *ptr, int chr)
{
	/* encode */
	if(chr <= 0x7F)
	{
		ptr[0] = (char)chr;
		return 1;
	}
	else if(chr <= 0x7FF)
	{
		ptr[0] = 0xC0|((chr>>6)&0x1F);
		ptr[1] = 0x80|(chr&0x3F);
		return 2;
	}
	else if(chr <= 0xFFFF)
	{
		ptr[0] = 0xE0|((chr>>12)&0x0F);
		ptr[1] = 0x80|((chr>>6)&0x3F);
		ptr[2] = 0x80|(chr&0x3F);
		return 3;
	}
	else if(chr <= 0x10FFFF)
	{
		ptr[0] = 0xF0|((chr>>18)&0x07);
		ptr[1] = 0x80|((chr>>12)&0x3F);
		ptr[2] = 0x80|((chr>>6)&0x3F);
		ptr[3] = 0x80|(chr&0x3F);
		return 4;
	}

	return 0;
}

int str_utf8_decode(const char **ptr)
{
	const char *buf = *ptr;
	int ch = 0;

	do
	{
		if((*buf&0x80) == 0x0)  /* 0xxxxxxx */
		{
			ch = (unsigned char)*buf;
			buf++;
		}
		else if((*buf&0xE0) == 0xC0) /* 110xxxxx */
		{
			ch  = (*buf++ & 0x3F) << 6; if(!(*buf) || (*buf&0xC0) != 0x80) break;
			ch += (*buf++ & 0x3F);
			if(ch < 0x80 || ch > 0x7FF) ch = -1;
		}
		else  if((*buf & 0xF0) == 0xE0)	/* 1110xxxx */
		{
			ch  = (*buf++ & 0x1F) << 12; if(!(*buf) || (*buf&0xC0) != 0x80) break;
			ch += (*buf++ & 0x3F) <<  6; if(!(*buf) || (*buf&0xC0) != 0x80) break;
			ch += (*buf++ & 0x3F);
			if(ch < 0x800 || ch > 0xFFFF) ch = -1;
		}
		else if((*buf & 0xF8) == 0xF0)	/* 11110xxx */
		{
			ch  = (*buf++ & 0x0F) << 18; if(!(*buf) || (*buf&0xC0) != 0x80) break;
			ch += (*buf++ & 0x3F) << 12; if(!(*buf) || (*buf&0xC0) != 0x80) break;
			ch += (*buf++ & 0x3F) <<  6; if(!(*buf) || (*buf&0xC0) != 0x80) break;
			ch += (*buf++ & 0x3F);
			if(ch < 0x10000 || ch > 0x10FFFF) ch = -1;
		}
		else
		{
			/* invalid */
			buf++;
			break;
		}

		*ptr = buf;
		return ch;
	} while(0);

	/* out of bounds */
	*ptr = buf;
	return -1;

}

int str_utf8_check(const char *str)
{
	while(*str)
	{
		if((*str&0x80) == 0x0)
			str++;
		else if((*str&0xE0) == 0xC0 && (*(str+1)&0xC0) == 0x80)
			str += 2;
		else if((*str&0xF0) == 0xE0 && (*(str+1)&0xC0) == 0x80 && (*(str+2)&0xC0) == 0x80)
			str += 3;
		else if((*str&0xF8) == 0xF0 && (*(str+1)&0xC0) == 0x80 && (*(str+2)&0xC0) == 0x80 && (*(str+3)&0xC0) == 0x80)
			str += 4;
		else
			return 0;
	}
	return 1;
}

void str_utf8_copy_num(char *dst, const char *src, int dst_size, int num)
{
	int new_cursor;
	int cursor = 0;
	dbg_assert(dst_size > 0, "dst_size invalid");

	while(src[cursor] && num > 0)
	{
		new_cursor = str_utf8_forward(src, cursor);
		if(new_cursor >= dst_size)			// reserve 1 byte for the null termination
			break;
		else
			cursor = new_cursor;
		--num;
	}

	str_copy(dst, src, cursor < dst_size ? cursor+1 : dst_size);
}

void str_utf8_stats(const char *str, int max_size, int max_count, int *size, int *count)
{
	*size = 0;
	*count = 0;
	while(*size < max_size && *count < max_count)
	{
		int new_size = str_utf8_forward(str, *size);
		if(new_size == *size || new_size >= max_size)
			break;
		*size = new_size;
		++(*count);
	}
}

unsigned str_quickhash(const char *str)
{
	unsigned hash = 5381;
	for(; *str; str++)
		hash = ((hash << 5) + hash) + (*str); /* hash * 33 + c */
	return hash;
}

struct SECURE_RANDOM_DATA
{
	int initialized;
#if defined(CONF_FAMILY_WINDOWS)
	HCRYPTPROV provider;
#else
	IOHANDLE urandom;
#endif
};

static struct SECURE_RANDOM_DATA secure_random_data = { 0 };

int secure_random_init()
{
	if(secure_random_data.initialized)
	{
		return 0;
	}
#if defined(CONF_FAMILY_WINDOWS)
	if(CryptAcquireContext(&secure_random_data.provider, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
	{
		secure_random_data.initialized = 1;
		return 0;
	}
	else
	{
		return 1;
	}
#else
	secure_random_data.urandom = io_open("/dev/urandom", IOFLAG_READ);
	if(secure_random_data.urandom)
	{
		secure_random_data.initialized = 1;
		return 0;
	}
	else
	{
		return 1;
	}
#endif
}

int secure_random_uninit()
{
	if(!secure_random_data.initialized)
	{
		return 0;
	}
#if defined(CONF_FAMILY_WINDOWS)
	if(CryptReleaseContext(secure_random_data.provider, 0))
	{
		secure_random_data.initialized = 0;
		return 0;
	}
	else
	{
		return 1;
	}
#else
	if(!io_close(secure_random_data.urandom))
	{
		secure_random_data.initialized = 0;
		return 0;
	}
	else
	{
		return 1;
	}
#endif
}

void secure_random_fill(void *bytes, unsigned length)
{
	if(!secure_random_data.initialized)
	{
		dbg_msg("secure", "called secure_random_fill before secure_random_init");
		dbg_break();
	}
#if defined(CONF_FAMILY_WINDOWS)
	if(!CryptGenRandom(secure_random_data.provider, length, bytes))
	{
		dbg_msg("secure", "CryptGenRandom failed, last_error=%lu", GetLastError());
		dbg_break();
	}
#else
	if(length != io_read(secure_random_data.urandom, bytes, length))
	{
		dbg_msg("secure", "io_read returned with a short read");
		dbg_break();
	}
#endif
}

int pid()
{
#if defined(CONF_FAMILY_WINDOWS)
	return _getpid();
#else
	return getpid();
#endif
}

void cmdline_fix(int *argc, const char ***argv)
{
#if defined(CONF_FAMILY_WINDOWS)
	int wide_argc = 0;
	int total_size = 0;
	int remaining_size;
	char **new_argv;
	WCHAR **wide_argv = CommandLineToArgvW(GetCommandLineW(), &wide_argc);
	dbg_assert(wide_argv != NULL, "CommandLineToArgvW failure");

	for(int i = 0; i < wide_argc; i++)
	{
		int size = WideCharToMultiByte(CP_UTF8, 0, wide_argv[i], -1, NULL, 0, NULL, NULL);
		dbg_assert(size != 0, "WideCharToMultiByte failure");
		total_size += size;
	}

	new_argv = (char **)malloc((wide_argc + 1) * sizeof(*new_argv));
	new_argv[0] = (char *)malloc(total_size);
	mem_zero(new_argv[0], total_size);

	remaining_size = total_size;
	for(int i = 0; i < wide_argc; i++)
	{
		int size = WideCharToMultiByte(CP_UTF8, 0, wide_argv[i], -1, new_argv[i], remaining_size, NULL, NULL);
		dbg_assert(size != 0, "WideCharToMultiByte failure");

		remaining_size -= size;
		new_argv[i + 1] = new_argv[i] + size;
	}

	new_argv[wide_argc] = 0;
	*argc = wide_argc;
	*argv = (const char **)new_argv;
#endif
}

void cmdline_free(int argc, const char **argv)
{
#if defined(CONF_FAMILY_WINDOWS)
	free((void *)*argv);
	free((char **)argv);
#endif
}

int bytes_be_to_int(const unsigned char *bytes)
{
	int Result;
	unsigned char *pResult = (unsigned char *)&Result;
	for(unsigned i = 0; i < sizeof(int); i++)
	{
#if defined(CONF_ARCH_ENDIAN_BIG)
		pResult[i] = bytes[i];
#else
		pResult[i] = bytes[sizeof(int) - i - 1];
#endif
	}
	return Result;
}

void int_to_bytes_be(unsigned char *bytes, int value)
{
	const unsigned char *pValue = (const unsigned char *)&value;
	for(unsigned i = 0; i < sizeof(int); i++)
	{
#if defined(CONF_ARCH_ENDIAN_BIG)
		bytes[i] = pValue[i];
#else
		bytes[sizeof(int) - i - 1] = pValue[i];
#endif
	}
}

unsigned bytes_be_to_uint(const unsigned char *bytes)
{
	return ((bytes[0] & 0xffu) << 24u) | ((bytes[1] & 0xffu) << 16u) | ((bytes[2] & 0xffu) << 8u) | (bytes[3] & 0xffu);
}

void uint_to_bytes_be(unsigned char *bytes, unsigned value)
{
	bytes[0] = (value >> 24u) & 0xffu;
	bytes[1] = (value >> 16u) & 0xffu;
	bytes[2] = (value >> 8u) & 0xffu;
	bytes[3] = value & 0xffu;
}


#if defined(__cplusplus)
}
#endif
