#include <baselib/stream/file.h>
#include <baselib/network.h>

#include "versions.h"

#define MACRO_MAKEINT(a,b,c,d) ((a<<24)|(b<<16)|(c<<8)|d)

// TODO: this is not KISS
class packet
{
	friend class connection;
protected:
	enum
	{
		MAX_PACKET_SIZE = 1024,
	};
	
	// packet data
	struct header
	{
		unsigned id;
		unsigned version;
		unsigned size_and_flags;
		unsigned crc;
		
		unsigned msg;
		unsigned ack;
		unsigned seq;
	};
	
	unsigned char packet_data[MAX_PACKET_SIZE];
	unsigned char *current;
	
	// these are used to prepend data in the packet
	// used for debugging so we have checks that we 
	// pack and unpack the same way
	enum
	{
		DEBUG_TYPE_INT=0x1,
		DEBUG_TYPE_STR=0x2,
		DEBUG_TYPE_RAW=0x3,
	};
	
	// writes an int to the packet
	void write_int_raw(int i)
	{
		// TODO: check for overflow
		*(int*)current = i;
		current += sizeof(int);
	}

	// reads an int from the packet
	int read_int_raw()
	{
		// TODO: check for overflow
		int i = *(int*)current;
		current += sizeof(int);
		return i;
	}
	
	void debug_insert_mark(int type, int size)
	{
		write_int_raw((type<<16)|size);
	}
	
	void debug_verify_mark(int type, int size)
	{
		if(read_int_raw() != ((type<<16) | size))
			dbg_assert(0, "error during packet disassembly");
	}
	
public:

	enum
	{
		FLAG_VITAL=1,
		FLAG_RESEND=2
	};

	packet(unsigned msg=0)
	{
		current = packet_data;
		current += sizeof(header);

		((header*)packet_data)->id = MACRO_MAKEINT('K','M','A',1);
		((header*)packet_data)->version = TEEWARS_NETVERSION;
		((header*)packet_data)->msg = msg;
	}
	
	void set_header(unsigned ack, unsigned seq)
	{
		((header*)packet_data)->ack = ack;
		((header*)packet_data)->seq = seq;
	}
	
	// writes an int to the packet
	void write_int(int i)
	{
		debug_insert_mark(DEBUG_TYPE_INT, 4);
		write_int_raw(i);
	}

	void write_raw(const char *raw, int size)
	{
		debug_insert_mark(DEBUG_TYPE_RAW, size);
		while(size--)
			*current++ = *raw++;
	}

	// writes a string to the packet
	void write_str(const char *str)
	{
		debug_insert_mark(DEBUG_TYPE_STR, 0);
		int s = strlen(str)+1;
		write_int_raw(s);
		for(;*str; current++, str++)
			*current = *str;
		*current = 0;
		current++;
	}
	
	// reads an int from the packet
	int read_int()
	{
		debug_verify_mark(DEBUG_TYPE_INT, 4);
		return read_int_raw();
	}
	
	// reads a string from the packet
	const char *read_str()
	{
		debug_verify_mark(DEBUG_TYPE_STR, 0);
		int size = read_int_raw();
		const char *s = (const char *)current;
		//dbg_msg("packet", "reading string '%s' (%d)", s, size);
		current += size;
		return s;
	}
	
	const char *read_raw(int size)
	{
		debug_verify_mark(DEBUG_TYPE_RAW, size);
		const char *d = (const char *)current;
		current += size;
		return d;
	}
	
	// TODO: impelement this
	bool is_good() const { return true; }

	unsigned version() const { return ((header*)packet_data)->version; }
	unsigned msg() const { return ((header*)packet_data)->msg; }
	unsigned seq() const { return ((header*)packet_data)->seq; }
	unsigned ack() const { return ((header*)packet_data)->ack; }
	unsigned flags() const { return (((header*)packet_data)->size_and_flags) & 0xffff; }
	
	// access functions to get the size and data
	int size() const { return (int)(current-(unsigned char*)packet_data); }
	int max_size() const { return MAX_PACKET_SIZE; }
	void *data() { return packet_data; }
};

// TODO: remove all the allocations from this class
class ring_buffer
{
	struct item
	{
		item *next;
		item *prev;
		int size;
	};
	
	item *first;
	item *last;
	
	unsigned buffer_size;
public:
	ring_buffer()
	{
		first = 0;
		last = 0;
		buffer_size = 0;
	}
	
	~ring_buffer()
	{
		reset();
	}
	
	void reset()
	{
		// clear all
		while(peek_data())
			next();		
	}
	
	void *alloc(int size)
	{
		item *i = (item*)mem_alloc(sizeof(item)+size, 1);
		i->size = size;
		
		i->prev = last;
		i->next = 0;
		if(last)
			last->next = i;
		else
			first = i;
		last = i;
		
		buffer_size += size;
		return (void*)(i+1);
	}
	
	unsigned peek_size()
	{
		if(!first)
			return 0;
		return first->size;
	}

	void *peek_data()
	{
		if(!first)
			return 0;
		return (void*)(first+1);
	}
	
