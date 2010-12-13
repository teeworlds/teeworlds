/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

/*#include "detect.h"*/
#include "system.h"
/*#include "e_console.h"*/

#if defined(CONF_FAMILY_UNIX)
	#include <sys/time.h>
	#include <unistd.h>

	/* unix net includes */
	#include <sys/stat.h>
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <sys/ioctl.h>
	#include <errno.h>
	#include <netdb.h>      
	#include <netinet/in.h>
	#include <fcntl.h>
	#include <pthread.h>

	#include <dirent.h>
	
	#if defined(CONF_PLATFORM_MACOSX)
		#include <Carbon/Carbon.h>
	#endif
	
#elif defined(CONF_FAMILY_WINDOWS)
	#define WIN32_LEAN_AND_MEAN 
	#define _WIN32_WINNT 0x0501 /* required for mingw to get getaddrinfo to work */
	#include <windows.h>
	#include <winsock2.h>
	#include <ws2tcpip.h>
	#include <fcntl.h>
	#include <direct.h>
	#include <errno.h>

	#ifndef EWOULDBLOCK
		#define EWOULDBLOCK WSAEWOULDBLOCK
	#endif
#else
	#error NOT IMPLEMENTED
#endif

#if defined(__cplusplus)
extern "C" {
#endif

IOHANDLE io_stdin() { return (IOHANDLE)stdin; }
IOHANDLE io_stdout() { return (IOHANDLE)stdout; }
IOHANDLE io_stderr() { return (IOHANDLE)stderr; }

static DBG_LOGGER loggers[16];
static int num_loggers = 0;

static NETSTATS network_stats = {0};
static MEMSTATS memory_stats = {0};

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
	*((unsigned*)0) = 0x0;
}

void dbg_msg(const char *sys, const char *fmt, ...)
{
	va_list args;
	char str[1024*4];
	char *msg;
	int i, len;

	str_format(str, sizeof(str), "[%08x][%s]: ", (int)time(0), sys);
	len = strlen(str);
	msg = (char *)str + len;

	va_start(args, fmt);
#if defined(CONF_FAMILY_WINDOWS)
	_vsnprintf(msg, sizeof(str)-len, fmt, args);
#else
	vsnprintf(msg, sizeof(str)-len, fmt, args);
#endif
	va_end(args);

	for(i = 0; i < num_loggers; i++)
		loggers[i](str);
}

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
	io_write(logfile, "\n", 1);
	io_flush(logfile);
}

void dbg_logger_stdout() { dbg_logger(logger_stdout); }
void dbg_logger_debugger() { dbg_logger(logger_debugger); }
void dbg_logger_file(const char *filename)
{
	logfile = io_open(filename, IOFLAG_WRITE);
	if(logfile)
		dbg_logger(logger_file);
	else
		dbg_msg("dbg/logger", "failed to open '%s' for logging", filename);

}
/* */

int memory_alloced = 0;

typedef struct MEMHEADER
{
	const char *filename;
	int line;
	int size;
	struct MEMHEADER *prev;
	struct MEMHEADER *next;
} MEMHEADER;

typedef struct MEMTAIL
{
	int guard;
} MEMTAIL;

static struct MEMHEADER *first = 0;
static const int MEM_GUARD_VAL = 0xbaadc0de;

void *mem_alloc_debug(const char *filename, int line, unsigned size, unsigned alignment)
{
	/* TODO: fix alignment */
	/* TODO: add debugging */
	MEMHEADER *header = (struct MEMHEADER *)malloc(size+sizeof(MEMHEADER)+sizeof(MEMTAIL));
	MEMTAIL *tail = (struct MEMTAIL *)(((char*)(header+1))+size);
	header->size = size;
	header->filename = filename;
	header->line = line;

	memory_stats.allocated += header->size;
	memory_stats.total_allocations++;
	memory_stats.active_allocations++;
	
	tail->guard = MEM_GUARD_VAL;

	header->prev = (MEMHEADER *)0;
	header->next = first;
	if(first)
		first->prev = header;
	first = header;
	
	/*dbg_msg("mem", "++ %p", header+1); */
	return header+1;
}

