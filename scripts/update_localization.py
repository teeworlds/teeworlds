import os, re, sys
match = re.search("(.*?)/[^/]*?$", sys.argv[0])
if match != None:
	os.chdir(os.getcwd() + "/" + match.group(1))

source_exts = [".c", ".cpp", ".h"]

def parse_source():
	stringtable = {}
	def process_line(line):
		if b'Localize("' in line:
			fields = line.split(b'Localize("', 1)[1].split(b'"', 1)
			stringtable[fields[0]] = ""
			process_line(fields[1])

	for root, dirs, files in os.walk("../src"):
		for name in files:
			filename = os.path.join(root, name)
			
			if os.sep + "external" + os.sep in filename:
				continue
			
			if filename[-2:] in source_exts or filename[-4:] in source_exts:
				for line in open(filename, "rb"):
					process_line(line)

	return stringtable

def load_languagefile(filename):
	f = open(filename, "rb")
	lines = f.readlines()
	f.close()

	stringtable = {}

	for i in range(0, len(lines)-1):
		l = lines[i].strip()
		if len(l) and not l[0:1] == b"=" and not l[0:1] == b"#":
			stringtable[l] = lines[i+1][3:].rstrip()

	return stringtable

def generate_languagefile(outputfilename, srctable, loctable):
	f = open(outputfilename, "wb")

	num_items = 0
	new_items = 0
	old_items = 0

	srctable_keys = []
	for key in srctable:
		srctable_keys.append(key)
	srctable_keys.sort()

	content = b"\n##### translated strings #####\n\n"
	for k in srctable_keys:
		if k in loctable and len(loctable[k]):
			content += k + b"\n== " + loctable[k] + b"\n\n"
			num_items += 1

	content += b"##### needs translation #####\n\n"
	for k in srctable_keys:
		if not k in loctable or len(loctable[k]) == 0:
			content += k + b"\n== \n\n"
			num_items += 1
			new_items += 1

	content += b"##### old translations #####\n\n"
	for k in loctable:
		if not k in srctable:
			content += k + b"\n== " + loctable[k] + b"\n\n"
			num_items += 1
			old_items += 1

	f.write(content)
	f.close()
	print("%-40s %8d %8d %8d" % (outputfilename, num_items, new_items, old_items))

srctable = parse_source()

print("%-40s %8s %8s %8s" % ("filename", "total", "new", "old"))

for filename in os.listdir("../data/languages"):
	if not ".txt" in filename:
		continue
	if filename == "index.txt":
		continue

	filename = "../data/languages/" + filename
	generate_languagefile(filename, srctable, load_languagefile(filename))
