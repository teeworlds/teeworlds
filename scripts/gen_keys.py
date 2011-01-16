import sys, os

# genereate keys.h file
f = file("src/engine/keys.h", "w")

keynames = []
for i in range(0, 512):
	keynames += ["&%d"%i]

print >>f, "#ifndef ENGINE_KEYS_H"
print >>f, "#define ENGINE_KEYS_H"
print >>f, '/* AUTO GENERATED! DO NOT EDIT MANUALLY! */'
print >>f, "enum"
print >>f, "{"

highestid = 0
for line in open("scripts/SDL_keysym.h"):
	l = line.strip().split("=")
	if len(l) == 2 and "SDLK_" in line:
		key = l[0].strip().replace("SDLK_", "KEY_")
		value = int(l[1].split(",")[0].strip())
		print >>f, "\t%s = %d,"%(key, value)
		
		keynames[value] = key.replace("KEY_", "").lower()
		
		if value > highestid:
			highestid =value

print >>f, "\tKEY_MOUSE_1 = %d,"%(highestid+1); keynames[highestid+1] = "mouse1"
print >>f, "\tKEY_MOUSE_2 = %d,"%(highestid+2); keynames[highestid+2] = "mouse2"
print >>f, "\tKEY_MOUSE_3 = %d,"%(highestid+3); keynames[highestid+3] = "mouse3"
print >>f, "\tKEY_MOUSE_4 = %d,"%(highestid+4); keynames[highestid+4] = "mouse4"
print >>f, "\tKEY_MOUSE_5 = %d,"%(highestid+5); keynames[highestid+5] = "mouse5"
print >>f, "\tKEY_MOUSE_6 = %d,"%(highestid+6); keynames[highestid+6] = "mouse6"
print >>f, "\tKEY_MOUSE_7 = %d,"%(highestid+7); keynames[highestid+7] = "mouse7"
print >>f, "\tKEY_MOUSE_8 = %d,"%(highestid+8); keynames[highestid+8] = "mouse8"
print >>f, "\tKEY_MOUSE_WHEEL_UP = %d,"%(highestid+9); keynames[highestid+9] = "mousewheelup"
print >>f, "\tKEY_MOUSE_WHEEL_DOWN = %d,"%(highestid+10); keynames[highestid+10] = "mousewheeldown"
print >>f, "\tKEY_LAST,"

print >>f, "};"
print >>f, ""
print >>f, "#endif"

# generate keynames.c file
f = file("src/engine/client/keynames.h", "w")
print >>f, '/* AUTO GENERATED! DO NOT EDIT MANUALLY! */'
print >>f, ''
print >>f, '#ifndef KEYS_INCLUDE'
print >>f, '#error do not include this header!'
print >>f, '#endif'
print >>f, ''
print >>f, "#include <string.h>"
print >>f, ""
print >>f, "const char g_aaKeyStrings[512][16] ="
print >>f, "{"
for n in keynames:
	print >>f, '\t"%s",'%n

print >>f, "};"
print >>f, ""

f.close()

