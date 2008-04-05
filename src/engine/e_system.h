/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#ifndef BASE_SYSTEM_H
#define BASE_SYSTEM_H

#include "e_detect.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Group: Debug */
/**********
	Function: dbg_assert
	
	Breaks into the debugger based on a test.
	
	Parameters:
		test - Result of the test.
		msg - Message that should be printed if the test fails.
	
	Remarks:
		Does nothing in release version of the library.
	
	See Also:
		<dbg_break>
**********/
void dbg_assert(int test, const char *msg);
#define dbg_assert(test,msg) dbg_assert_imp(__FILE__, __LINE__, test,  msg)
void dbg_assert_imp(const char *filename, int line, int test, const char *msg);

/**********
	Function: dbg_break
	
	Breaks into the debugger.
	
	Remarks:
		Does nothing in release version of the library.
	
	See Also:
		<dbg_assert>
**********/
void dbg_break();

/**********
	Function: dbg_msg
	
	Prints a debug message.
	
	Parameters:
		sys - A string that describes what system the message belongs to
		fmt - A printf styled format string.
	
	Remarks:
		Does nothing in relase version of the library.
		
	See Also:
		<dbg_assert>
**********/
void dbg_msg(const char *sys, const char *fmt, ...);

/* Group: Memory */

/**********
	Function: mem_alloc
	
	Allocates memory.
	
	Parameters:
		size - Size of the needed block.
		alignment - Alignment for the block.
	
	Returns:
		Returns a pointer to the newly allocated block. Returns a
		null pointer if the memory couldn't be allocated.
		
	Remarks:
		- Passing 0 to size will allocated the smallest amount possible
		and return a unique pointer.

	See Also:
		<mem_free>
**********/
void *mem_alloc_debug(const char *filename, int line, unsigned size, unsigned alignment);
#define mem_alloc(s,a) mem_alloc_debug(__FILE__, __LINE__, (s), (a))

/**********
	Function: mem_free
	
	Frees a block allocated through <mem_alloc>.
	
	Remarks:
		- In the debug version of the library the function will assert if
		a non-valid block is passed, like a null pointer or a block that
		isn't allocated.
	
	See Also:
		<mem_alloc>
**********/
void mem_free(void *block);

/**********
	Function: mem_copy
		Copies a a memory block.
	
	Parameters:
		dest - Destination.
		source - Source to copy.
		size - Size of the block to copy.
	
	Remarks:
		- This functions DOES NOT handles cases where source and
		destination is overlapping.
	
	See Also:
		<mem_move>
**********/
void mem_copy(void *dest, const void *source, unsigned size);

/**********
	Function: mem_move
		Copies a a memory block.
	
	Parameters:
		dest - Destination.
		source - Source to copy.
		size - Size of the block to copy.
	
	Remarks:
		- This functions handles cases where source and destination is overlapping.
	
	See Also:
		<mem_copy>
**********/
void mem_move(void *dest, const void *source, unsigned size);

/**********
	Function: mem_zero
		Sets a complete memory block to 0.
	
	Parameters:
		block - Pointer to the block to zero out.
		size - Size of the block.
**********/
void mem_zero(void *block, unsigned size);

/* ------- io ------- */
enum {
	IOFLAG_READ = 1,
	IOFLAG_WRITE = 2,
	IOFLAG_RANDOM = 4,

	IOSEEK_START = 0,
	IOSEEK_CUR = 1,
	IOSEEK_END = 2
};

typedef struct IOINTERNAL *IOHANDLE;

/**** Group: File IO ****/

/****
	Function: io_open
		Opens a file.

	Parameters:
		filename - File to open.
		flags - A set of flags. IOFLAG_READ, IOFLAG_WRITE, IOFLAG_RANDOM.

	Returns:
		Returns a handle to the file on success and 0 on failure.

****/
IOHANDLE io_open(const char *filename, int flags);

/****
	Function: io_read
		Reads data into a buffer from a file.

	Parameters:
		io - Handle to the file to read data from.
		buffer - Pointer to the buffer that will recive the data.
		size - Number of bytes to read from the file.
		
	Returns:
		Number of bytes read.

****/
unsigned io_read(IOHANDLE io, void *buffer, unsigned size);

/*****
	Function: io_skip
		Skips data in a file.
	
	Parameters:
		io - Handle to the file.
		size - Number of bytes to skip.
		
	Returns:
		Number of bytes skipped.
****/
unsigned io_skip(IOHANDLE io, unsigned size);

