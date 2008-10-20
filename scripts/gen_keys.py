import sys, os

# load header
glfw = "src/engine/external/glfw/include/GL/glfw.h"
lines = [line.strip() for line in file(glfw).readlines()]

# genereate keys.h file
f = file("src/engine/e_keys.h", "w")

keynames_sdl = []
for i in range(0, 512):
	keynames_sdl += ["&%d"%i]
	
keynames = {}
KEY_MOUSE_FIRST = 256+128
KEY_MOUSE_WHEEL_DOWN = KEY_MOUSE_FIRST-2
KEY_MOUSE_WHEEL_UP = KEY_MOUSE_FIRST-1

print >>f, "#ifndef ENGINE_KEYS_H"
print >>f, "#define ENGINE_KEYS_H"
print >>f, '/* AUTO GENERATED! DO NOT EDIT MANUALLY! */'
print >>f, "enum"
print >>f, "{"
print >>f, "#ifdef CONFIG_NO_SDL"

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
print >>f, "\tKEY_MOUSE_WHEEL_DOWN = %d," % (KEY_MOUSE_WHEEL_DOWN)
print >>f, "\tKEY_MOUSE_WHEEL_UP = %d," % (KEY_MOUSE_WHEEL_UP)
keynames[KEY_MOUSE_WHEEL_DOWN] = 'KEY_MOUSE_WHEEL_DOWN'
keynames[KEY_MOUSE_WHEEL_UP] = 'KEY_MOUSE_WHEEL_UP'

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

print >>f, "#else"

highestid = 0
for line in open("scripts/SDL_keysym.h"):
	l = line.strip().split("=")
	if len(l) == 2 and "SDLK_" in line:
		key = l[0].strip().replace("SDLK_", "KEY_")
		value = int(l[1].split(",")[0].strip())
		print >>f, "\t%s = %d,"%(key, value)
		
		keynames_sdl[value] = key.replace("KEY_", "").lower()
		
		if value > highestid:
			highestid =value

print >>f, "\tKEY_MOUSE_1 = %d,"%(highestid+1); keynames_sdl[highestid+1] = "mouse1"
print >>f, "\tKEY_MOUSE_2 = %d,"%(highestid+2); keynames_sdl[highestid+2] = "mouse2"
print >>f, "\tKEY_MOUSE_3 = %d,"%(highestid+3); keynames_sdl[highestid+3] = "mouse3"
print >>f, "\tKEY_MOUSE_4 = %d,"%(highestid+4); keynames_sdl[highestid+4] = "mouse4"
print >>f, "\tKEY_MOUSE_5 = %d,"%(highestid+5); keynames_sdl[highestid+5] = "mouse5"
print >>f, "\tKEY_MOUSE_6 = %d,"%(highestid+6); keynames_sdl[highestid+6] = "mouse6"
print >>f, "\tKEY_MOUSE_7 = %d,"%(highestid+7); keynames_sdl[highestid+7] = "mouse7"
print >>f, "\tKEY_MOUSE_8 = %d,"%(highestid+8); keynames_sdl[highestid+8] = "mouse8"
print >>f, "\tKEY_MOUSE_WHEEL_UP = %d,"%(highestid+9); keynames_sdl[highestid+9] = "mousewheelup"
print >>f, "\tKEY_MOUSE_WHEEL_DOWN = %d,"%(highestid+10); keynames_sdl[highestid+10] = "mousewheeldown"
print >>f, "\tKEY_LAST,"

print >>f, "\tKEY_DEL=KEY_DELETE,"
print >>f, "\tKEY_ENTER=KEY_RETURN,"
print >>f, "\tKEY_KP_SUBTRACT=KEY_KP_MINUS,"
print >>f, "\tKEY_KP_ADD=KEY_KP_PLUS,"
print >>f, "\tKEY_ESC=KEY_ESCAPE"

print >>f, "#endif"
print >>f, "};"
print >>f, ""
print >>f, "#endif"


# generate keynames.c file
f = file("src/engine/e_keynames.c", "w")
print >>f, '/* AUTO GENERATED! DO NOT EDIT MANUALLY! */'
print >>f, ''
print >>f, "#include <string.h>"
print >>f, ""
print >>f, "static const char key_strings[512][16] ="
print >>f, "{"
print >>f, "#ifdef CONFIG_NO_SDL"
for i in range(0, 512):
	n = "&%d"%i
	if i >= 48 and i <= 57 or i >= 65 and i <= 90:
		n = chr(i).lower()
	elif i in keynames:
		n = keynames[i][4:].lower().replace("_", "")
	print >>f, "\t\"%s\"," % n
print >>f, "#else"
print >>f, ""
for n in keynames_sdl:
	print >>f, '\t"%s",'%n

print >>f, "#endif"
print >>f, "};"
print >>f, ""
print >>f, "const char *inp_key_name(int k) { if (k >= 0 && k < 512) return key_strings[k]; else return key_strings[0]; }"
print >>f, "int inp_key_code(const char *key_name) { int i; if (!strcmp(key_name, \"-?-\")) return -1; else for (i = 0; i < 512; i++) if (!strcmp(key_strings[i], key_name)) return i; return -1; }"
print >>f, ""

f.close()

