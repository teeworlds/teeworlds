
#include "interface.h"
#include "packet.h"

// message packing
static data_packer packer;
static msg_info pack_info;

void msg_pack_int(int i) { packer.add_int(i); }
void msg_pack_string(const char *p, int limit) { packer.add_string(p, limit); }
void msg_pack_raw(const void *data, int size) { packer.add_raw((const unsigned char *)data, size); }

void msg_pack_start(int msg, int flags)
{
	packer.reset();
	pack_info.msg = msg;
	pack_info.flags = flags;
	
	msg_pack_int(msg);
}

void msg_pack_end()
{
	pack_info.size = packer.size();
	pack_info.data = packer.data();
}

const msg_info *msg_get_info()
{
	return &pack_info;
}

// message unpacking
static data_unpacker unpacker;
int msg_unpack_start(const void *data, int data_size)
{
	unpacker.reset((const unsigned char *)data, data_size);
	return msg_unpack_int();
}

int msg_unpack_int() { return unpacker.get_int(); }
const char *msg_unpack_string() { return unpacker.get_string(); }
const unsigned char *msg_unpack_raw(int size)  { return unpacker.get_raw(size); }
