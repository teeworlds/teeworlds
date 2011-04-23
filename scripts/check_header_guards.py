import os


PATH = "../src/"


def check_file(filename):
	file = open(filename)
	while 1:
		line = file.readline()
		if len(line) == 0:
			break
		if line[0] == "/" or line[0] == "*" or line[0] == "\r" or line[0] == "\n" or line[0] == "\t":
			continue
		if line[:7] == "#ifndef":
			hg = "#ifndef " + ("_".join(filename.split(PATH)[1].split("/"))[:-2]).upper() + "_H"
			if line[:-1] != hg:
				print "Wrong header guard in " + filename
		else:
			print "Missing header guard in " + filename
		break
	file.close()



def check_dir(dir):
	list = os.listdir(dir)
	for file in list:
		if os.path.isdir(dir+file):
			if file != "external" and file != "generated":
				check_dir(dir+file+"/")
		elif file[-2:] == ".h" and file != "keynames.h":
			check_file(dir+file)

check_dir(PATH)
