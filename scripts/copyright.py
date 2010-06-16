import imp, sys, os
imp.load_source("_compatibility", "../datasrc/_compatibility.py")
import _compatibility

notice = {"simple": b"// copyright (c) 2007 magnus auvinen, see licence.txt for more info\n", "extended": b"/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */\n"}
exclude = ["../src%sengine%sexternal" % (os.sep, os.sep), "../src%sosxlaunch" % os.sep]
updated_files = 0

def fix_copyright_notice(filename, type):
	global updated_files
	f = open(filename, "rb")
	lines = f.readlines()
	f.close()
	
	if len(lines)>0 and (lines[0].decode().lstrip()[:2]=="//" or lines[0].decode().lstrip()[:2]=="/*" and lines[0].decode().rstrip()[-2:]=="*/") and "copyright" in lines[0].decode():
		if lines[0] == notice[type]:
			return;
		lines[0] = notice[type]
	else:
		lines = [notice[type]] + lines
	open(filename, "wb").writelines(lines)
	updated_files += 1
	
skip = False
for root, dirs, files in os.walk("../src"):
	for excluding in exclude:
		if root[:len(excluding)] == excluding:
			skip = True
			break
	if skip == True:
		skip = False
		continue
	for name in files:
		filename = os.path.join(root, name)
		
		# TODO: Scanning the source of .h files to make decision of the notice
		if ".h" == filename[-2:] or ".cpp" == filename[-4:]:
			type = "simple"
		elif ".c" == filename[-2:]:
			type = "extended"
		else:
			continue
		
		fix_copyright_notice(filename, type)
grammar = "file"
if updated_files != 1:
	grammar += "s"
print("*** updated %d %s ***" % (updated_files, grammar))
_compatibility._input("Press enter to exit\n")
