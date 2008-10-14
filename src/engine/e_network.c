/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#include <base/system.h>

#include <string.h> /* strlen */

#include "e_config.h"
#include "e_network.h"
#include "e_network_internal.h"
#include "e_huffman.h"

void recvinfo_clear(NETRECVINFO *info)
{
	info->valid = 0;
}

void recvinfo_start(NETRECVINFO *info, NETADDR *addr, NETCONNECTION *conn, int cid)
{
	info->addr = *addr;
	info->conn = conn;
	info->client_id = cid;
	info->current_chunk = 0;
	info->valid = 1;
}

/* TODO: rename this function */
int recvinfo_fetch_chunk(NETRECVINFO *info, NETCHUNK *chunk)
{
	NETCHUNKHEADER header;
	unsigned char *data = info->data.chunk_data;
	int i;
	
	while(1)
	{
		/* check for old data to unpack */
		if(!info->valid || info->current_chunk >= info->data.num_chunks)
		{
			recvinfo_clear(info);
			return 0;
		}
		
		/* TODO: add checking here so we don't read too far */
		for(i = 0; i < info->current_chunk; i++)
		{
			data = unpack_chunk_header(data, &header);
			data += header.size;
		}
		
		/* unpack the header */	
		data = unpack_chunk_header(data, &header);
		info->current_chunk++;
		
		/* handle sequence stuff */
		if(info->conn && (header.flags&NET_CHUNKFLAG_VITAL))
		{
			if(header.sequence == (info->conn->ack+1)%NET_MAX_SEQUENCE)
			{
				/* in sequence */
				info->conn->ack = (info->conn->ack+1)%NET_MAX_SEQUENCE;
			}
			else
			{
				/* out of sequence, request resend */
				dbg_msg("conn", "asking for resend %d %d", header.sequence, (info->conn->ack+1)%NET_MAX_SEQUENCE);
				conn_want_resend(info->conn);
				continue; /* take the next chunk in the packet */
			}
		}
		
		/* fill in the info */
		chunk->client_id = info->client_id;
		chunk->address = info->addr;
		chunk->flags = 0;
		chunk->data_size = header.size;
		chunk->data = data;
		return 1;
	}
}


static IOHANDLE datalog = 0;
static HUFFMAN_STATE huffmanstate;

#define COMPRESSION 1

/* packs the data tight and sends it */
void send_packet_connless(NETSOCKET socket, NETADDR *addr, const void *data, int data_size)
{
	unsigned char buffer[NET_MAX_PACKETSIZE];
	buffer[0] = 0xff;
	buffer[1] = 0xff;
	buffer[2] = 0xff;
	buffer[3] = 0xff;
	buffer[4] = 0xff;
	buffer[5] = 0xff;
	mem_copy(&buffer[6], data, data_size);
	net_udp_send(socket, addr, buffer, 6+data_size);
}

int netcommon_compress(const void *data, int data_size, void *output, int output_size)
{
	return huffman_compress(&huffmanstate, data, data_size, output, output_size);
}

int netcommon_decompress(const void *data, int data_size, void *output, int output_size)
{
	return huffman_decompress(&huffmanstate, data, data_size, output, output_size);
}

void send_packet(NETSOCKET socket, NETADDR *addr, NETPACKETCONSTRUCT *packet)
{
	unsigned char buffer[NET_MAX_PACKETSIZE];
	int compressed_size = -1;
	int final_size = -1;
	
	/* log the data */
	if(datalog)
	{
		io_write(datalog, &packet->data_size, sizeof(packet->data_size));
		io_write(datalog, &packet->chunk_data, packet->data_size);
	}
	
	/* compress if its enabled */
	if(COMPRESSION)
		compressed_size = huffman_compress(&huffmanstate, packet->chunk_data, packet->data_size, &buffer[3], NET_MAX_PACKETSIZE-4);

	/* check if the compression was enabled, successful and good enough	*/
	if(compressed_size > 0 && compressed_size < packet->data_size)
	{
		final_size = compressed_size;
		packet->flags |= NET_PACKETFLAG_COMPRESSION;
	}
	else
	{
		/* use uncompressed data */
		final_size = packet->data_size;
		mem_copy(&buffer[3], packet->chunk_data, packet->data_size);
		packet->flags &= ~NET_PACKETFLAG_COMPRESSION;
	}

	/* set header and send the packet if all things are good */
	if(final_size >= 0)
	{
		buffer[0] = ((packet->flags<<4)&0xf0)|((packet->ack>>8)&0xf);
		buffer[1] = packet->ack&0xff;
		buffer[2] = packet->num_chunks;
		net_udp_send(socket, addr, buffer, NET_PACKETHEADERSIZE+final_size);
	}
}

