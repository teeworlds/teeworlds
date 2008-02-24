#!/usr/bin/python

import sys
import struct
import os

option_ptrsize = struct.calcsize("P")
option_intsize = struct.calcsize("l")
option_floatsize = struct.calcsize("f")
option_inttype = "long"
option_floattype = "float"

class node:
	def __init__(self):
		self.values = []
		self.children = []
		self.parent = 0
		
	def name(self):
		if len(self.values):
			return self.values[0]
		return ""

	def debug_print(self, level):
		print ("  "*level) + " ".join(self.values),
		if len(self.children):
			print "{"
			for c in self.children:
				c.debug_print(level+1)
			print ("  "*level)+"}"
		else:
			print ""
		
	def debug_root(self):
		for c in self.children:
			c.debug_print(0)

	# TODO: should return list of items in the tree,
	def gather(self, str):
		def recurse(parts, path, node):
			if not len(parts):
				r = {}
				path = path + "." + node.values[0]
				r = [node]
				#print "found", path
				return r
				
			l = []
			for c in node.children:
				if parts[0] == "*" or c.values[0] == parts[0]:
					if len(node.values):
						if len(path):
							l += recurse(parts[1:], path+"."+node.values[0], c)
						else:
							l += recurse(parts[1:], node.values[0], c)
					else:
						l += recurse(parts[1:], path, c)
			return l
		
		parts = str.split(".")
		return recurse(parts, "", self)	
		
	def find_node(self, str):
		parts = str.split(".")
		node = self
		for part in parts:
			if len(part) == 0:
				continue
			if part == "parent":
				node = node.parent
			else:
				found = 0
				for c in node.children:
					if part == c.values[0]:
						node = c
						found = 1
						break
						
		if node == self:
			return
		return node
		
	def get_single(self, str):
		parts = str.split("@")
		index = -1
		if len(parts) == 2:
			index = int(parts[1])
		
		node = self
		if len(parts[0]):
			node = self.find_node(parts[0])
		
		if not node:
			print "failed to get", str
			return Null
		
		if index == -1:
			return node.get_path()[1:]
		return node.values[index]

	def get_path(self):
		if self.parent == 0:
			return ""
		return self.parent.get_path() + "." + self.values[0]

	def get_single_name(self, str):
		return self.get_path()[1:] + "." + str

class parser:
	lines = []
		
	def parse_node(self, this_node):
		while len(self.lines):
			line = self.lines.pop(0)		# grab line
				
			fields = line.strip().split() # TODO: improve this to handle strings with spaces
			if not len(fields):
				continue
			
			if fields[-1] == '{':
				new_node = node()
				new_node.parent = this_node
				new_node.values = fields[:-1]
				this_node.children += [new_node]
				self.parse_node(new_node)
			elif fields[-1] == '}':
				break
			else:
				new_node = node()
				new_node.parent = this_node
				new_node.values = fields
				this_node.children += [new_node]

	def parse_file(self, filename):
		self.lines = file(filename).readlines()
		n = node()
		self.parse_node(n)
		return n
	
def parse_file(filename):
	return parser().parse_file(filename)

class pointer:
	def __init__(self, index, target):
		self.index = index
		self.target = target

class data_constructor:
	def __init__(self):
		self.data = ""
		self.trans = 0
		self.pointers = []
		self.targets = {}
		self.enums = {}

	def get_type(self, s):
		return self.trans.types[s]

	def allocate(self, size):
		index = len(self.data)
		self.data += "\0"*size
		return index

	def add_pointer(self, index, target):
		self.pointers += [pointer(index, target)]

	def add_enum(self, name, value):
		self.enums[name] = value
		
	def get_enum_value(self, name):
		if not name in self.enums:
			print "ERROR: couldn't find enum '%s'" % (name)
		return self.enums[name]
		
	def add_target(self, target, index):
		# TODO: warn about duplicates
		#print "add_target(target='%s' index=%d)" % (target, index)
		self.targets[target] = index
	
	def write(self, index, size, data):
		try:
			self.data = self.data[:index] + data + self.data[index+size:]
		except:
			print "write error:"
			print "\tself.data =", self.data
			print "\tdata =", data
			
	def patch_pointers(self):
		for p in self.pointers:
			if p.target in self.targets:
				i = self.targets[p.target]
				#print "ptr @ %d -> %s -> %d" % (p.index, p.target, i)
				data = struct.pack("P", i)
				self.write(p.index, len(data), data)
			else:
				print "ERROR: couldn't find target '%s' for pointer at %d" % (p.target, p.index)

