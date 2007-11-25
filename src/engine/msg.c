/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#include "interface.h"
#include "packer.h"

/* message packing */
static PACKER msg_packer;
static MSG_INFO pack_info;

void msg_pack_int(int i) { packer_add_int(&msg_packer, i); }
void msg_pack_string(const char *p, int limit) { packer_add_string(&msg_packer, p, limit); }
void msg_pack_raw(const void *data, int size) { packer_add_raw(&msg_packer, (const unsigned char *)data, size); }

void msg_pack_start_system(int msg, int flags)
{
	packer_reset(&msg_packer);
	pack_info.msg = (msg<<1)|1;
	pack_info.flags = flags;
	
	msg_pack_int(pack_info.msg);
}

void msg_pack_start(int msg, int flags)
{
	packer_reset(&msg_packer);
	pack_info.msg = msg<<1;
	pack_info.flags = flags;
	
	msg_pack_int(pack_info.msg);
}

void msg_pack_end()
{
	pack_info.size = packer_size(&msg_packer);
	pack_info.data = packer_data(&msg_packer);
}

const MSG_INFO *msg_get_info()
{
	return &pack_info;
}

/* message unpacking */
static UNPACKER msg_unpacker;
int msg_unpack_start(const void *data, int data_size, int *system)
{
	int msg;
	unpacker_reset(&msg_unpacker, (const unsigned char *)data, data_size);
	msg = msg_unpack_int();
	*system = msg&1;
	return msg>>1;
}

int msg_unpack_int() { return unpacker_get_int(&msg_unpacker); }
const char *msg_unpack_string() { return unpacker_get_string(&msg_unpacker); }
const unsigned char *msg_unpack_raw(int size)  { return unpacker_get_raw(&msg_unpacker, size); }
