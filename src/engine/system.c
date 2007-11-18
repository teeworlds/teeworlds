#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include "detect.h"
#include "system.h"

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
	#include <unistd.h>
#elif defined(CONF_FAMILY_WINDOWS)
	#define WIN32_LEAN_AND_MEAN 
	#define _WIN32_WINNT 0x0400
	#include <windows.h>
	#include <winsock2.h>
	#include <ws2tcpip.h>
	#include <shlobj.h> /* for SHGetFolderPathAndSubDir */
	#include <fcntl.h>
	#include <direct.h>
	#include <errno.h>

	#define EWOULDBLOCK WSAEWOULDBLOCK
#else
	#error NOT IMPLEMENTED
#endif

#if defined(__cplusplus)
extern "C" {
#endif

IOHANDLE logfile = 0;

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

int dbg_log_to_file(const char *filename)
{
	logfile = io_open(filename, IOFLAG_WRITE);
	if(logfile)
		return 1;
	return 0;
}

void dbg_msg(const char *sys, const char *fmt, ...)
{
	va_list args;
#if defined(CONF_FAMILY_WINDOWS)
	char str[1024];
	va_start(args, fmt);
	vsprintf(str, fmt, args);
	va_end(args);
	OutputDebugString(str);
	OutputDebugString("\n");
#endif

	va_start(args, fmt);
	printf("[%08x][%s]: ", (int)time(0), sys);
	vprintf(fmt, args);
	va_end(args);
	printf("\n");
	fflush(stdout);
}

int memory_alloced = 0;

struct memheader
{
	const char *filename;
	int line;
	int size;
	struct memheader *prev;
	struct memheader *next;
};

struct memtail
{
	int guard;
};

struct memheader *first = 0;

int mem_allocated()
{
	return memory_alloced;
}

void *mem_alloc_debug(const char *filename, int line, unsigned size, unsigned alignment)
{
	/* TODO: fix alignment */
	/* TODO: add debugging */
	struct memheader *header = (struct memheader *)malloc(size+sizeof(struct memheader)+sizeof(struct memtail));
	struct memtail *tail = (struct memtail *)(((char*)(header+1))+size);
	header->size = size;
	header->filename = filename;
	header->line = line;
	memory_alloced += header->size;
	
	tail->guard = 0xbaadc0de;

	header->prev = (struct memheader *)0;
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
		struct memheader *header = (struct memheader *)p - 1;
		struct memtail *tail = (struct memtail *)(((char*)(header+1))+header->size);
		
		if(tail->guard != 0xbaadc0de)
			dbg_msg("mem", "!! %p", p);
		/* dbg_msg("mem", "-- %p", p); */
		memory_alloced -= header->size;
		
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
	struct memheader *header = first;
	IOHANDLE f = io_open("memory.txt", IOFLAG_WRITE);
	
	while(header)
	{
		sprintf(buf, "%s(%d): %d\n", header->filename, header->line, header->size);
		io_write(f, buf, strlen(buf));
		header = header->next;
	}
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

IOHANDLE io_open(const char *filename, int flags)
{
	if(flags == IOFLAG_READ)
		return (IOHANDLE)fopen(filename, "rb");
	if(flags == IOFLAG_WRITE)
		return (IOHANDLE)fopen(filename, "wb");
	return 0x0;
}

unsigned io_read(IOHANDLE io, void *buffer, unsigned size)
{
	return fread(buffer, 1, size, (FILE*)io);
}

unsigned io_skip(IOHANDLE io, unsigned size)
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
static void netaddr4_to_sockaddr(const NETADDR4 *src, struct sockaddr *dest)
{
	struct sockaddr_in *p = (struct sockaddr_in *)dest;
	mem_zero(p, sizeof(struct sockaddr_in));
	p->sin_family = AF_INET;
	p->sin_port = htons(src->port);
	p->sin_addr.s_addr = htonl(src->ip[0]<<24|src->ip[1]<<16|src->ip[2]<<8|src->ip[3]);
}

static void sockaddr_to_netaddr4(const struct sockaddr *src, NETADDR4 *dst)
{
	unsigned int ip = htonl(((struct sockaddr_in*)src)->sin_addr.s_addr);
	dst->port = htons(((struct sockaddr_in*)src)->sin_port);
	dst->ip[0] = (unsigned char)((ip>>24)&0xFF);
	dst->ip[1] = (unsigned char)((ip>>16)&0xFF);
	dst->ip[2] = (unsigned char)((ip>>8)&0xFF);
	dst->ip[3] = (unsigned char)(ip&0xFF);
}

int net_addr4_cmp(const NETADDR4 *a, const NETADDR4 *b)
{
	if(	a->ip[0] != b->ip[0] ||
		a->ip[1] != b->ip[1] ||
		a->ip[2] != b->ip[2] ||
		a->ip[3] != b->ip[3] ||
		a->port != b->port
	)
		return 1;
	return 0;
}


int net_host_lookup(const char *hostname, unsigned short port, NETADDR4 *addr)
{
	struct hostent* ip = gethostbyname(hostname);

	if(ip && ip->h_length > 0)
	{
		addr->ip[0] = ip->h_addr_list[0][0];
		addr->ip[1] = ip->h_addr_list[0][1];
		addr->ip[2] = ip->h_addr_list[0][2];
		addr->ip[3] = ip->h_addr_list[0][3];
		addr->port = port;
		return 0;
	}

	return -1;
}

NETSOCKET net_udp4_create(NETADDR4 bindaddr)
{
	struct sockaddr addr;
	unsigned int mode = 1;
	int broadcast = 1;

	/* create socket */
	int sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(sock < 0)
		return NETSOCKET_INVALID;
	
	/* bind, we should check for error */
	netaddr4_to_sockaddr(&bindaddr, &addr);
	if(bind(sock, &addr, sizeof(addr)) != 0)
	{
		net_udp4_close(sock);
		return NETSOCKET_INVALID;
	}
	
	/* set non-blocking */
#if defined(CONF_FAMILY_WINDOWS)
	ioctlsocket(sock, FIONBIO, (unsigned long *)&mode);
#else
	ioctl(sock, FIONBIO, (unsigned long *)&mode);
#endif

	/* set boardcast */
	setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (const char*)&broadcast, sizeof(broadcast));
	
	/* return */
	return sock;
}

int net_udp4_send(NETSOCKET sock, const NETADDR4 *addr, const void *data, int size)
{
	struct sockaddr sa;
	int d;
	mem_zero(&sa, sizeof(sa));
	netaddr4_to_sockaddr(addr, &sa);
	d = sendto((int)sock, (const char*)data, size, 0, &sa, sizeof(sa));
	if(d < 0)
		dbg_msg("net", "sendto error %d %x", d, d);
	return d;
}

int net_udp4_recv(NETSOCKET sock, NETADDR4 *addr, void *data, int maxsize)
{
	struct sockaddr from;
	int bytes;
	socklen_t fromlen = sizeof(struct sockaddr);
	bytes = recvfrom(sock, (char*)data, maxsize, 0, &from, &fromlen);
	if(bytes > 0)
	{
		sockaddr_to_netaddr4(&from, addr);
		return bytes;
	}
	else if(bytes == 0)
		return 0;
	return -1; /* error */
}

int net_udp4_close(NETSOCKET sock)
{
#if defined(CONF_FAMILY_WINDOWS)
	closesocket(sock);
#else
	close((int)sock);
#endif
	return 0;
}

NETSOCKET net_tcp4_create(const NETADDR4 *a)
{
    struct sockaddr addr;

    /* create socket */
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0)
        return NETSOCKET_INVALID;

    /* bind, we should check for error */
    netaddr4_to_sockaddr(a, &addr);
    bind(sock, &addr, sizeof(addr));

    /* return */
    return sock;
}

