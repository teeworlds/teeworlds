import os, imp, sys
from datatypes import *
import content
import network

def create_enum_table(names, num):
	lines = []
	lines += ["enum", "{"]
	lines += ["\t%s=0,"%names[0]]
	for name in names[1:]:
		lines += ["\t%s,"%name]
	lines += ["\t%s" % num, "};"]
	return lines

def create_flags_table(names):
	lines = []
	lines += ["enum", "{"]
	i = 0
	for name in names:
		lines += ["\t%s = 1<<%d," % (name,i)]
		i += 1
	lines += ["};"]
	return lines

def EmitEnum(names, num):
	print("enum")
	print("{")
	print("\t%s=0," % names[0])
	for name in names[1:]:
		print("\t%s," % name)
	print("\t%s" % num)
	print("};")

def EmitFlags(names, num):
	print("enum")
	print("{")
	i = 0
	for name in names:
		print("\t%s = 1<<%d," % (name,i))
		i += 1
	print("};")

gen_network_header = False
gen_network_source = False
gen_client_content_header = False
gen_client_content_source = False
gen_server_content_header = False
gen_server_content_source = False

if "network_header" in sys.argv: gen_network_header = True
if "network_source" in sys.argv: gen_network_source = True
if "client_content_header" in sys.argv: gen_client_content_header = True
if "client_content_source" in sys.argv: gen_client_content_source = True
if "server_content_header" in sys.argv: gen_server_content_header = True
if "server_content_source" in sys.argv: gen_server_content_source = True

if gen_client_content_header:
	print("#ifndef CLIENT_CONTENT_HEADER")
	print("#define CLIENT_CONTENT_HEADER")

if gen_server_content_header:
	print("#ifndef SERVER_CONTENT_HEADER")
	print("#define SERVER_CONTENT_HEADER")


if gen_client_content_header or gen_server_content_header:
	# print some includes
	print('#include <engine/graphics.h>')
	print('#include <engine/sound.h>')

	# emit the type declarations
	contentlines = open("datasrc/content.py", "rb").readlines()
	order = []
	for line in contentlines:
		line = line.strip()
		if line[:6] == "class ".encode() and "(Struct)".encode() in line:
			order += [line.split()[1].split("(".encode())[0].decode("ascii")]
	for name in order:
		EmitTypeDeclaration(content.__dict__[name])

	# the container pointer
	print('extern CDataContainer *g_pData;')

	# enums
	EmitEnum(["IMAGE_%s"%i.name.value.upper() for i in content.container.images.items], "NUM_IMAGES")
	EmitEnum(["ANIM_%s"%i.name.value.upper() for i in content.container.animations.items], "NUM_ANIMS")
	EmitEnum(["SPRITE_%s"%i.name.value.upper() for i in content.container.sprites.items], "NUM_SPRITES")

if gen_client_content_source or gen_server_content_source:
	if gen_client_content_source:
		print('#include "client_data.h"')
	if gen_server_content_source:
		print('#include "server_data.h"')
	EmitDefinition(content.container, "datacontainer")
	print('CDataContainer *g_pData = &datacontainer;')

