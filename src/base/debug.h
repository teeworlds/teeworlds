/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

/*
	Title: OS Abstraction: Debug
*/

#ifndef BASE_DEBUG_H
#define BASE_DEBUG_H

#include "detect.h"
#include "fileio.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __GNUC__
#define GNUC_ATTRIBUTE(x) __attribute__(x)
#else
#define GNUC_ATTRIBUTE(x)
#endif

/* Group: Debug */
/*

	Function: dbg_assert
		Breaks into the debugger based on a test.

	Parameters:
		test - Result of the test.
		msg - Message that should be printed if the test fails.

	Remarks:
		Does nothing in release version of the library.

	See Also:
		<dbg_break>
*/
void dbg_assert(int test, const char *msg);
#define dbg_assert(test,msg) dbg_assert_imp(__FILE__, __LINE__, test, msg)
void dbg_assert_imp(const char *filename, int line, int test, const char *msg);


#ifdef __clang_analyzer__
#include <assert.h>
#undef dbg_assert
#define dbg_assert(test,msg) assert(test)
#endif

/*
	Function: dbg_break
		Breaks into the debugger.

	Remarks:
		Does nothing in release version of the library.

	See Also:
		<dbg_assert>
*/
void dbg_break();

/*
	Function: dbg_msg

	Prints a debug message.

	Parameters:
		sys - A string that describes what system the message belongs to
		fmt - A printf styled format string.

	Remarks:
		Does nothing in release version of the library.

	See Also:
		<dbg_assert>
*/
void dbg_msg(const char *sys, const char *fmt, ...)
GNUC_ATTRIBUTE((format(printf, 2, 3)));

typedef void (*DBG_LOGGER)(const char *line);
void dbg_logger(DBG_LOGGER logger);

void dbg_logger_stdout();
void dbg_logger_debugger();
void dbg_logger_file(const char *filename);
void dbg_logger_filehandle(IOHANDLE handle);

#if defined(CONF_FAMILY_WINDOWS)
void dbg_console_init();
void dbg_console_cleanup();
#endif

#ifdef __cplusplus
}
#endif

#endif
