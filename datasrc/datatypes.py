
	
GlobalIdCounter = 0
def GetID():
	global GlobalIdCounter
	GlobalIdCounter += 1
	return GlobalIdCounter
def GetUID():
	return "x%d"%GetID()

class BaseType:
	def __init__(self, type_name):
		self._type_name = type_name
		self._target_name = "INVALID"
		self._id = GetID() # this is used to remeber what order the members have in structures etc
	
	def Identifyer(self): return "x"+str(self._id)
	def TargetName(self): return self._target_name
	def TypeName(self): return self._type_name
	def ID(self): return self._id;
	
	def EmitDeclaration(self, name):
		return ["%s %s;"%(self.TypeName(), name)]
	def EmitPreDefinition(self, target_name):
		self._target_name = target_name
		return []
	def EmitDefinition(self, name):
		return []

class MemberType:
	def __init__(self, name, var):
		self.name = name
		self.var = var
		
class Struct(BaseType):
	def __init__(self, type_name):
		BaseType.__init__(self, type_name)
	def Members(self):
		def sorter(a,b):
			return a.var.ID()-b.var.ID()
		m = []
		for name in self.__dict__:
			if name[0] == "_":
				continue
			m += [MemberType(name, self.__dict__[name])]
		try:
			m.sort(sorter)
		except:
			for v in m:
				print v.name, v.var
			sys.exit(-1)
		return m
		
	def EmitTypeDeclaration(self, name):
		lines = []
		lines += ["struct " + self.TypeName()]
		lines += ["{"]
		for member in self.Members():
			lines += ["\t"+l for l in member.var.EmitDeclaration(member.name)]
		lines += ["};"]
		return lines

	def EmitPreDefinition(self, target_name):
		BaseType.EmitPreDefinition(self, target_name)
		lines = []
		for member in self.Members():
			lines += member.var.EmitPreDefinition(target_name+"."+member.name)
		return lines
	def EmitDefinition(self, name):
		lines = ["/* %s */ {" % self.TargetName()]
		for member in self.Members():
			lines += ["\t" + " ".join(member.var.EmitDefinition("")) + ","]
		lines += ["}"]
		return lines
		
class Array(BaseType):
	def __init__(self, type):
		BaseType.__init__(self, type.TypeName())
		self.type = type
		self.items = []
	def Add(self, instance):
		if instance.TypeName() != self.type.TypeName():
			error("bah")
		self.items += [instance]
	def EmitDeclaration(self, name):
		return ["int num_%s;"%(name),
			"%s *%s;"%(self.TypeName(), name)]
	def EmitPreDefinition(self, target_name):
		BaseType.EmitPreDefinition(self, target_name)

		lines = []
		i = 0
		for item in self.items:
			lines += item.EmitPreDefinition("%s[%d]"%(self.Identifyer(), i))
			i += 1
			
		if len(self.items):
			lines += ["static %s %s[] = {"%(self.TypeName(), self.Identifyer())]
			for item in self.items:
				itemlines = item.EmitDefinition("")
				lines += ["\t" + " ".join(itemlines).replace("\t", " ") + ","]
			lines += ["};"]
		else:
			lines += ["static %s *%s = 0;"%(self.TypeName(), self.Identifyer())]
			
		return lines
	def EmitDefinition(self, name):
		return [str(len(self.items))+","+self.Identifyer()]

# Basic Types

class Int(BaseType):
	def __init__(self, value):
		BaseType.__init__(self, "int")
		self.value = value
	def Set(self, value):
		self.value = value
	def EmitDefinition(self, name):
		return ["%d"%self.value]
		#return ["%d /* %s */"%(self.value, self._target_name)]
		
class Float(BaseType):
	def __init__(self, value):
		BaseType.__init__(self, "float")
		self.value = value
	def Set(self, value):
		self.value = value
	def EmitDefinition(self, name):
		return ["%f"%self.value]
		#return ["%d /* %s */"%(self.value, self._target_name)]
		
class String(BaseType):
	def __init__(self, value):
		BaseType.__init__(self, "const char*")
		self.value = value
	def Set(self, value):
		self.value = value
	def EmitDefinition(self, name):
		return ['"'+self.value+'"']
		
class Pointer(BaseType):
	def __init__(self, type, target):
		BaseType.__init__(self, "%s*"%type().TypeName())
		self.target = target
	def Set(self, target):
		self.target = target
	def EmitDefinition(self, name):
		return ["&"+self.target.TargetName()]

# helper functions

def EmitTypeDeclaration(root):
	for l in root().EmitTypeDeclaration(""):
		print l

def EmitDefinition(root, name):
	for l in root.EmitPreDefinition(name):
		print l
	print "%s %s = " % (root.TypeName(), name)
	for l in root.EmitDefinition(name):
		print l
	print ";"