class type:
	def __init__(self):
		self.name = ""
		
	def size(self):
		pass

class structure:
	def __init__(self):
		self.name = ""
		self.members = []

	def size(self):
		s = 0
		for m in self.members:
			s += m.size()
		return s
	
	def emit_header_code(self, out):
		print >>out, "struct", self.name
		print >>out, "{"
		for m in self.members:
			for l in m.get_code():
				print >>out, "\t" + l
		print >>out, "};"
		print >>out, ""

	def emit_source_code(self, out):
		print >>out, "static void patch_ptr_%s(%s *self, char *base)" % (self.name, self.name)
		print >>out, "{"
		for m in self.members:
			for l in m.get_patch_code("self", "base"):
				print >>out, "\t" + l
		print >>out, "}"
		print >>out, ""

	def emit_data(self, cons, index, src_data):
		#print self.name+":"
		member_index = index
		for m in self.members:
			#print "\t" + m.name
			m.emit_data(cons, member_index, src_data)
			member_index += m.size()

class variable:
	def __init__(self):
		self.expr = ""
		self.type = ""
		self.subtype = ""
	
	def get_code(self):
		return []

	def get_patch_code(self, ptrname, basename):
		return []
	
	def emit_data(self, cons, index, src_data):
		pass

class variable_int(variable):
	def get_code(self):
		return ["%s %s;" % (option_inttype, self.name)]
	def size(self):
		return option_intsize
	def emit_data(self, cons, index, src_data):
		try:
			value = int(self.expr)
		except:
			value = int(src_data.get_single(self.expr))
		#print "int", self.name, "=", value, "@", index
		data = struct.pack("l", value)
		cons.write(index, len(data), data)

class variable_float(variable):
	def get_code(self):
		return ["%s %s;" % (option_floattype, self.name)]
	def size(self):
		return option_floatsize
	def emit_data(self, cons, index, src_data):
		try:
			value = float(self.expr)
		except:
			value = float(src_data.get_single(self.expr))
		#print "int", self.name, "=", value, "@", index
		data = struct.pack("f", value)
		cons.write(index, len(data), data)
		
class variable_string(variable):
	def get_code(self):
		return ["char *%s;" % (self.name)]
	def get_patch_code(self, ptrname, basename):
		return ["patch_ptr((char **)&(%s->%s), %s);" % (ptrname, self.name, basename)]
	def size(self):
		return option_ptrsize
	def emit_data(self, cons, index, src_data):
		string = src_data.get_single(self.expr)
		string = string.strip()[1:-1] # skip " and "

		string_index = cons.allocate(len(string)+1)
		cons.write(string_index, len(string), string)

		data = struct.pack("P", string_index) # TODO: solve this
		cons.write(index, len(data), data)

class variable_ptr(variable):
	def get_code(self):
		return ["%s *%s;" % (self.subtype, self.name)]
	def get_patch_code(self, ptrname, basename):
		return ["patch_ptr((char**)&(%s->%s), %s);" % (ptrname, self.name, basename)]
	def size(self):
		return option_ptrsize
	def emit_data(self, cons, index, src_data):
		target = src_data.get_single(self.expr)
		cons.add_pointer(index, target)

class variable_enum(variable):
	def get_code(self):
		return ["long *%s;" % (self.name)]
	def size(self):
		return option_intsize
	def emit_data(self, cons, index, src_data):
		target = src_data.get_single(self.expr)
		data = struct.pack("l", cons.get_enum_value(target))
		cons.write(index, len(data), data)

class variable_instance(variable):
	def get_code(self):
		return ["%s %s;" % (self.subtype, self.name)]
	def get_patch_code(self, ptrname, basename):
		return ["patch_ptr_%s(&(%s->%s), %s);" % (self.subtype, ptrname, self.name, basename)]
	def size(self):
		return self.translator.types[self.subtype].size()
	def emit_data(self, cons, index, src_data):
		target = src_data.find_node(self.expr)
		translator.types[self.subtype].emit_data(cons, index, target)
		#target = 
		#cons.add_pointer(index, target)

