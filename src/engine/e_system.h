/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */

/*
	Title: OS Abstraction
*/

#ifndef BASE_SYSTEM_H
#define BASE_SYSTEM_H

#include "e_detect.h"

#ifdef __cplusplus
extern "C" {
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
#define dbg_assert(test,msg) dbg_assert_imp(__FILE__, __LINE__, test,  msg)
void dbg_assert_imp(const char *filename, int line, int test, const char *msg);

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
		Does nothing in relase version of the library.
		
	See Also:
		<dbg_assert>
*/
void dbg_msg(const char *sys, const char *fmt, ...);

/* Group: Memory */

/*
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
*/
void *mem_alloc_debug(const char *filename, int line, unsigned size, unsigned alignment);
#define mem_alloc(s,a) mem_alloc_debug(__FILE__, __LINE__, (s), (a))

/*
	Function: mem_free
		Frees a block allocated through <mem_alloc>.
	
	Remarks:
		- In the debug version of the library the function will assert if
		a non-valid block is passed, like a null pointer or a block that
		isn't allocated.
	
	See Also:
		<mem_alloc>
*/
void mem_free(void *block);

/*
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
*/
void mem_copy(void *dest, const void *source, unsigned size);

/*
	Function: mem_move
		Copies a a memory block
	
	Parameters:
		dest - Destination
		source - Source to copy
		size - Size of the block to copy
	
	Remarks:
		- This functions handles cases where source and destination
		is overlapping
	
	See Also:
		<mem_copy>
*/
void mem_move(void *dest, const void *source, unsigned size);

/*
	Function: mem_zero
		Sets a complete memory block to 0
	
	Parameters:
		block - Pointer to the block to zero out
		size - Size of the block
*/
void mem_zero(void *block, unsigned size);

/*
	Function: mem_comp
		Compares two blocks of memory
	
	Parameters:
		a - First block of data
		b - Second block of data
		size - Size of the data to compare
		
	Returns:
		<0 - Block a is lesser then block b
		0 - Block a is equal to block b
		>0 - Block a is greater then block b
*/
int mem_comp(const void *a, const void *b, int size);

/* Group: File IO */
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
	Function: io_skip
		Skips data in a file.
	
	Parameters:
		io - Handle to the file.
		size - Number of bytes to skip.
		
	Returns:
		Number of bytes skipped.
*/
unsigned io_skip(IOHANDLE io, unsigned size);

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


/* Group: Threads */

/*
	Function: thread_sleep
		Suspends the current thread for a given period.
	
	Parameters:
		milliseconds - Number of milliseconds to sleep.
*/
void thread_sleep(int milliseconds);

/*
	Function: thread_create
		Creates a new thread.
	
	Parameters:
		threadfunc - Entry point for the new thread.
		user - Pointer to pass to the thread.
		
*/
void *thread_create(void (*threadfunc)(void *), void *user);

/*
	Function: thread_wait
		Waits for a thread to be done or destroyed.
	
	Parameters:
		thread - Thread to wait for.
*/
void thread_wait(void *thread);

/*
	Function: thread_destoy
		Destroys a thread.
	
	Parameters:
		thread - Thread to destroy.
*/
void thread_destroy(void *thread);

/*
	Function: thread_yeild
		Yeild the current threads execution slice.
*/
void thread_yield();


/* Group: Locks */
typedef void* LOCK;

LOCK lock_create();
void lock_destroy(LOCK lock);

int lock_try(LOCK lock);
void lock_wait(LOCK lock);
void lock_release(LOCK lock);

/* Group: Timer */
#ifdef __GNUC__
/* if compiled with -pedantic-errors it will complain about long
	not being a C90 thing.
*/
__extension__ typedef long long int64;
#else
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
		Retrives the current time as a UNIX timestamp
	
	Returns:
		The time as a UNIX timestamp
*/
unsigned time_timestamp();

/* Group: Network General ipv4 */
typedef int NETSOCKET;
enum
{
	NETSOCKET_INVALID = -1,
	
	NETTYPE_INVALID = 0,
	NETTYPE_IPV4 = 1,
	NETTYPE_IPV6 = 2,
	NETTYPE_ALL = ~0
};

/*
typedef struct
{
	unsigned char ip[4];
	unsigned short port;
} NETADDR4;*/

typedef struct
{
	unsigned int type;
	unsigned char ip[16];
	unsigned short port;
} NETADDR;

/*
	Function: net_host_lookup
		Does a hostname lookup by name and fills out the passed NETADDR struct with the recieved details.

	Returns:
		0 on success.
*/
int net_host_lookup(const char *hostname, NETADDR *addr, int types);

/* Group: Network UDP */

/*
	Function: net_udp_create
		Creates a UDP socket and binds it to a port.

	Parameters:
		port - Port to bind the socket to.
	
	Returns:
		On success it returns an handle to the socket. On failure it returns NETSOCKET_INVALID.
*/
NETSOCKET net_udp_create(NETADDR bindaddr);

/*
	Function: net_udp_send
		Sends a packet over an UDP socket.

	Parameters:
		sock - Socket to use.
		addr - Where to send the packet.
		data - Pointer to the packet data to send.
		size - Size of the packet.
	
	Returns:
		On success it returns the number of bytes sent. Returns -1 on error.
*/
int net_udp_send(NETSOCKET sock, const NETADDR *addr, const void *data, int size);

/*
	Function: net_udp_recv
		Recives a packet over an UDP socket.

	Parameters:
		sock - Socket to use.
		addr - Pointer to an NETADDR that will recive the address.
		data - Pointer to a buffer that will recive the data.
		maxsize - Maximum size to recive.
	
	Returns:
		On success it returns the number of bytes recived. Returns -1 on error.
*/
int net_udp_recv(NETSOCKET sock, NETADDR *addr, void *data, int maxsize);

/*
	Function: net_udp_close
		Closes an UDP socket.

	Parameters:
		sock - Socket to close.
	
	Returns:
		Returns 0 on success. -1 on error.
*/
int net_udp_close(NETSOCKET sock);


/* Group: Network TCP */

/*
	Function: net_tcp_create
	
	DOCTODO: serp
*/
NETSOCKET net_tcp_create(const NETADDR *a);

/*
	Function: net_tcp_set_non_blocking

	DOCTODO: serp
*/
int net_tcp_set_non_blocking(NETSOCKET sock);

/*
	Function: net_tcp_set_non_blocking

	DOCTODO: serp
*/
int net_tcp_set_blocking(NETSOCKET sock);

/*
	Function: net_tcp_listen

	DOCTODO: serp 
*/
int net_tcp_listen(NETSOCKET sock, int backlog);

/*
	Function: net_tcp_accept
	
	DOCTODO: serp
*/
int net_tcp_accept(NETSOCKET sock, NETSOCKET *new_sock, NETADDR *a);

/*
	Function: net_tcp_connect
	
	DOCTODO: serp
*/
int net_tcp_connect(NETSOCKET sock, const NETADDR *a);

/*
	Function: net_tcp_connect_non_blocking
	
	DOCTODO: serp
*/
int net_tcp_connect_non_blocking(NETSOCKET sock, const NETADDR *a);

/*
	Function: net_tcp_send
	
	DOCTODO: serp
*/
int net_tcp_send(NETSOCKET sock, const void *data, int size);

/*
	Function: net_tcp_recv
	
	DOCTODO: serp
*/
int net_tcp_recv(NETSOCKET sock, void *data, int maxsize);

/*
	Function: net_tcp_close
	
	DOCTODO: serp
*/
int net_tcp_close(NETSOCKET sock);

/*
	Function: net_errno

	DOCTODO: serp
*/
int net_errno();

/*
	Function: net_would_block

	DOCTODO: serp
*/
int net_would_block();

/*
	Function: net_init

	DOCTODO: serp
*/
int net_init();

/* Group: Strings */

/*
	Function: str_append
		Appends a string to another.
	
	Parameters:
		dst - Pointer to a buffer that contains a string.
		src - String to append.
		dst_size - Size of the buffer of the dst string.
		
	Remarks:
		- The strings are treated as zero-termineted strings.
		- Garantees that dst string will contain zero-termination.
*/
void str_append(char *dst, const char *src, int dst_size);

/*
	Function: str_copy
		Copies a string to another.
		
	Parameters:
		dst - Pointer to a buffer that shall recive the string.
		src - String to be copied.
		dst_size - Size of the buffer dst.

	Remarks:
		- The strings are treated as zero-termineted strings.
		- Garantees that dst string will contain zero-termination.
*/
void str_copy(char *dst, const char *src, int dst_size);

/*
	Function: str_format
		Performs printf formating into a buffer.
		
	Parameters:
		buffer - Pointer to the buffer to recive the formated string.
		buffer_size - Size of the buffer.
		format - printf formating string.
		... - Parameters for the formating.

	Remarks:
		- See the C manual for syntax for the printf formating string.
		- The strings are treated as zero-termineted strings.
		- Garantees that dst string will contain zero-termination.
*/
void str_format(char *buffer, int buffer_size, const char *format, ...);

/*
	Function: str_sanitize_strong
		Replaces all characters below 32 and above 127 with whitespace.
	
	Parameters:
		str - String to sanitize.

	Remarks:
		- The strings are treated as zero-termineted strings.
*/
void str_sanitize_strong(char *str);

/*
	Function: str_sanitize
		Replaces all characters below 32 and above 127 with whitespace with
		exception to \r, \n and \r.
	
	Parameters:
		str - String to sanitize.

	Remarks:
		- The strings are treated as zero-termineted strings.
*/
void str_sanitize(char *str);

/*
	Function: str_comp_nocase
		Compares to strings case insensitive.
	
	Parameters:
		a - String to compare.
		b - String to compare.
	
	Returns:	
		<0 - String g a is lesser then string b
		0 - String a is equal to string b
		>0 - String a is greater then string b

	Remarks:
		- Only garanted to work with a-z/A-Z.
		- The strings are treated as zero-termineted strings.
*/
int str_comp_nocase(const char *a, const char *b);

/*
	Function: str_find_nocase
		Finds a string inside another string case insensitive.

	Parameters:
		haystack - String to search in
		needle - String to search for
		
	Returns:
		A pointer into haystack where the needle was found.
		Returns NULL of needle could not be found.

	Remarks:
		- Only garanted to work with a-z/A-Z.
		- The strings are treated as zero-termineted strings.
*/
const char *str_find_nocase(const char *haystack, const char *needle);


/*
	Function: str_hex
		Takes a datablock and generates a hexstring of it.

	Parameters:
		dst - Buffer to fill with hex data
		dst_size - size of the buffer
		data - Data to turn into hex
		data - Size of the data

	Remarks:
		- The desination buffer will be zero-terminated
*/
void str_hex(char *dst, int dst_size, const void *data, int data_size);


/*
	Function: fs_listdir
		Lists the files in a directory
		
	Parameters:
		dir - Directory to list
		cb - Callback function to call for each entry
		user - Pointer to give to the callback
	
	Returns:
		DOCTODO
*/
typedef void (*fs_listdir_callback)(const char *name, int is_dir, void *user);
int fs_listdir(const char *dir, fs_listdir_callback cb, void *user);

/*
	Function: fs_makedir
		Creates a directory
	
	Parameters:
		path - Directory to create
	
	Returns:
		DOCTODO
	
	Remarks:
		Does not create several directories if needed. "a/b/c" will result
		in a failure if b or a does not exist.
*/
int fs_makedir(const char *path);

/*
	Function: fs_storage_path
		Fetches per user configuration directory.
	
	Returns:
		DOCTODO
	
	Remarks:
		- Returns ~/.appname on UNIX based systems
		- Returns %APPDATA%/Appname on Windows based systems
*/
int fs_storage_path(const char *appname, char *path, int max);


/*
	Group: Undocumented
*/

int net_addr_comp(const NETADDR *a, const NETADDR *b);
int net_addr_str(const NETADDR *addr, char *string, int max_length);
int net_socket_read_wait(NETSOCKET sock, int time);

void mem_debug_dump();
int mem_allocated();

void swap_endian(void *data, unsigned elem_size, unsigned num);

typedef void (*DBG_LOGGER)(const char *line);
void dbg_logger(DBG_LOGGER logger);
void dbg_logger_stdout();
void dbg_logger_debugger();
void dbg_logger_file(const char *filename);

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
