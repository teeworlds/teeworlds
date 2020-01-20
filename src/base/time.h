/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

/*
	Title: OS Abstraction - Time and Date
*/

#ifndef BASE_TIME_H
#define BASE_TIME_H

#include "detect.h"
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __GNUC__
#define GNUC_ATTRIBUTE(x) __attribute__(x)
/* if compiled with -pedantic-errors it will complain about long not being a C90 thing. */
__extension__ typedef long long int64;
#else
#define GNUC_ATTRIBUTE(x)
typedef long long int64;
#endif

/*
	Function: time_get
		Fetches a sample from a high resolution timer.

	Returns:
		Current value of the timer.

	Remarks:
		To know how fast the timer is ticking, see <time_freq>.
*/
int64 time_get();

/*
	Function: time_freq
		Returns the frequency of the high resolution timer.

	Returns:
		Returns the frequency of the high resolution timer.
*/
int64 time_freq();

/*
	Function: time_timestamp
		Retrieves the current time as a UNIX timestamp

	Returns:
		The time as a UNIX timestamp
*/
int time_timestamp();

/*
	Function: time_houroftheday
		Retrieves the hours since midnight (0..23)

	Returns:
		The current hour of the day
*/
int time_houroftheday();

/*
	Function: time_isxmasday
		Checks if it's xmas

	Returns:
		1 - if it's a xmas day
		0 - if not
*/
int time_isxmasday();

/*
	Function: time_iseasterday
		Checks if today is in between Good Friday and Easter Monday (Gregorian calendar)

	Returns:
		1 - if it's egg time
		0 - if not
*/
int time_iseasterday();

/*
	Function: str_timestamp
		Copies a time stamp in the format year-month-day_hour-minute-second to the string.

	Parameters:
		buffer - Pointer to a buffer that shall receive the time stamp string.
		buffer_size - Size of the buffer.

	Remarks:
		- Guarantees that buffer string will contain zero-termination.
*/
void str_timestamp(char *buffer, int buffer_size);
void str_timestamp_format(char *buffer, int buffer_size, const char *format)
GNUC_ATTRIBUTE((format(strftime, 3, 0)));
void str_timestamp_ex(time_t time, char *buffer, int buffer_size, const char *format)
GNUC_ATTRIBUTE((format(strftime, 4, 0)));

#define FORMAT_TIME "%H:%M:%S"
#define FORMAT_SPACE "%Y-%m-%d %H:%M:%S"
#define FORMAT_NOSPACE "%Y-%m-%d_%H-%M-%S"

#ifdef __cplusplus
}
#endif

#endif