class variable_array(variable):
	def get_code(self):
		return ["long num_%s;" % self.name,
			"%s *%s;" % (self.subtype, self.name)]
	def get_patch_code(self, ptrname, baseptr):
		code = []
		code += ["patch_ptr((char**)&(%s->%s), %s);" % (ptrname, self.name, baseptr)]
		code += ["for(int i = 0; i < %s->num_%s; i++)" % (ptrname, self.name)]
		code += ["\tpatch_ptr_%s(%s->%s+i, %s);" % (self.subtype, ptrname, self.name, baseptr)]
		return code
	def emit_data(self, cons, index, src_data):
		array_data = src_data.gather(self.expr)
		array_type = cons.get_type(self.subtype)
		size = array_type.size()*len(array_data)

		#print "packing array", self.name
		#print "\ttype =", array_type.name
		#print "\tsize =", array_type.size()
		array_index = cons.allocate(size)
		data = struct.pack("lP", len(array_data), array_index) # TODO: solve this
		cons.write(index, len(data), data)

		member_index = array_index
		for node in array_data:
			cons.add_target(node.get_path()[1:], member_index)
			array_type.emit_data(cons, member_index, node)
			member_index += array_type.size()
			#print "array", member_index

	def size(self):
		return option_ptrsize+option_intsize

class const_arrayint:
	def __init__(self):
		self.name = ""
		self.values = []
		
	def emit_header_code(self, out):
		print >>out, "enum"
		print >>out, "{"
		for i in xrange(0, len(self.values)):
			print >>out, "\t%s_%s = %d," % (self.name.upper(), self.values[i].upper(), i)
		
		print >>out, "\tNUM_%sS = %d" % (self.name.upper(), len(self.values))
		print >>out, "};"
		print >>out, ""
		
class translator:
	def __init__(self):
		self.types = {}
		self.structs = []
		self.constants = []
		self.data = 0
		self.srcdata = 0

		self.types["int"] = variable_int()
		self.types["float"] = variable_float()
		self.types["string"] = variable_string()
		self.types["ptr"] = variable_ptr()
		self.types["array"] = variable_array()

	def parse_variable(self, node):
		if len(node.values) != 4:
			print node.values
			raise "error parsing variable"
			
		type = node.values[0]
		subtype = ""
		if type == "int":
			v = variable_int()
		elif type == "enum":
			v = variable_enum()
		elif type == "float":
			v = variable_float()
		elif type == "string":
			v = variable_string()
		else:
			# complex type
			parts = type.split(":")
			if len(parts) != 2:
				raise "can't emit code for variable %s of type %s" % (self.name, self.type)
			elif parts[0] == "ptr":
				subtype = parts[1]
				v = variable_ptr()
			elif parts[0] == "instance":
				subtype = parts[1]
				v = variable_instance()
			elif parts[0] == "array":
				subtype = parts[1]
				v = variable_array()
			else:
				raise "can't emit code for variable %s of type %s" % (self.name, self.type)

		v.translator = self
		v.type = node.values[0]
		v.subtype = subtype
		v.name = node.values[1]
		assignment = node.values[2]
		v.expr = node.values[3]
		if assignment != "=":
			raise "error parsing variable. expected ="
		return v

	def parse_struct(self, node):
		if len(node.values) != 2:
			raise "error parsing struct"
		s = structure()
		s.name = node.values[1]
		
		for statement in node.children:
			s.members += [self.parse_variable(statement)]
		return s
	
	def parse_constant(self, node):
		if len(node.values) != 5:
			print node.values
			raise "error parsing constant"
		
		type = node.values[1]
		name = node.values[2]
		assignment = node.values[3]
		expression = node.values[4]
		
		if assignment != "=":
			print node.values
			raise "error parsing constant"
			
		ints = const_arrayint()
		ints.name = name
		
		items = self.srcdata.gather(expression)
		for c in items:
			ints.values += [c.name()]
		self.constants += [ints]
		
	def parse(self, script, srcdata):
		self.srcdata = srcdata
		for statement in script.children:
			if statement.values[0] == "struct":
				s = self.parse_struct(statement)
				self.structs += [s]
				self.types[s.name] = s
			elif statement.values[0] == "const":
				self.parse_constant(statement)
			else:
				raise "unknown statement:" + statement
				
	def emit_header_code(self, out):
		for c in self.constants:
			c.emit_header_code(out)
		
		for s in self.structs:
			s.emit_header_code(out)
		print >>out, ""
		print >>out, "struct data_container *load_data_from_file(const char *filename);"
		print >>out, "struct data_container *load_data_from_memory(unsigned char *filename);"
		print >>out, ""
		

	def emit_source_code(self, out, header_filename):
		print >>out, '''

#include "%s"
#include <stdio.h>
#include <stdlib.h>

static void patch_ptr(char **ptr, char *base)
{
	*ptr = base+(size_t)(*ptr);
}
''' % header_filename

		for s in self.structs:
			s.emit_source_code(out)
		print >>out, '''

data_container *load_data_from_memory(unsigned char *mem)
{
	if(mem[0] != sizeof(void*))
		return 0;
	if(mem[1] != sizeof(long))
		return 0;
	if(mem[2] != sizeof(float))
		return 0;

	/* patch all pointers */
	data_container *con = (data_container*)(mem + 4);
	patch_ptr_data_container(con, (char *)con);
	return con;
}

data_container *load_data_from_file(const char *filename)
{
	unsigned char *data = 0;
	int size;

	/* open file */
	FILE *f = fopen(filename, "rb");

	/* get size */
	fseek(f, 0, SEEK_END);
	size = ftell(f);
	fseek(f, 0, SEEK_SET);

	/* allocate, read data and close file */
	data = (unsigned char *)malloc(size);
	fread(data, 1, size, f);
	fclose(f);

	return load_data_from_memory(data);
}

'''

	def emit_data(self):
		for s in self.structs:
			if s.name == "data_container":
				#print "found data_container"
				cons = data_constructor()
				cons.trans = self
				i = cons.allocate(s.size())
				s.emit_data(cons, i, self.srcdata)
				cons.patch_pointers()
				header = struct.pack("bbbb", option_ptrsize, option_intsize, option_floatsize, 0)
				return header + cons.data
			
