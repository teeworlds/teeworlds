#include <baselib/stream/file.h>
#include <baselib/network.h>

// TODO: this is not KISS
class packet
{
protected:
	enum
	{
		MAX_PACKET_SIZE = 1024,
	};
	
	// packet data
	struct header
	{
		unsigned msg;
		unsigned ack;
		unsigned seq;
	};
	
	unsigned char packet_data[MAX_PACKET_SIZE-sizeof(header)];
	unsigned char *current;
	
	// these are used to prepend data in the packet
	// used for debugging so we have checks that we 
	// pack and unpack the same way
	enum
	{
		DEBUG_TYPE_SHIFT=24,
		DEBUG_SIZE_MASK=0xffff,
		DEBUG_TYPE_INT=0x1,
		DEBUG_TYPE_STR=0x2,
		DEBUG_TYPE_RAW=0x3,
	};
	
	// writes an int to the packet
	void write_int_raw(int i)
	{
		// TODO: byteorder fix, should make sure that it writes big endian
		// TODO: check for overflow
		*(int*)current = i;
		current += sizeof(int);
	}

	// reads an int from the packet
	void read_int_raw(int *i)
	{
		// TODO: byteorder fix, should make sure that it reads big endian
		// TODO: check for overflow
		*i = *(int*)current;
		current += sizeof(int);
	}
	
public:
	packet(unsigned msg=0)
	{
		current = packet_data;
		current += sizeof(header);
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
		write_int_raw((DEBUG_TYPE_INT<<DEBUG_TYPE_SHIFT) | 4);
		write_int_raw(i);
	}

	void write_raw(const char *raw, int size)
	{
		write_int_raw((DEBUG_TYPE_RAW<<DEBUG_TYPE_SHIFT) | size);
		while(size--)
			*current++ = *raw++;
	}

	// writes a string to the packet
	void write_str(const char *str, int storagesize)
	{
		write_int_raw((DEBUG_TYPE_STR<<DEBUG_TYPE_SHIFT) | storagesize);
		
		while(storagesize-1) // make sure to zero terminate
		{
			if(!*str)
				break;
			
			*current = *str;
			current++;
			str++;
			storagesize--;
		}
		
		while(storagesize)
		{
			*current = 0;
			current++;
			storagesize--;
		}
	}
	
	// reads an int from the packet
	int read_int()
	{
		int t;
		read_int_raw(&t);
		dbg_assert(t == ((DEBUG_TYPE_INT<<DEBUG_TYPE_SHIFT) | 4), "error during packet disassembly");
		read_int_raw(&t);
		return t;
	}
	
	// reads a string from the packet
	void read_str(char *str, int storagesize)
	{
		int t;
		read_int_raw(&t);
		dbg_assert(t == ((DEBUG_TYPE_STR<<DEBUG_TYPE_SHIFT) | storagesize), "error during packet disassembly.");
		mem_copy(str, current, storagesize);
		current += storagesize;
		dbg_assert(str[storagesize-1] == 0, "string should be zero-terminated.");
	}
	
	void read_raw(char *data, int size)
	{
		int t;
		read_int_raw(&t);
		dbg_assert(t == ((DEBUG_TYPE_RAW<<DEBUG_TYPE_SHIFT) | size), "error during packet disassembly.");
		while(size--)
			*data++ = *current++;
	}
	
	// TODO: impelement this
	bool is_good() const { return true; }

	unsigned msg() const { return ((header*)packet_data)->msg; }
	unsigned seq() const { return ((header*)packet_data)->seq; }
	unsigned ack() const { return ((header*)packet_data)->ack; }
	
	// access functions to get the size and data
	int size() const { return (int)(current-(unsigned char*)packet_data); }
	int max_size() const { return MAX_PACKET_SIZE; }
	void *data() { return packet_data; }
};

//
class connection
{
	baselib::socket_udp4 *socket;
	baselib::netaddr4 addr;
	unsigned seq;
	unsigned ack;
	unsigned last_ack;
	
	unsigned counter_sent_bytes;
	unsigned counter_recv_bytes;
	
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
		this->addr = *addr;
		this->socket = socket;
		last_ack = 0;
		ack = 0;
		seq = 0;
		counter_reset();
	}
	
	void send(packet *p)
	{
		if(p->msg()&(31<<1))
		{
			// vital packet
			seq++;
			// TODO: save packet, we might need to retransmit
		}
		
		//dbg_msg("network/connection", "sending packet. msg=%x size=%x seq=%x ", p->msg(), p->size(), seq);
		p->set_header(ack, seq);
		socket->send(&address(), p->data(), p->size());
		counter_sent_bytes += p->size();
	}
	
	packet *feed(packet *p)
	{
		counter_recv_bytes += p->size();
		
		if(p->msg()&(31<<1))
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
				// TODO: request resend
				// packet loss
				dbg_msg("network/connection", "packet loss! seq=%x ack=%x+1", p->seq(), ack);
				return p;
			}
			else
			{
				// we already got this packet
				return 0x0;
			}
		}
		
		if(last_ack != p->ack())
		{
			// TODO: remove acked packets
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
	NETMSG_VITAL=0x80000000,
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

	
	NETMSG_CLIENT_DONE=NETMSG_VITAL|NETMSG_CONTEXT_CONNECT|3,
		// nothing
	
	// game phase
	NETMSG_SERVER_SNAP = NETMSG_CONTEXT_GAME|1, // server will spam these
		// int num_parts
		// int part
		// int size
		// data *
		
	NETMSG_CLIENT_INPUT = NETMSG_CONTEXT_GAME|1, // client will spam these
		// int input[MAX_INPUTS]
	
	NETMSG_SERVER_EVENT = NETMSG_CONTEXT_GAME|NETMSG_VITAL|2,
	NETMSG_CLIENT_EVENT = NETMSG_CONTEXT_GAME|NETMSG_VITAL|2,

	NETMSG_CLIENT_CHECKALIVE = NETMSG_CONTEXT_GAME|NETMSG_VITAL|3, // check if client is alive
	
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