# NETWORK
if gen_network_header:

	print("#ifndef GAME_GENERATED_PROTOCOL_H")
	print("#define GAME_GENERATED_PROTOCOL_H")
	print(network.RawHeader)

	for e in network.Enums:
		for l in create_enum_table(["%s_%s"%(e.name, v) for v in e.values], 'NUM_%sS'%e.name): print(l)
		print("")

	for e in network.Flags:
		for l in create_flags_table(["%s_%s" % (e.name, v) for v in e.values]): print(l)
		print("")

	for l in create_enum_table(["NETOBJ_INVALID"]+[o.enum_name for o in network.Objects], "NUM_NETOBJTYPES"): print(l)
	print("")
	for l in create_enum_table(["NETMSG_INVALID"]+[o.enum_name for o in network.Messages], "NUM_NETMSGTYPES"): print(l)
	print("")

	for item in network.Objects + network.Messages:
		for line in item.emit_declaration():
			print(line)
		print("")

	EmitEnum(["SOUND_%s"%i.name.value.upper() for i in content.container.sounds.items], "NUM_SOUNDS")
	EmitEnum(["WEAPON_%s"%i.name.value.upper() for i in content.container.weapons.id.items], "NUM_WEAPONS")

	print("""

class CNetObjHandler
{
	const char *m_pMsgFailedOn;
	char m_aMsgData[1024];
	const char *m_pObjFailedOn;
	int m_NumObjFailures;
	bool CheckInt(const char *pErrorMsg, int Value, int Min, int Max);
	bool CheckFlag(const char *pErrorMsg, int Value, int Mask);

	static const char *ms_apObjNames[];
	static int ms_aObjSizes[];
	static const char *ms_apMsgNames[];

public:
	CNetObjHandler();

	int ValidateObj(int Type, const void *pData, int Size);
	const char *GetObjName(int Type) const;
	int GetObjSize(int Type) const;
	const char *FailedObjOn() const;
	int NumObjFailures() const;
	
	const char *GetMsgName(int Type) const;
	void *SecureUnpackMsg(int Type, CUnpacker *pUnpacker);
	const char *FailedMsgOn() const;
};

""")

	print("#endif // GAME_GENERATED_PROTOCOL_H")