def create_translator(script, srcdata):
	t = translator()
	t.parse(script, srcdata)
	return t
	
def validate(script, validator):
	def validate_values(values, check):
		if not len(check) or check[0] == "*":
			print "too many values"
			return
		p = check[0].split(":")
		type = p[0]
		name = p[1]
		
		# TODO: check type and stuff
	
		# recurse
		if len(values) > 1:
			if not len(check):
				print "unexpected value"
			validate_values(values[1:], check[1:])
		else:
			if len(check) > 1 and check[1] != "*":
				print "to few values"
	
	if len(script.values):
		validate_values(script.values, validator.values)
	
	for child in script.children:
		tag = child.values[0]
		n = validator.find_node("tag:"+tag)
		if not n:
			found = 0
			for vc in validator.children:
				if "ident:" in vc.values[0]:
					validate(child, vc)
					print vc.values[0]
					found = 1
					break
					
			if not found:
				print "error:", tag, "not found"
		else:
			print "tag:"+tag
			validate(child, n)

input_filename = sys.argv[1]
script_filename = sys.argv[2]

output_filename = 0
coutput_filename = 0
header_filename = 0
source_filename = 0
sheader_filename = 0

if sys.argv[3] == '-h':
	header_filename = sys.argv[4]
elif sys.argv[3] == '-s':
	source_filename = sys.argv[4]
	sheader_filename = sys.argv[5]
elif sys.argv[3] == '-d':
	output_filename = sys.argv[4]
elif sys.argv[3] == '-c':
	coutput_filename = sys.argv[4]

srcdata = parse_file(input_filename)
script = parse_file(script_filename)

translator = create_translator(script, srcdata)

if header_filename:
	translator.emit_header_code(file(header_filename, "w"))
if source_filename:
	translator.emit_source_code(file(source_filename, "w"), os.path.basename(sheader_filename))

if output_filename:
	rawdata = translator.emit_data()
	file(output_filename, "wb").write(rawdata)
if coutput_filename:
	i = 0
	rawdata = translator.emit_data()
	f = file(coutput_filename, "w")

	print >>f,"unsigned char internal_data[] = {"
	print >>f,str(ord(rawdata[0])),
	for d in rawdata[1:]:
	    s = ","+str(ord(d))
	    print >>f,s,
	    i += len(s)+1
	
	    if i >= 70:
	        print >>f,""
	        i = 0
	print >>f,""
	print >>f,"};"
	print >>f,""
	f.close()
