/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "strings.h"
#include "time.h"
#include "debug.h"

#if defined(CONF_FAMILY_WINDOWS)
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
#endif

#if defined(__cplusplus)
extern "C" {
#endif

static DBG_LOGGER loggers[16];
static int num_loggers = 0;

void dbg_logger(DBG_LOGGER logger)
{
	loggers[num_loggers++] = logger;
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
	*((volatile unsigned*)0) = 0x0;
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

	len = strlen(str);
	msg = (char *)str + len;

	va_start(args, fmt);
#if defined(CONF_FAMILY_WINDOWS) && !defined(__GNUC__)
	_vsprintf_p(msg, sizeof(str)-len, fmt, args);
#else
	vsnprintf(msg, sizeof(str)-len, fmt, args);
#endif
	va_end(args);

	for(i = 0; i < num_loggers; i++)
		loggers[i](str);
}

#if defined(CONF_FAMILY_WINDOWS)
static void logger_win_console(const char *line)
{
	#define _MAX_LENGTH 1024
	#define _MAX_LENGTH_ERROR (_MAX_LENGTH+32)

	static const int UNICODE_REPLACEMENT_CHAR = 0xfffd;

	static const char *STR_TOO_LONG = "(str too long)";
	static const char *INVALID_UTF8 = "(invalid utf8)";

	wchar_t wline[_MAX_LENGTH_ERROR];
	size_t len = 0;

	const char *read = line;
	const char *error = STR_TOO_LONG;
	while(len < _MAX_LENGTH)
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

			dbg_assert(len < _MAX_LENGTH_ERROR, "str too short for error");
			wline[len++] = character;
			read++;
		}
	}

	// Terminate the line
	dbg_assert(len < _MAX_LENGTH_ERROR, "str too short for \\r");
	wline[len++] = '\r';
	dbg_assert(len < _MAX_LENGTH_ERROR, "str too short for \\n");
	wline[len++] = '\n';

	// Ignore any error that might occur
	WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), wline, len, 0, 0);

	#undef _MAX_LENGTH
	#undef _MAX_LENGTH_ERROR
}
#endif

static void logger_stdout(const char *line)
{
	printf("%s\n", line);
	fflush(stdout);
}

static void logger_debugger(const char *line)
{
#if defined(CONF_FAMILY_WINDOWS)
	OutputDebugString(line);
	OutputDebugString("\n");
#endif
}


static IOHANDLE logfile = 0;
static void logger_file(const char *line)
{
	io_write(logfile, line, strlen(line));
	io_write_newline(logfile);
	io_flush(logfile);
}

void dbg_logger_stdout()
{
#if defined(CONF_FAMILY_WINDOWS)
	if(GetFileType(GetStdHandle(STD_OUTPUT_HANDLE)) == FILE_TYPE_CHAR)
	{
		dbg_logger(logger_win_console);
		return;
	}
#endif
	dbg_logger(logger_stdout);
}

void dbg_logger_debugger() { dbg_logger(logger_debugger); }
void dbg_logger_file(const char *filename)
{
	IOHANDLE handle = io_open(filename, IOFLAG_WRITE);
	if(handle)
		dbg_logger_filehandle(handle);
	else
		dbg_msg("dbg/logger", "failed to open '%s' for logging", filename);
}

void dbg_logger_filehandle(IOHANDLE handle)
{
	logfile = handle;
	if(logfile)
		dbg_logger(logger_file);
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
#endif

#if defined(__cplusplus)
}
#endif