void mem_free(void *p)
{
	if(p)
	{
		MEMHEADER *header = (MEMHEADER *)p - 1;
		MEMTAIL *tail = (MEMTAIL *)(((char*)(header+1))+header->size);
		
		if(tail->guard != MEM_GUARD_VAL)
			dbg_msg("mem", "!! %p", p);
		/* dbg_msg("mem", "-- %p", p); */
		memory_stats.allocated -= header->size;
		memory_stats.active_allocations--;
		
		if(header->prev)
			header->prev->next = header->next;
		else
			first = header->next;
		if(header->next)
			header->next->prev = header->prev;
		
		free(header);
	}
}

void mem_debug_dump()
{
	char buf[1024];
	MEMHEADER *header = first;
	IOHANDLE f = io_open("memory.txt", IOFLAG_WRITE);
	
	while(header)
	{
		str_format(buf, sizeof(buf), "%s(%d): %d\n", header->filename, header->line, header->size);
		io_write(f, buf, strlen(buf));
		header = header->next;
	}
	
	io_close(f);
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

int mem_check_imp()
{
	MEMHEADER *header = first;
	while(header)
	{
		MEMTAIL *tail = (MEMTAIL *)(((char*)(header+1))+header->size);
		if(tail->guard != MEM_GUARD_VAL)
		{
			dbg_msg("mem", "Memory check failed at %s(%d): %d", header->filename, header->line, header->size);
			return 0;
		}
		header = header->next;
	}

	return 1;
}

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
		if(handle == INVALID_HANDLE_VALUE || str_comp(filename+length-str_length(finddata.cFileName), finddata.cFileName))
			return 0x0;
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
	}

	return fseek((FILE*)io, offset, origin);
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

int io_close(IOHANDLE io)
{
	fclose((FILE*)io);
	return 1;
}

int io_flush(IOHANDLE io)
{
	fflush((FILE*)io);
	return 0;
}

void *thread_create(void (*threadfunc)(void *), void *u)
{
#if defined(CONF_FAMILY_UNIX)
	pthread_t id;
	pthread_create(&id, NULL, (void *(*)(void*))threadfunc, u);
	return (void*)id;
#elif defined(CONF_FAMILY_WINDOWS)
	return CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)threadfunc, u, 0, NULL);
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
#if defined(CONF_FAMILY_UNIX)
	void *r = 0;
	pthread_join((pthread_t)thread, &r);
#else
	/*#error not implemented*/
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




#if defined(CONF_FAMILY_UNIX)
typedef pthread_mutex_t LOCKINTERNAL;
#elif defined(CONF_FAMILY_WINDOWS)
typedef CRITICAL_SECTION LOCKINTERNAL;
#else
	#error not implemented on this platform
#endif

