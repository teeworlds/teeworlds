
class Object:
	pass

class Enum():
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

class NetVariable:
	def __init__(self, name):
		self.name = name
	def emit_declaration(self):
		return []
	def emit_validate(self):
		return []

class NetString(NetVariable):
	def emit_declaration(self):
		return ["const char *%s;"%self.name]

class NetIntAny(NetVariable):
	def emit_declaration(self):
		return ["int %s;"%self.name]

class NetIntRange(NetIntAny):
	def __init__(self, name, min, max):
		NetIntAny.__init__(self,name)
		self.min = str(min)
		self.max = str(max)
	def emit_validate(self):
		return ["netobj_clamp_int(obj->%s, %s %s)"%(self.name, self.min, self.max)]

class NetBool(NetIntRange):
	def __init__(self, name):
		NetIntRange.__init__(self,name,0,1)

class NetTick(NetIntRange):
	def __init__(self, name):
		NetIntRange.__init__(self,name,0,'max_int')
