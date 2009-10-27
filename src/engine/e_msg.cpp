/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#include "e_common_interface.h"
#include "e_packer.h"

/* message packing */
static CPacker msg_packer;
static MSG_INFO pack_info;
static int packer_failed = 0;

void msg_pack_int(int i) { msg_packer.AddInt(i); }
void msg_pack_string(const char *p, int limit) { msg_packer.AddString(p, limit); }
void msg_pack_raw(const void *data, int size) { msg_packer.AddRaw((const unsigned char *)data, size); }

void msg_pack_start_system(int msg, int flags)
{
	msg_packer.Reset();
	pack_info.msg = (msg<<1)|1;
	pack_info.flags = flags;
	packer_failed = 0;
	
	msg_pack_int(pack_info.msg);
}

void msg_pack_start(int msg, int flags)
{
	msg_packer.Reset();
	pack_info.msg = msg<<1;
	pack_info.flags = flags;
	packer_failed = 0;
	
	msg_pack_int(pack_info.msg);
}

void msg_pack_end()
{
	if(msg_packer.Error())
	{
		packer_failed = 1;
		pack_info.size = 0;
		pack_info.data = (unsigned char *)"";
	}
	else
	{
		pack_info.size = msg_packer.Size();
		pack_info.data = msg_packer.Data();
	}
}

const MSG_INFO *msg_get_info()
{
	if(packer_failed)
		return 0;
	return &pack_info;
}

/* message unpacking */
static CUnpacker msg_unpacker;
int msg_unpack_start(const void *data, int data_size, int *system)
{
	int msg;
	msg_unpacker.Reset((const unsigned char *)data, data_size);
	msg = msg_unpack_int();
	*system = msg&1;
	return msg>>1;
}

int msg_unpack_int() { return msg_unpacker.GetInt(); }
const char *msg_unpack_string() { return msg_unpacker.GetString(); }
const unsigned char *msg_unpack_raw(int size)  { return msg_unpacker.GetRaw(size); }
int msg_unpack_error() { return msg_unpacker.Error(); }