int net_tcp4_set_non_blocking(NETSOCKET sock)
{
	unsigned int mode = 1;
#if defined(CONF_FAMILY_WINDOWS)
	return ioctlsocket(sock, FIONBIO, (unsigned long *)&mode);
#else
	return ioctl(sock, FIONBIO, (unsigned long *)&mode);
#endif
}

int net_tcp4_set_blocking(NETSOCKET sock)
{
	unsigned int mode = 0;
#if defined(CONF_FAMILY_WINDOWS)
	return ioctlsocket(sock, FIONBIO, (unsigned long *)&mode);
#else
	return ioctl(sock, FIONBIO, (unsigned long *)&mode);
#endif
}

int net_tcp4_listen(NETSOCKET sock, int backlog)
{
	return listen(sock, backlog);
}

int net_tcp4_accept(NETSOCKET sock, NETSOCKET *new_sock, NETADDR4 *a)
{
	int s;
	socklen_t sockaddr_len;
	struct sockaddr addr;

	sockaddr_len = sizeof(addr);

	s = accept(sock, &addr, &sockaddr_len);

	if (s != -1)
	{
		sockaddr_to_netaddr4(&addr, a);
		*new_sock = s;
	}
	return s;
}

int net_tcp4_connect(NETSOCKET sock, const NETADDR4 *a)
{
  struct sockaddr addr;

  netaddr4_to_sockaddr(a, &addr);
  return connect(sock, &addr, sizeof(addr)); 
}