if gen_network_source:
	# create names
	lines = []

	lines += ['#include <engine/shared/protocol.h>']
	lines += ['#include <engine/message.h>']
	lines += ['#include "protocol.h"']

	lines += ['CNetObjHandler::CNetObjHandler()']
	lines += ['{']
	lines += ['\tm_pMsgFailedOn = "";']
	lines += ['\tm_pObjFailedOn = "";']
	lines += ['\tm_NumObjFailures = 0;']
	lines += ['}']
	lines += ['']
	lines += ['const char *CNetObjHandler::FailedObjOn() const { return m_pObjFailedOn; }']
	lines += ['int CNetObjHandler::NumObjFailures() const { return m_NumObjFailures; }']
	lines += ['const char *CNetObjHandler::FailedMsgOn() const { return m_pMsgFailedOn; }']
	lines += ['']
	lines += ['']
	lines += ['']
	lines += ['']

	lines += ['static const int max_int = 0x7fffffff;']
	lines += ['']

	lines += ['bool CNetObjHandler::CheckInt(const char *pErrorMsg, int Value, int Min, int Max)']
	lines += ['{']
	lines += ['\tif(Value < Min || Value > Max) { m_pObjFailedOn = pErrorMsg; m_NumObjFailures++; return false; }']
	lines += ['\treturn true;']
	lines += ['}']
	lines += ['']

	lines += ['bool CNetObjHandler::CheckFlag(const char *pErrorMsg, int Value, int Mask)']
	lines += ['{']
	lines += ['\tif((Value&Mask) != Value) { m_pObjFailedOn = pErrorMsg; m_NumObjFailures++; return false; }']
	lines += ['\treturn true;']
	lines += ['}']
	lines += ['']

	lines += ["const char *CNetObjHandler::ms_apObjNames[] = {"]
	lines += ['\t"invalid",']
	lines += ['\t"%s",' % o.name for o in network.Objects]
	lines += ['\t""', "};", ""]

	lines += ["int CNetObjHandler::ms_aObjSizes[] = {"]
	lines += ['\t0,']
	lines += ['\tsizeof(%s),' % o.struct_name for o in network.Objects]
	lines += ['\t0', "};", ""]


	lines += ['const char *CNetObjHandler::ms_apMsgNames[] = {']
	lines += ['\t"invalid",']
	for msg in network.Messages:
		lines += ['\t"%s",' % msg.name]
	lines += ['\t""']
	lines += ['};']
	lines += ['']

	lines += ['const char *CNetObjHandler::GetObjName(int Type) const']
	lines += ['{']
	lines += ['\tif(Type < 0 || Type >= NUM_NETOBJTYPES) return "(out of range)";']
	lines += ['\treturn ms_apObjNames[Type];']
	lines += ['};']
	lines += ['']

	lines += ['int CNetObjHandler::GetObjSize(int Type) const']
	lines += ['{']
	lines += ['\tif(Type < 0 || Type >= NUM_NETOBJTYPES) return 0;']
	lines += ['\treturn ms_aObjSizes[Type];']
	lines += ['};']
	lines += ['']


	lines += ['const char *CNetObjHandler::GetMsgName(int Type) const']
	lines += ['{']
	lines += ['\tif(Type < 0 || Type >= NUM_NETMSGTYPES) return "(out of range)";']
	lines += ['\treturn ms_apMsgNames[Type];']
	lines += ['};']
	lines += ['']


	for l in lines:
		print(l)

	if 0:
		for item in network.Objects:
			for line in item.emit_validate():
				print(line)
			print("")

	# create validate tables
		lines = []
		lines += ['static int validate_invalid(void *data, int size) { return -1; }']
		lines += ["typedef int(*VALIDATEFUNC)(void *data, int size);"]
		lines += ["static VALIDATEFUNC validate_funcs[] = {"]
		lines += ['\tvalidate_invalid,']
		lines += ['\tvalidate_%s,' % o.name for o in network.Objects]
		lines += ["\t0x0", "};", ""]

		lines += ["int netobj_validate(int type, void *data, int size)"]
		lines += ["{"]
		lines += ["\tif(type < 0 || type >= NUM_NETOBJTYPES) return -1;"]
		lines += ["\treturn validate_funcs[type](data, size);"]
		lines += ["};", ""]

	lines = []
	lines += ['int CNetObjHandler::ValidateObj(int Type, const void *pData, int Size)']
	lines += ['{']
	lines += ['\tswitch(Type)']
	lines += ['\t{']

	for item in network.Objects:
		for line in item.emit_validate():
			lines += ["\t" + line]
		lines += ['\t']
	lines += ['\t}']
	lines += ['\treturn -1;']
	lines += ['};']
	lines += ['']

 #int Validate(int Type, void *pData, int Size);

	if 0:
		for item in network.Messages:
			for line in item.emit_unpack():
				print(line)
			print("")

		lines += ['static void *secure_unpack_invalid(CUnpacker *pUnpacker) { return 0; }']
		lines += ['typedef void *(*SECUREUNPACKFUNC)(CUnpacker *pUnpacker);']
		lines += ['static SECUREUNPACKFUNC secure_unpack_funcs[] = {']
		lines += ['\tsecure_unpack_invalid,']
		for msg in network.Messages:
			lines += ['\tsecure_unpack_%s,' % msg.name]
		lines += ['\t0x0']
		lines += ['};']

	#
	lines += ['void *CNetObjHandler::SecureUnpackMsg(int Type, CUnpacker *pUnpacker)']
	lines += ['{']
	lines += ['\tm_pMsgFailedOn = 0;']
	lines += ['\tm_pObjFailedOn = 0;']
	lines += ['\tswitch(Type)']
	lines += ['\t{']


	for item in network.Messages:
		for line in item.emit_unpack():
			lines += ["\t" + line]
		lines += ['\t']

	lines += ['\tdefault:']
	lines += ['\t\tm_pMsgFailedOn = "(type out of range)";']
	lines += ['\t\tbreak;']
	lines += ['\t}']
	lines += ['\t']
	lines += ['\tif(pUnpacker->Error())']
	lines += ['\t\tm_pMsgFailedOn = "(unpack error)";']
	lines += ['\t']
	lines += ['\tif(m_pMsgFailedOn || m_pObjFailedOn) {']
	lines += ['\t\tif(!m_pMsgFailedOn)']
	lines += ['\t\t\tm_pMsgFailedOn = "";']
	lines += ['\t\tif(!m_pObjFailedOn)']
	lines += ['\t\t\tm_pObjFailedOn = "";']
	lines += ['\t\treturn 0;']
	lines += ['\t}']
	lines += ['\tm_pMsgFailedOn = "";']
	lines += ['\tm_pObjFailedOn = "";']
	lines += ['\treturn m_aMsgData;']
	lines += ['};']
	lines += ['']


	for l in lines:
		print(l)

if gen_client_content_header or gen_server_content_header:
	print("#endif")