# Network stuff after this

class Object:
	pass

class Enum():
	def __init__(self, name, values):
		self.name = name
		self.values = values

class Flags():
	def __init__(self, name, values):
		self.name = name
		self.values = values

class NetObject:
	def __init__(self, name, variables):
		l = name.split(":")
		self.name = l[0].lower()
		self.base = ""
		if len(l) > 1:
			self.base = l[1]
		self.base_struct_name = "NETOBJ_%s" % self.base.upper()
		self.struct_name = "NETOBJ_%s" % self.name.upper()
		self.enum_name = "NETOBJTYPE_%s" % self.name.upper()
		self.variables = variables
	def emit_declaration(self):
		if self.base:
			lines = ["struct %s : public %s"%(self.struct_name,self.base_struct_name), "{"]
		else:
			lines = ["struct %s"%self.struct_name, "{"]
		for v in self.variables:
			lines += ["\t"+line for line in v.emit_declaration()]
		lines += ["};"]
		return lines
	def emit_validate(self):
		lines = ["static int validate_%s(void *data, int size)" % self.name]
		lines += ["{"]
		lines += ["\t%s *obj = (%s *)data;"%(self.struct_name, self.struct_name)]
		lines += ["\tif(sizeof(*obj) != size) return -1;"]
		for v in self.variables:
			lines += ["\t"+line for line in v.emit_validate()]
		lines += ["\treturn 0;"]
		lines += ["}"]
		return lines
		

class NetEvent(NetObject):
	def __init__(self, name, variables):
		NetObject.__init__(self, name, variables)
		self.base_struct_name = "NETEVENT_%s" % self.base.upper()
		self.struct_name = "NETEVENT_%s" % self.name.upper()
		self.enum_name = "NETEVENTTYPE_%s" % self.name.upper()

class NetMessage(NetObject):
	def __init__(self, name, variables):
		NetObject.__init__(self, name, variables)
		self.base_struct_name = "NETMSG_%s" % self.base.upper()
		self.struct_name = "NETMSG_%s" % self.name.upper()
		self.enum_name = "NETMSGTYPE_%s" % self.name.upper()
	def emit_unpack(self):
		lines = []
		lines += ["static void *secure_unpack_%s()" % self.name]
		lines += ["{"]
		lines += ["\tstatic %s msg;" % self.struct_name]
		for v in self.variables:
			lines += ["\t"+line for line in v.emit_unpack()]
		for v in self.variables:
			lines += ["\t"+line for line in v.emit_unpack_check()]
		lines += ["\treturn &msg;"]
		lines += ["}"]
		return lines
	def emit_declaration(self):
		extra = []
		extra += ["\tvoid pack(int flags)"]
		extra += ["\t{"]
		extra += ["\t\tmsg_pack_start(%s, flags);"%self.enum_name]
		for v in self.variables:
			extra += ["\t\t"+line for line in v.emit_pack()]
		extra += ["\t\tmsg_pack_end();"]
		extra += ["\t}"]
		
		
		lines = NetObject.emit_declaration(self)
		lines = lines[:-1] + extra + lines[-1:]
		return lines
		

class NetVariable:
	def __init__(self, name):
		self.name = name
	def emit_declaration(self):
		return []
	def emit_validate(self):
		return []
	def emit_pack(self):
		return []
	def emit_unpack(self):
		return []
	def emit_unpack_check(self):
		return []

class NetString(NetVariable):
	def emit_declaration(self):
		return ["const char *%s;"%self.name]
	def emit_unpack(self):
		return ["msg.%s = msg_unpack_string();" % self.name]
	def emit_pack(self):
		return ["msg_pack_string(%s, -1);" % self.name]

class NetIntAny(NetVariable):
	def emit_declaration(self):
		return ["int %s;"%self.name]
	def emit_unpack(self):
		return ["msg.%s = msg_unpack_int();" % self.name]
	def emit_pack(self):
		return ["msg_pack_int(%s);" % self.name]

class NetIntRange(NetIntAny):
	def __init__(self, name, min, max):
		NetIntAny.__init__(self,name)
		self.min = str(min)
		self.max = str(max)
	def emit_validate(self):
		return ["netobj_clamp_int(\"%s\", obj->%s, %s, %s);"%(self.name,self.name, self.min, self.max)]
	def emit_unpack_check(self):
		return ["if(msg.%s < %s || msg.%s > %s) { msg_failed_on = \"%s\"; return 0; }" % (self.name, self.min, self.name, self.max, self.name)]

class NetBool(NetIntRange):
	def __init__(self, name):
		NetIntRange.__init__(self,name,0,1)

class NetTick(NetIntRange):
	def __init__(self, name):
		NetIntRange.__init__(self,name,0,'max_int')
