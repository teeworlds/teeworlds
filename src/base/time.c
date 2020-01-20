/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "time.h"

#if defined(CONF_FAMILY_UNIX)
	#include <sys/time.h>
#elif defined(CONF_FAMILY_WINDOWS)
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
#else
	#error NOT IMPLEMENTED
#endif

#if defined(__cplusplus)
extern "C" {
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

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
#endif
void str_timestamp_ex(time_t time_data, char *buffer, int buffer_size, const char *format)
{
	struct tm *time_info;
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

#if defined(__cplusplus)
}
#endif
