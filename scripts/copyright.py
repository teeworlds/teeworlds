import sys, os, re
match = re.search("(.*?)/[^/]*?$", sys.argv[0])
if match != None:
	os.chdir(os.getcwd() + "/" + match.group(1))

notice = {"simple": [b"// (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information.\n", b"// If you are missing that file, acquire a complete release at teeworlds.com.\n"], "extended": [b"/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */\n", b"/* If you are missing that file, acquire a complete release at teeworlds.com.                */\n"]}
exclude = ["../src%sengine%sexternal" % (os.sep, os.sep), "../src%sosxlaunch" % os.sep]
updated_files = 0

def fix_copyright_notice(filename, type):
	global updated_files
	f = open(filename, "rb")
	lines = f.readlines()
	f.close()

	i = 0
	length_lines = len(lines)
	if length_lines > 0:
		while i <= length_lines and (lines[i].decode("utf-8").lstrip()[:2] == "//" or lines[i].decode("utf-8").lstrip()[:2] == "/*" and lines[i].decode("utf-8").rstrip()[-2:] == "*/") and ("Magnus" in lines[i].decode("utf-8") or "magnus" in lines[i].decode("utf-8") or "Auvinen" in lines[i].decode("utf-8") or "auvinen" in lines[i].decode("utf-8") or "license" in lines[i].decode("utf-8") or "teeworlds" in lines[i].decode("utf-8")):
			i += 1
	length_notice = len(notice[type])
	if i > 0:
		j = 0
		while lines[j] == notice[type][j]:
			j += 1
			if j == length_notice:
				return
		k = j
		j = 0
		while j < length_notice -1 - k:
			lines = [notice[type][j]] + lines
			j += 1
		while j < length_notice:
			lines[j] = notice[type][j]
			j += 1
	if length_lines == 0 or i == 0:
		j = length_notice - 1
		while j >= 0:
			lines = [notice[type][j]] + lines
			j -= 1
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

output = "file"
if updated_files != 1:
	output += "s"
print("*** updated %d %s ***" % (updated_files, output))
