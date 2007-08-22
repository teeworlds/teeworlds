import sys, os

# load header
glfw = "src/engine/external/glfw/include/GL/glfw.h"
lines = [line.strip() for line in file(glfw).readlines()]

# genereate keys.h file
f = file("src/engine/keys.h", "w")

keynames = {}
KEY_MOUSE_FIRST = 256+128

print >>f, "#ifndef ENGINE_KEYS_H"
print >>f, "#define ENGINE_KEYS_H"
print >>f, '/* AUTO GENERATED! DO NOT EDIT MANUALLY! */'
print >>f, ""
print >>f, "enum"
print >>f, "{"

# do keys
for line in lines:
	if "GLFW_KEY" in line:
		l = line.split()
		key = l[1].replace("GLFW_", "").strip()
		value = l[2].replace("GLFW_","").strip()

		# ignore special last key
		if key == "KEY_LAST":
			continue

		# print to file					
		print >>f, "\t%s = %s," % (key.upper(), value)
		
			# add to keynames
		exec("%s = %s" % (key, value))
		exec("keynames[%s] = '%s'" % (value, key))

# do mouse buttons
print >>f, "\tKEY_MOUSE_FIRST = %d," % KEY_MOUSE_FIRST
for line in lines:
	if "GLFW_MOUSE" in line and not "CURSOR" in line:
		l = line.split()
		key = l[1].replace("GLFW_", "").strip()
		value = l[2].replace("GLFW_","").strip()
		
		if len(key) == len("MOUSE_BUTTON_X"): # only match MOUSE_X
			key = "KEY_MOUSE_" + key[-1]
			value = "KEY_MOUSE_FIRST+%s"% (int(key[-1])-1)
			
			# print to file
			print >>f, "\t%s = %s," % (key, value)
			
			# add to keynames
			exec("%s = %s" % (key, value))
			exec("keynames[%s] = '%s'" % (value, key))

print >>f, "\tKEY_LAST"
print >>f, "};"
print >>f, ""
print >>f, "#endif"
f.close()


# generate keynames.c file
f = file("src/engine/keynames.c", "w")
print >>f, '/* AUTO GENERATED! DO NOT EDIT MANUALLY! */'
print >>f, ''
print >>f, "#include <string.h>"
print >>f, ""
print >>f, "static const char key_strings[512][16] ="
print >>f, "{"
for i in range(0, 512):
	n = "#%d"%i
	if i >= 48 and i <= 57 or i >= 65 and i <= 90:
		n = chr(i).lower()
	elif i in keynames:
		n = keynames[i][4:].lower().replace("_", "")
	print >>f, "\t\"%s\"," % n

print >>f, "};"
print >>f, ""
print >>f, "const char *inp_key_name(int k) { if (k >= 0 && k < 512) return key_strings[k]; else return key_strings[0]; }"
print >>f, "int inp_key_code(const char *key_name) { int i; if (!strcmp(key_name, \"-?-\")) return -1; else for (i = 0; i < 512; i++) if (!strcmp(key_strings[i], key_name)) return i; return -1; }"
print >>f, ""

f.close()