int net_tcp4_connect_non_blocking(NETSOCKET sock, const NETADDR4 *a)
{
	struct sockaddr addr;
	int res;

	netaddr4_to_sockaddr(a, &addr);
	net_tcp4_set_non_blocking(sock);
  	res = connect(sock, &addr, sizeof(addr));
	net_tcp4_set_blocking(sock);

	return res;
}

int net_tcp4_send(NETSOCKET sock, const void *data, int size)
{
  int d;
  d = send((int)sock, (const char*)data, size, 0);
  return d;
}

int net_tcp4_recv(NETSOCKET sock, void *data, int maxsize)
{
  int bytes;
  bytes = recv((int)sock, (char*)data, maxsize, 0);
  return bytes;
}

int net_tcp4_close(NETSOCKET sock)
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

int fs_listdir(const char *dir, fs_listdir_callback cb, void *user)
{
#if defined(CONF_FAMILY_WINDOWS)
	WIN32_FIND_DATA finddata;
	HANDLE handle;
	char buffer[1024*2];
	strcpy(buffer, dir);
	strcat(buffer, "/*");

	handle = FindFirstFileA(buffer, &finddata);

	if (handle == INVALID_HANDLE_VALUE)
		return 0;

	/* add all the entries */
	do
	{
		if(finddata.cFileName[0] != '.')
			cb(finddata.cFileName, 0, user);
	} while (FindNextFileA(handle, &finddata));

	FindClose(handle);
	return 0;
#else
	struct dirent *entry;
	DIR *d = opendir(dir);

	if(!d)
		return 0;
		
	while((entry = readdir(d)) != NULL)
		cb(entry->d_name, 0, user);

	/* close the directory and return */
	closedir(d);
	return 0;
#endif
}

int fs_storage_path(const char *appname, char *path, int max)
{
#if defined(CONF_FAMILY_WINDOWS)
	HRESULT r;
	char home[MAX_PATH];
	r = SHGetFolderPath (NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, home);
	if(r != 0)
			return 1;
	_snprintf(path, max, "%s/%s", home, appname);
	return 0;
#else
	char *home = getenv("HOME");
	int i;
	if(!home)
		return 0;

	snprintf(path, max, "%s/.%s", home, appname);
	for(i = strlen(home)+2; path[i]; i++)
		path[i] = tolower(path[i]);
	
	return 1;
#endif
}

int fs_makedir(const char *path)
{
#if defined(CONF_FAMILY_WINDOWS)
	if(_mkdir(path) == 0)
			return 0;
	if(errno == EEXIST)
		return 0;
	return 1;
#else
	if(mkdir(path, 0755) == 0)
		return 0;
	if(errno == EEXIST)
		return 0;
	return 1;
#endif
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

#if defined(__cplusplus)
}
#endif
