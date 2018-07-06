#!/usr/bin/env python3
import os
import sys

os.chdir(os.path.dirname(os.path.realpath(sys.argv[0])) + "/..")

PATH = "src/"

def check_file(filename):
	error = False
	with open(filename) as file:
		for line in file:
			if len(line) == 0:
				break
			if line[0] == "/" or line[0] == "*" or line[0] == "\r" or line[0] == "\n" or line[0] == "\t":
				continue
			if line.startswith("#ifndef"):
				hg = "#ifndef " + ("_".join(filename.split(PATH)[1].split("/"))[:-2]).upper() + "_H"
				if line[:-1] != hg:
					error = True
					print("Wrong header guard in {}".format(filename))
			else:
				error = True
				print("Missing header guard in {}".format(filename))
			break
	return error

def check_dir(dir):
	errors = 0
	list = os.listdir(dir)
	for file in list:
		path = dir + file
		if os.path.isdir(path):
			if file != "external" and file != "generated":
				errors += check_dir(path + "/")
		elif file.endswith(".h") and file != "keynames.h":
			errors += check_file(path)
	return errors

if __name__ == '__main__':
	sys.exit(int(check_dir(PATH) != 0))