/* TODO: rename this function */
int unpack_packet(unsigned char *buffer, int size, NETPACKETCONSTRUCT *packet)
{
	/* check the size */
	if(size < NET_PACKETHEADERSIZE || size > NET_MAX_PACKETSIZE)
	{
		dbg_msg("", "packet too small, %d", size);
		return -1;
	}
	
	/* read the packet */
	packet->flags = buffer[0]>>4;
	packet->ack = ((buffer[0]&0xf)<<8) | buffer[1];
	packet->num_chunks = buffer[2];
	packet->data_size = size - NET_PACKETHEADERSIZE;
	
	if(packet->flags&NET_PACKETFLAG_CONNLESS)
	{
		packet->flags = NET_PACKETFLAG_CONNLESS;
		packet->ack = 0;
		packet->num_chunks = 0;
		packet->data_size = size - 6;
		mem_copy(packet->chunk_data, &buffer[6], packet->data_size);
	}
	else
	{
		if(packet->flags&NET_PACKETFLAG_COMPRESSION)
			huffman_decompress(&huffmanstate, &buffer[3], packet->data_size, packet->chunk_data, sizeof(packet->chunk_data));
		else
			mem_copy(packet->chunk_data, &buffer[3], packet->data_size);
	}
	
	/* return success */
	return 0;
}


/* TODO: change the arguments of this function */
unsigned char *pack_chunk_header(unsigned char *data, int flags, int size, int sequence)
{
	data[0] = ((flags&3)<<6)|((size>>4)&0x3f);
	data[1] = (size&0xf);
	if(flags&NET_CHUNKFLAG_VITAL)
	{
		data[1] |= (sequence>>2)&0xf0;
		data[2] = sequence&0xff;
		return data + 3;
	}
	return data + 2;
}

unsigned char *unpack_chunk_header(unsigned char *data, NETCHUNKHEADER *header)
{
	header->flags = (data[0]>>6)&3;
	header->size = ((data[0]&0x3f)<<4) | (data[1]&0xf);
	header->sequence = -1;
	if(header->flags&NET_CHUNKFLAG_VITAL)
	{
		header->sequence = ((data[1]&0xf0)<<2) | data[2];
		return data + 3;
	}
	return data + 2;
}


void netcommon_openlog(const char *filename)
{
	datalog = io_open(filename, IOFLAG_WRITE);
}

static const unsigned freq_table[256+1] = {
	1<<30,4545,2657,431,1950,919,444,482,2244,617,838,542,715,1814,304,240,754,212,647,186,
	283,131,146,166,543,164,167,136,179,859,363,113,157,154,204,108,137,180,202,176,
	872,404,168,134,151,111,113,109,120,126,129,100,41,20,16,22,18,18,17,19,
	16,37,13,21,362,166,99,78,95,88,81,70,83,284,91,187,77,68,52,68,
	59,66,61,638,71,157,50,46,69,43,11,24,13,19,10,12,12,20,14,9,
	20,20,10,10,15,15,12,12,7,19,15,14,13,18,35,19,17,14,8,5,
	15,17,9,15,14,18,8,10,2173,134,157,68,188,60,170,60,194,62,175,71,
	148,67,167,78,211,67,156,69,1674,90,174,53,147,89,181,51,174,63,163,80,
	167,94,128,122,223,153,218,77,200,110,190,73,174,69,145,66,277,143,141,60,
	136,53,180,57,142,57,158,61,166,112,152,92,26,22,21,28,20,26,30,21,
	32,27,20,17,23,21,30,22,22,21,27,25,17,27,23,18,39,26,15,21,
	12,18,18,27,20,18,15,19,11,17,33,12,18,15,19,18,16,26,17,18,
	9,10,25,22,22,17,20,16,6,16,15,20,14,18,24,335,1517};

void netcommon_init()
{
	huffman_init(&huffmanstate, freq_table);
}
