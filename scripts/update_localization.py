import os, re
from os.path import dirname, normpath, realpath

file_dir = dirname(realpath(__file__))
file_dir = file_dir.replace("\\", "/") # for windows
match = re.search('(.*)/scripts$', file_dir)
if match != None:
	os.chdir(normpath(match.group(1)))

source_exts = [".c", ".cpp", ".h"]
content_author = ""

def parse_source():
	stringtable = {}
	def process_line(line):
		if 'Localize("'.encode() in line:
			fields = line.split('Localize("'.encode(), 1)[1].split('"'.encode(), 1)
			stringtable[fields[0]] = ""
			process_line(fields[1])

	for root, dirs, files in os.walk("src"):
		for name in files:
			filename = os.path.join(root, name)
			
			if os.sep + "external" + os.sep in filename:
				continue
			
			if filename[-2:] in source_exts or filename[-4:] in source_exts:
				f = open(filename, "rb")
				for line in f:
					process_line(line)
				f.close()

	return stringtable

def load_languagefile(filename):
	f = open(filename, "rb")
	lines = f.readlines()
	f.close()

	stringtable = {}
	authorpart = 0
	global content_author

	for i in range(0, len(lines)-1):
		if authorpart == 0 and "\"authors\":".encode() in lines[i]:
			authorpart = 1
			content_author = lines[i]
		elif authorpart == 1:
			if  "\"translated strings\":".encode() in lines[i]:
				authorpart = 2
			else:
				content_author += lines[i]
		elif "\"or\":".encode() in lines[i]:
				stringtable[lines[i].strip()[7:-2]] = lines[i+1].strip()[7:-1]

	return stringtable

def generate_languagefile(outputfilename, srctable, loctable):
	f = open(outputfilename, "wb")

	num_items = 0
	new_items = 0
	old_items = 0

	srctable_keys = []
	for key in srctable:
		srctable_keys.append(key)
	srctable_keys = sorted(srctable_keys, key=lambda s: s.lower())

	content = content_author

	content += "\"translated strings\": [\n".encode()
	for k in srctable_keys:
		if k in loctable and len(loctable[k]):
			if not num_items == 0:
				content += ",\n".encode()
			content += "\t{\n\t\t\"or\": \"".encode() + k + "\",\n\t\t\"tr\": \"".encode() + loctable[k] + "\"\n\t}".encode()
			num_items += 1
	content += "],\n".encode()

	content += "\"needs translation\": [\n".encode()
	for k in srctable_keys:
		if not k in loctable or len(loctable[k]) == 0:
			if not new_items == 0:
				content += ",\n".encode()
			content += "\t{\n\t\t\"or\": \"".encode() + k + "\",\n\t\t\"tr\": \"\"\n\t}".encode()
			num_items += 1
			new_items += 1
	content += "],\n".encode()

	loctable_keys = []
	for key in loctable:
		loctable_keys.append(key)
	loctable_keys = sorted(loctable_keys, key=lambda s: s.lower())

	content += "\"old translations\": [\n".encode()
	for k in loctable_keys:
		if not k in srctable:
			if not old_items == 0:
				content += ",\n".encode()
			content += "\t{\n\t\t\"or\": \"".encode() + k + "\",\n\t\t\"tr\": \"".encode() + loctable[k] + "\"\n\t}".encode()
			num_items += 1
			old_items += 1
	content += "]\n}\n".encode()

	f.write(content)
	f.close()
	print("%-40s %8d %8d %8d" % (outputfilename, num_items, new_items, old_items))

srctable = parse_source()

print("%-40s %8s %8s %8s" % ("filename", "total", "new", "old"))

for filename in os.listdir("data/languages"):
	if not filename[-5:] == ".json" or filename == "index.json":
		continue

	filename = "data/languages/" + filename
	generate_languagefile(filename, srctable, load_languagefile(filename))
