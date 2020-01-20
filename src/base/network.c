/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <stdlib.h>

#include "detect.h"
#include "memory.h"
#include "strings.h"
#include "debug.h"
#include "network.h"

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
	#include <arpa/inet.h>

	#include <dirent.h>

	#if defined(CONF_PLATFORM_MACOSX)
		#include <Carbon/Carbon.h>
	#endif

#elif defined(CONF_FAMILY_WINDOWS)
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
	#include <winsock2.h>
	#include <ws2tcpip.h>
#else
	#error NOT IMPLEMENTED
#endif

#if defined(CONF_PLATFORM_SOLARIS)
	#include <sys/filio.h> // FIO* flags
#endif

#if defined(__cplusplus)
extern "C" {
#endif

static NETSTATS network_stats = {0};

static NETSOCKET invalid_socket = {NETTYPE_INVALID, -1, -1};

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
		dbg_msg("system", "couldn't not convert NETADDR of type %d to ipv6", src->type);
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

int net_addr_comp(const NETADDR *a, const NETADDR *b)
{
	return mem_comp(a, b, sizeof(NETADDR));
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
			if(WSAStringToAddress(buf, AF_INET6, NULL, (struct sockaddr *)&sa6, &size) != 0)
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
		int error = WSAGetLastError();
		if(FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS, 0, error, 0, buf, sizeof(buf), 0) == 0)
			buf[0] = 0;
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
			int error = WSAGetLastError();
			if(error == WSAEADDRINUSE && use_random_port)
				continue;
			if(FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS, 0, error, 0, buf, sizeof(buf), 0) == 0)
				buf[0] = 0;
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
			dbg_msg("net", "can't sent ipv4 traffic to this socket");
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
			dbg_msg("net", "can't sent ipv6 traffic to this socket");
	}
	/*
	else
		dbg_msg("net", "can't sent to network of type %d", addr->type);
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

void net_stats(NETSTATS *stats_inout)
{
	*stats_inout = network_stats;
}

unsigned bytes_be_to_uint(const unsigned char *bytes)
{
	return (bytes[0]<<24) | (bytes[1]<<16) | (bytes[2]<<8) | bytes[3];
}

#if defined(__cplusplus)
}
#endif