/*****
	Function: io_write
	
	Writes data from a buffer to file.
	
	Parameters:
		io - Handle to the file.
		buffer - Pointer to the data that should be written.
		size - Number of bytes to write.
		
	Returns:
		Number of bytes written.
*****/
unsigned io_write(IOHANDLE io, const void *buffer, unsigned size);

/*****
	Function: io_seek
		Seeks to a specified offset in the file.
	
	Parameters:
		io - Handle to the file.
		offset - Offset from pos to stop.
		origin - Position to start searching from.
		
	Returns:
		Returns 0 on success.
*****/
int io_seek(IOHANDLE io, int offset, int origin);

/*****
	Function: io_tell
		Gets the current position in the file.
	
	Parameters:
		io - Handle to the file.
		
	Returns:
		Returns the current position. -1L if an error occured.
*****/
long int io_tell(IOHANDLE io);

/*****
	Function: io_length
		Gets the total length of the file. Resetting cursor to the beginning
	
	Parameters:
		io - Handle to the file.
		
	Returns:
		Returns the total size. -1L if an error occured.
*****/
long int io_length(IOHANDLE io);

/*****
	Function: io_close
		Closes a file.
	
	Parameters:
		io - Handle to the file.
		
	Returns:
		Returns 0 on success.
*****/
int io_close(IOHANDLE io);

/*****
	Function: io_flush
		Empties all buffers and writes all pending data.
	
	Parameters:
		io - Handle to the file.
		
	Returns:
		Returns 0 on success.
*****/
int io_flush(IOHANDLE io);

/**** Group: Threads ****/

/*****
	Function: thread_sleep
	
	Suspends the current thread for a given period.
	
	Parameters:
		milliseconds - Number of milliseconds to sleep.
*****/
void thread_sleep(int milliseconds);

/**** Group: Locks ****/
typedef void* LOCK;

LOCK lock_create();
void lock_destroy(LOCK lock);

int lock_try(LOCK lock);
void lock_wait(LOCK lock);
void lock_release(LOCK lock);

/**** Group: Timer ****/
#ifdef __GNUC__
/* if compiled with -pedantic-errors it will complain about long
	not being a C90 thing.
*/
__extension__ typedef long long int64;
#else
typedef long long int64;
#endif
/*****
	Function: time_get
	
	Fetches a sample from a high resolution timer.
	
	Returns:
		Current value of the timer.

	Remarks:
		To know how fast the timer is ticking, see <time_freq>.
*****/
int64 time_get();

/*****
	Function: time_freq
	
	Returns the frequency of the high resolution timer.
	
	Returns:
		Returns the frequency of the high resolution timer.
*****/
int64 time_freq();

/**** Group: Network General ipv4 ****/
typedef int NETSOCKET;
enum
{
	NETSOCKET_INVALID = -1
};

typedef struct 
{
	unsigned char ip[4];
	unsigned short port;
} NETADDR4;

/*****
	Function: net_host_lookup
	
	Does a hostname lookup by name and fills out the passed NETADDE4 struct with the recieved details.

	Returns:
		0 on success.
*****/
int net_host_lookup(const char *hostname, unsigned short port, NETADDR4 *addr);

/**** Group: Network UDP4 ****/

/*****
	Function: net_udp4_create
	
	Creates a UDP4 socket and binds it to a port.

	Parameters:
		port - Port to bind the socket to.
	
	Returns:
		On success it returns an handle to the socket. On failure it returns NETSOCKET_INVALID.
*****/
NETSOCKET net_udp4_create(NETADDR4 bindaddr);

/*****
	Function: net_udp4_send
	
	Sends a packet over an UDP4 socket.

	Parameters:
		sock - Socket to use.
		addr - Where to send the packet.
		data - Pointer to the packet data to send.
		size - Size of the packet.
	
	Returns:
		On success it returns the number of bytes sent. Returns -1 on error.
*****/
int net_udp4_send(NETSOCKET sock, const NETADDR4 *addr, const void *data, int size);

/*****
	Function: net_udp4_recv
	
	Recives a packet over an UDP4 socket.

	Parameters:
		sock - Socket to use.
		addr - Pointer to an NETADDR4 that will recive the address.
		data - Pointer to a buffer that will recive the data.
		maxsize - Maximum size to recive.
	
	Returns:
		On success it returns the number of bytes recived. Returns -1 on error.
*****/
int net_udp4_recv(NETSOCKET sock, NETADDR4 *addr, void *data, int maxsize);

