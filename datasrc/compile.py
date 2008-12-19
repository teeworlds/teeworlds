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
	print "enum"
	print "{"
	print "\t%s=0,"%names[0]
	for name in names[1:]:
		print "\t%s,"%name
	print "\t%s" % num
	print "};"

def EmitFlags(names, num):
	print "enum"
	print "{"
	i = 0
	for name in names:
		print "\t%s = 1<<%d," % (name,i)
		i += 1
	print "};"
		

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
	print "#ifndef CLIENT_CONTENT_HEADER"
	print "#define CLIENT_CONTENT_HEADER"

if gen_server_content_header:
	print "#ifndef SERVER_CONTENT_HEADER"
	print "#define SERVER_CONTENT_HEADER"


if gen_client_content_header or gen_server_content_header:
	# emit the type declarations
	contentlines = file("datasrc/content.py").readlines()
	order = []
	for line in contentlines:
		line = line.strip()
		if line[:6] == "class " and '(Struct)' in line:
			order += [line.split()[1].split("(")[0]]
	for name in order:
		EmitTypeDeclaration(content.__dict__[name])
		
	# the container pointer
	print 'extern DATACONTAINER *data;';
	
	# enums
	EmitEnum(["IMAGE_%s"%i.name.value.upper() for i in content.container.images.items], "NUM_IMAGES")
	EmitEnum(["ANIM_%s"%i.name.value.upper() for i in content.container.animations.items], "NUM_ANIMS")
	EmitEnum(["SPRITE_%s"%i.name.value.upper() for i in content.container.sprites.items], "NUM_SPRITES")

if gen_client_content_source or gen_server_content_source:
	if gen_client_content_source:
		print '#include "gc_data.hpp"'
	if gen_server_content_source:
		print '#include "gs_data.hpp"'
	EmitDefinition(content.container, "datacontainer")
	print 'DATACONTAINER *data = &datacontainer;';

# NETWORK
if gen_network_header:
	
	print "#ifndef FILE_G_PROTOCOL_H"
	print "#define FILE_G_PROTOCOL_H"
	print network.RawHeader
	
	for e in network.Enums:
		for l in create_enum_table(["%s_%s"%(e.name, v) for v in e.values], 'NUM_%sS'%e.name): print l
		print ""

	for e in network.Flags:
		for l in create_flags_table(["%s_%s" % (e.name, v) for v in e.values]): print l
		print ""
		
	for l in create_enum_table(["NETOBJ_INVALID"]+[o.enum_name for o in network.Objects], "NUM_NETOBJTYPES"): print l
	print ""
	for l in create_enum_table(["NETMSG_INVALID"]+[o.enum_name for o in network.Messages], "NUM_NETMSGTYPES"): print l
	print ""
		
	for item in network.Objects + network.Messages:
		for line in item.emit_declaration():
			print line
		print ""
		
	EmitEnum(["SOUND_%s"%i.name.value.upper() for i in content.container.sounds.items], "NUM_SOUNDS")
	EmitEnum(["WEAPON_%s"%i.name.value.upper() for i in content.container.weapons.id.items], "NUM_WEAPONS")

	print "int netobj_validate(int type, void *data, int size);"
	print "const char *netobj_get_name(int type);"
	print "int netobj_get_size(int type);"
	print "void *netmsg_secure_unpack(int type);"
	print "const char *netmsg_get_name(int type);"
	print "const char *netmsg_failed_on();"
	print "int netobj_num_corrections();"
	print "const char *netobj_corrected_on();"
	print "#endif // FILE_G_PROTOCOL_H"
	

if gen_network_source:
	# create names
	lines = []
	
	lines += ['#include <engine/e_common_interface.h>']
	lines += ['#include "g_protocol.hpp"']

	lines += ['const char *msg_failed_on = "";']
	lines += ['const char *obj_corrected_on = "";']
	lines += ['static int num_corrections = 0;']
	lines += ['int netobj_num_corrections() { return num_corrections; }']
	lines += ['const char *netobj_corrected_on() { return obj_corrected_on; }']
	lines += ['const char *netmsg_failed_on() { return msg_failed_on; }']
	lines += ['const int max_int = 0x7fffffff;']

	lines += ['static int netobj_clamp_int(const char *error_msg, int v, int min, int max)']
	lines += ['{']
	lines += ['\tif(v<min) { obj_corrected_on = error_msg; num_corrections++; return min; }']
	lines += ['\tif(v>max) { obj_corrected_on = error_msg; num_corrections++; return max; }']
	lines += ['\treturn v;']
	lines += ['}']

	lines += ["static const char *netobj_names[] = {"]
	lines += ['\t"invalid",']
	lines += ['\t"%s",' % o.name for o in network.Objects]
	lines += ['\t""', "};", ""]

	lines += ["static int netobj_sizes[] = {"]
	lines += ['\t0,']
	lines += ['\tsizeof(%s),' % o.struct_name for o in network.Objects]
	lines += ['\t0', "};", ""]
	for l in lines:
		print l

	for item in network.Objects:
		for line in item.emit_validate():
			print line
		print ""

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
	
	lines += ['const char *netobj_get_name(int type)']
	lines += ['{']
	lines += ['\tif(type < 0 || type >= NUM_NETOBJTYPES) return "(out of range)";']
	lines += ['\treturn netobj_names[type];']
	lines += ['};']
	lines += ['']

	lines += ['int netobj_get_size(int type)']
	lines += ['{']
	lines += ['\tif(type < 0 || type >= NUM_NETOBJTYPES) return 0;']
	lines += ['\treturn netobj_sizes[type];']
	lines += ['};']
	lines += ['']
	
	for item in network.Messages:
		for line in item.emit_unpack():
			print line
		print ""

	lines += ['static void *secure_unpack_invalid() { return 0; }']
	lines += ['typedef void *(*SECUREUNPACKFUNC)();']
	lines += ['static SECUREUNPACKFUNC secure_unpack_funcs[] = {']
	lines += ['\tsecure_unpack_invalid,']
	for msg in network.Messages:
		lines += ['\tsecure_unpack_%s,' % msg.name]
	lines += ['\t0x0']
	lines += ['};']
	
	#
	lines += ['void *netmsg_secure_unpack(int type)']
	lines += ['{']
	lines += ['\tvoid *msg;']
	lines += ['\tmsg_failed_on = "";']
	lines += ['\tif(type < 0 || type >= NUM_NETMSGTYPES) { msg_failed_on = "(type out of range)"; return 0; }']
	lines += ['\tmsg = secure_unpack_funcs[type]();']
	lines += ['\tif(msg_unpack_error()) { msg_failed_on = "(unpack error)"; return 0; }']
	lines += ['\treturn msg;']
	lines += ['};']
	lines += ['']

	lines += ['static const char *message_names[] = {']
	lines += ['\t"invalid",']
	for msg in network.Messages:
		lines += ['\t"%s",' % msg.name]
	lines += ['\t""']
	lines += ['};']
	lines += ['']

	lines += ['const char *netmsg_get_name(int type)']
	lines += ['{']
	lines += ['\tif(type < 0 || type >= NUM_NETMSGTYPES) return "(out of range)";']
	lines += ['\treturn message_names[type];']
	lines += ['};']
	lines += ['']

	for l in lines:
		print l
	
if gen_client_content_header or gen_server_content_header:
	print "#endif"