	void next()
	{
		if(first)
		{
			item *next = first->next;
			buffer_size += first->size;
			mem_free(first);
			first = next;
			if(first)
				first->prev = 0;
			else
				last = 0;
		}
	}
	
	unsigned size() { return buffer_size; }
};

//
class connection
{
	baselib::socket_udp4 *socket;
	baselib::netaddr4 addr;
	unsigned seq;
	unsigned ack;
	
	unsigned counter_sent_bytes;
	unsigned counter_recv_bytes;
	
	int needs_resend;
	
	/*
	struct resend_packet
	{
		resend_packet *next;
		unsigned seq;
		unsigned msg;
		unsigned size;
		char data[1];
	};
	
	resend_packet *first_resend;
	resend_packet *last_resend;
	*/
	
	ring_buffer resend_buffer;

	void save_for_resend(packet *p)
	{
		/*
		packet *n = (packet *)resend_buffer.alloc(p->size());
		mem_copy(n->data(), p->data(), p->size());
		n->current = (unsigned char*)n->data() + p->size();
		*/
	}
	
	void remove_resends(unsigned ack)
	{
		/*
		while(1)
		{
			packet *p = (packet *)resend_buffer.peek_data();
			if(!p)
				break;
			
			if(p->seq() > ack)
				break;
			resend_buffer.next();
		}*/
	}
	
public:
	void counter_reset()
	{
		counter_sent_bytes = 0;
		counter_recv_bytes = 0;
	}
	
	void counter_get(unsigned *sent, unsigned *recved)
	{
		*sent = counter_sent_bytes;
		*recved = counter_recv_bytes;
	}

	void init(baselib::socket_udp4 *socket, const baselib::netaddr4 *addr)
	{
		resend_buffer.reset();
		
		this->addr = *addr;
		this->socket = socket;
		ack = 0;
		seq = 0;
		needs_resend = 0;
		counter_reset();
	}
	
	void send(packet *p)
	{
		if(p->flags()&packet::FLAG_VITAL)
			seq++;
		
		p->set_header(ack, seq);

		if(p->flags()&packet::FLAG_VITAL)
			save_for_resend(p);

		// TODO: request resend if needed, use needs_resend variable
		
		socket->send(&address(), p->data(), p->size());
		counter_sent_bytes += p->size();
	}
	
	packet *feed(packet *p)
	{
		counter_recv_bytes += p->size();
		
		if(p->flags()&packet::FLAG_VITAL)
		{
			if(p->seq() == ack+1)
			{
				// packet in seqence, ack it
				ack++;
				//dbg_msg("network/connection", "packet in sequence. seq/ack=%x", ack);
				return p;
			}
			else if(p->seq() > ack)
			{
				// packet loss
				needs_resend = 1;
				dbg_msg("network/connection", "packet loss! seq=%x ack=%x+1", p->seq(), ack);
				return p;
			}
			else
			{
				// we already got this packet
				return 0x0;
			}
		}
		
		// remove resends
		remove_resends(p->ack());
		
		// handle resends
		if(p->flags()&packet::FLAG_RESEND)
		{
			// peer as requested a resend of all non acked packages.
			
		}
		
		return p;		
	}
	
	const baselib::netaddr4 &address() const { return addr; }
	
	void update()
	{
	}
};

//const char *NETWORK_VERSION = "development";

enum
{
	NETMSG_CONTEXT_CONNECT=0x00010000,
	NETMSG_CONTEXT_GAME=0x00020000,
	NETMSG_CONTEXT_GLOBAL=0x00040000,
	
	// connection phase
	NETMSG_CLIENT_CONNECT=NETMSG_CONTEXT_CONNECT|1,
		// str32 name
		// str32 clan
		// str32 password
		// str32 skin	
	
	// TODO: These should be implemented to make the server send the map to the client on connect
	// NETMSG_CLIENT_FETCH,
	// NETMSG_SERVER_MAPDATA,
	
	NETMSG_SERVER_ACCEPT=NETMSG_CONTEXT_CONNECT|2,
		// str32 mapname

	
	NETMSG_CLIENT_DONE=NETMSG_CONTEXT_CONNECT|3,
		// nothing
	
	// game phase
	NETMSG_SERVER_SNAP = NETMSG_CONTEXT_GAME|1, // server will spam these
		// int num_parts
		// int part
		// int size
		// data *
		
	NETMSG_CLIENT_INPUT = NETMSG_CONTEXT_GAME|1, // client will spam these
		// int input[MAX_INPUTS]
	
	NETMSG_SERVER_EVENT = NETMSG_CONTEXT_GAME|2,
	NETMSG_CLIENT_EVENT = NETMSG_CONTEXT_GAME|2,

	NETMSG_CLIENT_CHECKALIVE = NETMSG_CONTEXT_GAME|3, // check if client is alive
	
	NETMSG_CLIENT_ERROR=0x0fffffff,
		// str128 reason
		
	NETMSG_SERVER_ERROR=0x0fffffff,
		// str128 reason
};

enum
{
	MAX_NAME_LENGTH=32,
	MAX_CLANNAME_LENGTH=32,
	MAX_INPUT_SIZE=128,
	MAX_SNAPSHOT_SIZE=64*1024,
	MAX_SNAPSHOT_PACKSIZE=768
};
