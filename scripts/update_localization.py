import sys, os

source_exts = [".c", ".cpp", ".h", ".hpp"]

def parse_source():
	stringtable = {}
	def process_line(line):
		if 'localize("' in line:
			fields = line.split('localize("', 2)[1].split('"', 2)
			stringtable[fields[0]] = ""
			process_line(fields[1])

	for root, dirs, files in os.walk("src"):
		for name in files:
			filename = os.path.join(root, name)
			
			if os.sep + "external" + os.sep in filename:
				continue
			
			if filename[-2:] in source_exts or filename[-4:] in source_exts:
				for line in file(filename):
					process_line(line)
	
	return stringtable

def load_languagefile(filename):
	f = file(filename)
	lines = f.readlines()
	f.close()
	stringtable = {}

	for i in xrange(0, len(lines)-1):
		l = lines[i].strip()
		if len(l) and not l[0] == '=' and not l[0] == '#':
			stringtable[l] = lines[i+1][3:].strip()
			
	return stringtable


def generate_languagefile(outputfilename, srctable, loctable):
	tmpfilename = outputfilename[:-1]+"_"
	f = file(tmpfilename, "w")

	num_items = 0
	new_items = 0
	old_items = 0

	srctable_keys = srctable.keys()
	srctable_keys.sort()

	print >>f, ""
	print >>f, "##### translated strings #####"
	print >>f, ""
	for k in srctable_keys:
		if k in loctable and len(loctable[k]):
			print >>f, k
			print >>f, "==", loctable[k]
			print >>f, ""
			num_items += 1


	print >>f, "##### needs translation ####"
	print >>f,  ""
	for k in srctable_keys:
		if not k in loctable or len(loctable[k]) == 0:
			print >>f, k
			print >>f, "==", srctable[k]
			print >>f, ""
			num_items += 1
			new_items += 1

	print >>f, "##### old translations ####"
	print >>f, ""
	for k in loctable:
		if not k in srctable:
			print >>f, k
			print >>f, "==", loctable[k]
			print >>f, ""
			num_items += 1
			old_items += 1

	print "%-40s %8s %8s %8s" % ("filename", "total", "new", "old")
	print "%-40s %8d %8d %8d" % (outputfilename, num_items, new_items, old_items)
	f.close()
	
	os.rename(tmpfilename, outputfilename)

srctable = parse_source()

for filename in os.listdir("data/languages"):
	if not ".txt" in filename:
		continue
		
	filename = "data/languages/" + filename
	generate_languagefile(filename, srctable, load_languagefile(filename))



