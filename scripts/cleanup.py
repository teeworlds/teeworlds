import os, sys, string

for (root,dirs,files) in os.walk("src"):
	for filename in files:
		filename = os.path.join(root, filename)
		if "/." in filename or "/external/" in filename or "/base/" in filename or "/generated/" in filename:
			continue

		if not (".hpp" in filename or ".h" in filename or ".cpp" in filename):
			continue
		
		f = open(filename, "r")
		new_file = []
		for line in f:
			new_file.append(line.rstrip())
		f.close()

		f = open(filename, "w")
		f.write('\n'.join(new_file) + '\n')
		f.close()