/*****
	Function: net_udp4_close
	
	Closes an UDP4 socket.

	Parameters:
		sock - Socket to close.
	
	Returns:
		Returns 0 on success. -1 on error.
*****/
int net_udp4_close(NETSOCKET sock);


/**** Group: Network TCP4 ****/

/*****
	Function: net_tcp4_create
	
	DOCTODO: serp
*****/
NETSOCKET net_tcp4_create(const NETADDR4 *a);

/*****
	Function: net_tcp4_set_non_blocking

	DOCTODO: serp
*****/
int net_tcp4_set_non_blocking(NETSOCKET sock);

/*****
	Function: net_tcp4_set_non_blocking

	DOCTODO: serp
*****/
int net_tcp4_set_blocking(NETSOCKET sock);

/*****
	Function: net_tcp4_listen

	DOCTODO: serp 
*****/
int net_tcp4_listen(NETSOCKET sock, int backlog);

/*****
	Function: net_tcp4_accept
	
	DOCTODO: serp
*****/
int net_tcp4_accept(NETSOCKET sock, NETSOCKET *new_sock, NETADDR4 *a);

/*****
	Function: net_tcp4_connect
	
	DOCTODO: serp
*****/
int net_tcp4_connect(NETSOCKET sock, const NETADDR4 *a);

/*****
	Function: net_tcp4_connect_non_blocking
	
	DOCTODO: serp
*****/
int net_tcp4_connect_non_blocking(NETSOCKET sock, const NETADDR4 *a);

/*****
	Function: net_tcp4_send
	
	DOCTODO: serp
*****/
int net_tcp4_send(NETSOCKET sock, const void *data, int size);

/*****
	Function: net_tcp4_recv
	
	DOCTODO: serp
*****/
int net_tcp4_recv(NETSOCKET sock, void *data, int maxsize);

/*****
	Function: net_tcp4_close
	
	DOCTODO: serp
*****/
int net_tcp4_close(NETSOCKET sock);

/*****
	Function: net_errno

	DOCTODO: serp
*****/
int net_errno();

/*****
	Function: net_would_block

	DOCTODO: serp
*****/
int net_would_block();

/*****
	Function: net_init

	DOCTODO: serp
*****/
int net_init();



/* NOT DOCUMENTED */
typedef void (*fs_listdir_callback)(const char *name, int is_dir, void *user);
int fs_listdir(const char *dir, fs_listdir_callback cb, void *user);
int fs_storage_path(const char *appname, char *path, int max);
int fs_makedir(const char *path);
int net_addr4_cmp(const NETADDR4 *a, const NETADDR4 *b);
int net_socket_read_wait(NETSOCKET sock, int time);

void mem_debug_dump();
int mem_allocated();

void *thread_create(void (*threadfunc)(void *), void *user);
void thread_wait(void *thread);
void thread_destroy(void *thread);
void thread_yield();
unsigned time_timestamp();

void swap_endian(void *data, unsigned elem_size, unsigned num);

/* #define cache_prefetch(addr) __builtin_prefetch(addr) */

/*typedef unsigned char [256] pstr;
void pstr_format(pstr *str, )*/

void str_append(char *dst, const char *src, int dst_size);
void str_copy(char *dst, const char *src, int dst_size);
void str_format(char *buffer, int buffer_size, const char *format, ...);
void str_sanitize_strong(char *str);
void str_sanitize(char *str);
int str_comp_nocase(const char *a, const char *b);
const char *str_find_nocase(const char *haystack, const char *needle);
void str_hex(char *dst, int dst_size, const void *data, int data_size);

typedef void (*DBG_LOGGER)(const char *line);
void dbg_logger(DBG_LOGGER logger);
void dbg_logger_stdout();
void dbg_logger_debugger();
void dbg_logger_file(const char *filename);

IOHANDLE io_stdin();
IOHANDLE io_stdout();
IOHANDLE io_stderr();

typedef struct
{
	int sent_packets;
	int sent_bytes;
	int recv_packets;
	int recv_bytes;
} NETSTATS;

void net_stats(NETSTATS *stats);

#ifdef __cplusplus
}
#endif

#endif
