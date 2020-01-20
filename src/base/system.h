/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

/*
	Title: OS Abstraction: Main include and misc. functions
*/

#ifndef BASE_SYSTEM_H
#define BASE_SYSTEM_H

#include "detect.h"
#include "memory.h"
#include "strings.h"
#include "time.h"
#include "fileio.h"
#include "filesystem.h"
#include "network.h"
#include "threading.h"
#include "debug.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
	Function: secure_random_init
		Initializes the secure random module.
		You *MUST* check the return value of this function.

	Returns:
		0 - Initialization succeeded.
		1 - Initialization failed.
*/
int secure_random_init();

/*
	Function: secure_random_fill
		Fills the buffer with the specified amount of random bytes.

	Parameters:
		bytes - Pointer to the start of the buffer.
		length - Length of the buffer.
*/
void secure_random_fill(void *bytes, unsigned length);

/*
	Function: pid
		Gets the process ID of the current process

	Returns:
		The process ID of the current process.
*/
int pid();

#ifdef __cplusplus
}
#endif

#endif