LOCK lock_create()
{
	LOCKINTERNAL *lock = (LOCKINTERNAL*)mem_alloc(sizeof(LOCKINTERNAL), 4);

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

int lock_try(LOCK lock)
{
#if defined(CONF_FAMILY_UNIX)
	return pthread_mutex_trylock((LOCKINTERNAL *)lock);
#elif defined(CONF_FAMILY_WINDOWS)
	return TryEnterCriticalSection((LPCRITICAL_SECTION)lock);
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

void lock_release(LOCK lock)
{
#if defined(CONF_FAMILY_UNIX)
	pthread_mutex_unlock((LOCKINTERNAL *)lock);
#elif defined(CONF_FAMILY_WINDOWS)
	LeaveCriticalSection((LPCRITICAL_SECTION)lock);
#else
	#error not implemented on this platform
#endif
}

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
static void netaddr_to_sockaddr(const NETADDR *src, struct sockaddr *dest)
{
	/* TODO: IPv6 support */
	struct sockaddr_in *p = (struct sockaddr_in *)dest;
	mem_zero(p, sizeof(struct sockaddr_in));
	p->sin_family = AF_INET;
	p->sin_port = htons(src->port);
	p->sin_addr.s_addr = htonl(src->ip[0]<<24|src->ip[1]<<16|src->ip[2]<<8|src->ip[3]);
}

static void sockaddr_to_netaddr(const struct sockaddr *src, NETADDR *dst)
{
	/* TODO: IPv6 support */
	unsigned int ip = htonl(((struct sockaddr_in*)src)->sin_addr.s_addr);
	mem_zero(dst, sizeof(NETADDR));
	dst->type = NETTYPE_IPV4;
	dst->port = htons(((struct sockaddr_in*)src)->sin_port);
	dst->ip[0] = (unsigned char)((ip>>24)&0xFF);
	dst->ip[1] = (unsigned char)((ip>>16)&0xFF);
	dst->ip[2] = (unsigned char)((ip>>8)&0xFF);
	dst->ip[3] = (unsigned char)(ip&0xFF);
}

int net_addr_comp(const NETADDR *a, const NETADDR *b)
{
	return mem_comp(a, b, sizeof(NETADDR));
}

void net_addr_str(const NETADDR *addr, char *string, int max_length)
{
	if(addr->type == NETTYPE_IPV4)
		str_format(string, max_length, "%d.%d.%d.%d:%d", addr->ip[0], addr->ip[1], addr->ip[2], addr->ip[3], addr->port);
	else if(addr->type == NETTYPE_IPV6)
	{
		str_format(string, max_length, "[%x:%x:%x:%x:%x:%x:%x:%x]:%d",
			(addr->ip[0]<<8)|addr->ip[1], (addr->ip[2]<<8)|addr->ip[3], (addr->ip[4]<<8)|addr->ip[5], (addr->ip[6]<<8)|addr->ip[7],
			(addr->ip[8]<<8)|addr->ip[9], (addr->ip[10]<<8)|addr->ip[11], (addr->ip[12]<<8)|addr->ip[13], (addr->ip[14]<<8)|addr->ip[15],
			addr->port);
	}
	else
		str_format(string, max_length, "unknown type %d", addr->type);
}

int net_host_lookup(const char *hostname, NETADDR *addr, int types)
{
	/* TODO: IPv6 support */
	struct addrinfo hints;
	struct addrinfo *result;
	int e;
	
	mem_zero(&hints, sizeof(hints));
	hints.ai_family = AF_INET;

	e = getaddrinfo(hostname, NULL, &hints, &result);
	if(e != 0 || !result)
		return -1;

	sockaddr_to_netaddr(result->ai_addr, addr);
	freeaddrinfo(result);
	addr->port = 0;
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
		/* TODO: ipv6 */
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


NETSOCKET net_udp_create(NETADDR bindaddr)
{
	/* TODO: IPv6 support */
	struct sockaddr addr;
	unsigned long mode = 1;
	int broadcast = 1;

	/* create socket */
	int sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(sock < 0)
		return NETSOCKET_INVALID;
	
	/* bind, we should check for error */
	netaddr_to_sockaddr(&bindaddr, &addr);
	if(bind(sock, &addr, sizeof(addr)) != 0)
	{
		net_udp_close(sock);
		return NETSOCKET_INVALID;
	}
	
	/* set non-blocking */
#if defined(CONF_FAMILY_WINDOWS)
	ioctlsocket(sock, FIONBIO, &mode);
#else
	ioctl(sock, FIONBIO, &mode);
#endif

	/* set boardcast */
	setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (const char*)&broadcast, sizeof(broadcast));
	
	/* return */
	return sock;
}

int net_udp_send(NETSOCKET sock, const NETADDR *addr, const void *data, int size)
{
	struct sockaddr sa;
	int d;
	mem_zero(&sa, sizeof(sa));
	netaddr_to_sockaddr(addr, &sa);
	d = sendto((int)sock, (const char*)data, size, 0, &sa, sizeof(sa));
	/*if(d < 0)
	{
		char addrstr[256];
		net_addr_str(addr, addrstr, sizeof(addrstr));
		
		dbg_msg("net", "sendto error %d %x", d, d);
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
	struct sockaddr from;
	int bytes;
	socklen_t fromlen = sizeof(struct sockaddr);
	bytes = recvfrom(sock, (char*)data, maxsize, 0, &from, &fromlen);
	if(bytes > 0)
	{
		sockaddr_to_netaddr(&from, addr);
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
#if defined(CONF_FAMILY_WINDOWS)
	closesocket(sock);
#else
	close((int)sock);
#endif
	return 0;
}

NETSOCKET net_tcp_create(const NETADDR *a)
{
	/* TODO: IPv6 support */
    struct sockaddr addr;

    /* create socket */
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0)
        return NETSOCKET_INVALID;

    /* bind, we should check for error */
    netaddr_to_sockaddr(a, &addr);
    bind(sock, &addr, sizeof(addr));

    /* return */
    return sock;
}

int net_tcp_set_non_blocking(NETSOCKET sock)
{
	unsigned long mode = 1;
#if defined(CONF_FAMILY_WINDOWS)
	return ioctlsocket(sock, FIONBIO, &mode);
#else
	return ioctl(sock, FIONBIO, &mode);
#endif
}

int net_tcp_set_blocking(NETSOCKET sock)
{
	unsigned long mode = 0;
#if defined(CONF_FAMILY_WINDOWS)
	return ioctlsocket(sock, FIONBIO, &mode);
#else
	return ioctl(sock, FIONBIO, &mode);
#endif
}

int net_tcp_listen(NETSOCKET sock, int backlog)
{
	return listen(sock, backlog);
}

int net_tcp_accept(NETSOCKET sock, NETSOCKET *new_sock, NETADDR *a)
{
	int s;
	socklen_t sockaddr_len;
	struct sockaddr addr;

	sockaddr_len = sizeof(addr);

	s = accept(sock, &addr, &sockaddr_len);

	if (s != -1)
	{
		sockaddr_to_netaddr(&addr, a);
		*new_sock = s;
	}
	return s;
}

int net_tcp_connect(NETSOCKET sock, const NETADDR *a)
{
  struct sockaddr addr;

  netaddr_to_sockaddr(a, &addr);
  return connect(sock, &addr, sizeof(addr)); 
}

int net_tcp_connect_non_blocking(NETSOCKET sock, const NETADDR *a)
{
	struct sockaddr addr;
	int res;

	netaddr_to_sockaddr(a, &addr);
	net_tcp_set_non_blocking(sock);
  	res = connect(sock, &addr, sizeof(addr));
	net_tcp_set_blocking(sock);

	return res;
}

int net_tcp_send(NETSOCKET sock, const void *data, int size)
{
  int d;
  d = send((int)sock, (const char*)data, size, 0);
  return d;
}

int net_tcp_recv(NETSOCKET sock, void *data, int maxsize)
{
  int bytes;
  bytes = recv((int)sock, (char*)data, maxsize, 0);
  return bytes;
}

int net_tcp_close(NETSOCKET sock)
{
#if defined(CONF_FAMILY_WINDOWS)
	closesocket(sock);
#else
	close((int)sock);
#endif
	return 0;
}

int net_errno()
{
	return errno;
}

int net_would_block()
{
	return net_errno() == EWOULDBLOCK;
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

int fs_listdir(const char *dir, FS_LISTDIR_CALLBACK cb, int type, void *user)
{
#if defined(CONF_FAMILY_WINDOWS)
	WIN32_FIND_DATA finddata;
	HANDLE handle;
	char buffer[1024*2];
	int length;
	str_format(buffer, sizeof(buffer), "%s/*", dir);

	handle = FindFirstFileA(buffer, &finddata);

	if (handle == INVALID_HANDLE_VALUE)
		return 0;

	str_format(buffer, sizeof(buffer), "%s/", dir);
	length = str_length(buffer);

	/* add all the entries */
	do
	{
		str_copy(buffer+length, finddata.cFileName, (int)sizeof(buffer)-length);
		cb(finddata.cFileName, fs_is_dir(buffer), type, user);
	}
	while (FindNextFileA(handle, &finddata));

	FindClose(handle);
	return 0;
#else
	struct dirent *entry;
	char buffer[1024*2];
	int length;
	DIR *d = opendir(dir);

	if(!d)
		return 0;
		
	str_format(buffer, sizeof(buffer), "%s/", dir);
	length = str_length(buffer);

	while((entry = readdir(d)) != NULL)
	{
		str_copy(buffer+length, entry->d_name, (int)sizeof(buffer)-length);
		cb(entry->d_name, fs_is_dir(buffer), type, user);
	}

	/* close the directory and return */
	closedir(d);
	return 0;
#endif
}

int fs_storage_path(const char *appname, char *path, int max)
{
#if defined(CONF_FAMILY_WINDOWS)
	HRESULT r;
	char *home = getenv("APPDATA");
	if(!home)
		return -1;
	_snprintf(path, max, "%s/%s", home, appname);
	return 0;
#else
	char *home = getenv("HOME");
#if !defined(CONF_PLATFORM_MACOSX)
	int i;
#endif
	if(!home)
		return -1;

#if defined(CONF_PLATFORM_MACOSX)
	snprintf(path, max, "%s/Library/Application Support/%s", home, appname);
#else
	snprintf(path, max, "%s/.%s", home, appname);
	for(i = strlen(home)+2; path[i]; i++)
		path[i] = tolower(path[i]);
#endif
	
	return 0;
#endif
}

int fs_makedir(const char *path)
{
#if defined(CONF_FAMILY_WINDOWS)
	if(_mkdir(path) == 0)
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

int fs_is_dir(const char *path)
{
#if defined(CONF_FAMILY_WINDOWS)
	/* TODO: do this smarter */
	WIN32_FIND_DATA finddata;
	HANDLE handle;
	char buffer[1024*2];
	str_format(buffer, sizeof(buffer), "%s/*", path);

	if ((handle = FindFirstFileA(buffer, &finddata)) == INVALID_HANDLE_VALUE)
		return 0;

	FindClose(handle);
	return 1;
#else
	struct stat sb;
	if (stat(path, &sb) == -1)
		return 0;
	
	if (S_ISDIR(sb.st_mode))
		return 1;
	else
		return 0;
#endif
}

int fs_chdir(const char *path)
{
	if(fs_is_dir(path))
	{
		if(chdir(path))
			return 1;
		else
			return 0;
	}
	else
		return 1;
}

char *fs_getcwd(char *buffer, int buffer_size)
{
	if(buffer == 0)
		return 0;
#if defined(CONF_FAMILY_WINDOWS)
	return _getcwd(buffer, buffer_size);
#else
	return getcwd(buffer, buffer_size);
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
	if(remove(filename) != 0)
		return 1;
	return 0;
}

int fs_rename(const char *oldname, const char *newname)
{
	if(rename(oldname, newname) != 0)
		return 1;
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

    tv.tv_sec = 0;
    tv.tv_usec = 1000*time;

    FD_ZERO(&readfds);
    FD_SET(sock, &readfds);

    /* don't care about writefds and exceptfds */
    select(sock+1, &readfds, NULL, NULL, &tv);
    if(FD_ISSET(sock, &readfds))
    	return 1;
    return 0;
}

unsigned time_timestamp()
{
	return time(0);
}

void str_append(char *dst, const char *src, int dst_size)
{
	int s = strlen(dst);
	int i = 0;
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
	strncpy(dst, src, dst_size);
	dst[dst_size-1] = 0; /* assure null termination */
}

int str_length(const char *str)
{
	return (int)strlen(str);
}

void str_format(char *buffer, int buffer_size, const char *format, ...)
{
#if defined(CONF_FAMILY_WINDOWS)
	va_list ap;
	va_start(ap, format);
	_vsnprintf(buffer, buffer_size, format, ap);
    va_end(ap);
#else
	va_list ap;
	va_start(ap, format);
	vsnprintf(buffer, buffer_size, format, ap);
    va_end(ap);
#endif

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

char *str_skip_to_whitespace(char *str)
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

/* case */
int str_comp_nocase(const char *a, const char *b)
{
#if defined(CONF_FAMILY_WINDOWS)
	return _stricmp(a,b);
#else
	return strcasecmp(a,b);
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
				++a; ++b;
			}
			while(*a >= '0' && *a <= '9' && *b >= '0' && *b <= '9');

			if(*a >= '0' && *a <= '9')
				return 1;
			else if(*b >= '0' && *b <= '9')
				return -1;
			else if(result)
				return result;
		}

		if(*a != *b)
			break;
	}
	return *a - *b;
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
	int b;

	for(b = 0; b < data_size && b < dst_size/4-4; b++)
	{
		dst[b*3] = hex[((const unsigned char *)data)[b]>>4];
		dst[b*3+1] = hex[((const unsigned char *)data)[b]&0xf];
		dst[b*3+2] = ' ';
		dst[b*3+3] = 0;
	}
}

void str_timestamp(char *buffer, int buffer_size)
{
	time_t time_data;
	struct tm *time_info;
	
	time(&time_data);
	time_info = localtime(&time_data);
	strftime(buffer, buffer_size, "%Y-%m-%d_%H-%M-%S", time_info);
	buffer[buffer_size-1] = 0;	/* assure null termination */
}

int mem_comp(const void *a, const void *b, int size)
{
	return memcmp(a,b,size);
}

const MEMSTATS *mem_stats()
{
	return &memory_stats;
}

void net_stats(NETSTATS *stats_inout)
{
	*stats_inout = network_stats;
}

void gui_messagebox(const char *title, const char *message)
{
#if defined(CONF_PLATFORM_MACOSX)
	DialogRef theItem;
	DialogItemIndex itemIndex;

	/* FIXME: really needed? can we rely on glfw? */
	/* HACK - get events without a bundle */
	ProcessSerialNumber psn;
	GetCurrentProcess(&psn);
	TransformProcessType(&psn,kProcessTransformToForegroundApplication);
	SetFrontProcess(&psn);
	/* END HACK */

	CreateStandardAlert(kAlertStopAlert,
			CFStringCreateWithCString(NULL, title, kCFStringEncodingASCII),
			CFStringCreateWithCString(NULL, message, kCFStringEncodingASCII),
			NULL,
			&theItem);

	RunStandardAlert(theItem, NULL, &itemIndex);
#elif defined(CONF_FAMILY_UNIX)
	static char cmd[1024];
	/* use xmessage which is available on nearly every X11 system */
	snprintf(cmd, sizeof(cmd), "xmessage -center -title '%s' '%s'",
		title,
		message);

	(void)system(cmd);
#elif defined(CONF_FAMILY_WINDOWS)
	MessageBox(NULL,
		message,
		title,
		MB_ICONEXCLAMATION | MB_OK);
#else
	/* this is not critical */
	#warning not implemented
#endif
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



static int str_utf8_isstart(char c)
{
	if((c&0xC0) == 0x80)  /* 10xxxxxx */
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
			ch = *buf;
			buf++;
		}
		else if((*buf&0xE0) == 0xC0) /* 110xxxxx */
		{
			ch  = (*buf++ & 0x3F) << 6; if(!(*buf)) break;
			ch += (*buf++ & 0x3F);
			if(ch == 0) ch = -1;
		}
		else  if((*buf & 0xF0) == 0xE0)	/* 1110xxxx */
		{
			ch  = (*buf++ & 0x1F) << 12; if(!(*buf)) break;
			ch += (*buf++ & 0x3F) <<  6; if(!(*buf)) break;
			ch += (*buf++ & 0x3F);
			if(ch == 0) ch = -1;
		}
		else if((*buf & 0xF8) == 0xF0)	/* 11110xxx */
		{
			ch  = (*buf++ & 0x0F) << 18; if(!(*buf)) break;
			ch += (*buf++ & 0x3F) << 12; if(!(*buf)) break;
			ch += (*buf++ & 0x3F) <<  6; if(!(*buf)) break;
			ch += (*buf++ & 0x3F);
			if(ch == 0) ch = -1;
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


unsigned str_quickhash(const char *str)
{
	unsigned hash = 5381;
	for(; *str; str++)
		hash = ((hash << 5) + hash) + (*str); /* hash * 33 + c */
	return hash;
}


#if defined(__cplusplus)
}
#endif
