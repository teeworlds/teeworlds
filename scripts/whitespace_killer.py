#!/usr/bin/env python
## Programm to clean whitespaces from programm code - by XXLTomate ##

import sys, os

class Tabcleaner():
	def __init__(self, args):
		self.args = args
		self.hardClean = False
		self.simuClean = False
		self.filesCleaned = 0
		self.linesCleaned = 0
		self.charsCleaned = 0

	def run(self):
		if len(self.args) == 1:
			self.printHelp()
		for arg in self.args[1:]:
			if arg == "--help":
				self.printHelp()
			if arg == "--hard":
				self.hardClean = True
				continue
			if arg == "--simu":
				self.simuClean = True
				continue
			if arg[-1] == '/':
				arg = arg[:-1]
			self.parseFiles(arg)

		self.printReport()

	def printHelp(self):
		print "========"
		print "Programm to clean the tabs and spaces from unused codelines."
		print "Syntax: %s [option] file/dir [file/dir [...]]" %self.args[0]
		print "Options:"
		print "--help:   shows this help"
		print "--hard:   removes unused whitespaces also behind the codelines"
		print "--simu:   only simulate, don't write back to disk"
		quit()

	def parseFiles(self, path):
		if not os.path.exists(path):
			print "File or directory %s not found" %path
		else:
			if os.path.isfile(path):
				self.clean(path)
			elif os.path.isdir(path):
				for newdir in os.listdir(path):
					self.parseFiles("%s/%s"  %(path, newdir))
			else:
				print "This could be a link: %s" %path

	def clean(self, file):
		#Open file and read content
		fileHandler = open(file, 'r')
		tmpFile = fileHandler.readlines()
		fileHandler.close()

		fileChanged = False
		tmpString = ""
		fileLinesCleaned = 0
		fileCharsCleaned = 0
		if self.hardClean:
			for line in tmpFile:
				lineChanged = False
				tmpLine = ""
				newLine = []
				for i in range(len(line)):
					newLine.append(line[i])

				newLine.reverse()
				for i in range(len(newLine)):
					if not (newLine[i] == '\t' or newLine[i]  == ' ' or  newLine[i]  == '\n'):
						break
					else:
						if not newLine[i] == '\n':
							newLine[i] = ''
							fileChanged = True
							lineChanged = True
							fileCharsCleaned += 1

				newLine.reverse()
				for char in newLine:
					tmpLine += char

				if lineChanged:
					fileLinesCleaned += 1

				tmpString += tmpLine
		else:
			for line in tmpFile:
				for char in line:
					#If not a whitespace char break
					if not (char == '\t' or char  == ' ' or  char  == '\n'):
						break
					else:
						if char  == '\n':
							if len(line) > 1:
								fileCharsCleaned += len(line) - 1
								fileLinesCleaned += 1
								fileChanged = True
								line = "\n"
				#Add lines together
				tmpString += line

		#Write back to file
		if fileChanged:
			if not self.simuClean:
				fileHandler = open(file, 'w')
				fileHandler.write(tmpString)
				fileHandler.close()
			self.charsCleaned += fileCharsCleaned
			self.linesCleaned += fileLinesCleaned
			self.filesCleaned += 1
			print "%s: %i chars in %i lines" %(file, fileCharsCleaned, fileLinesCleaned)

	def printReport(self):
		print "========"
		if not self.charsCleaned and self.simuClean:
			print "Nothing to clean :-)"
		elif not self.charsCleaned:
			print "Nothing cleaned :-)"
		elif self.simuClean:
			print "%i chars in %i lines in %s files would be cleaned!" %(self.charsCleaned, self.linesCleaned, self.filesCleaned)
		else:
			print "%i chars in %i lines in %s files cleaned!" %(self.charsCleaned, self.linesCleaned, self.filesCleaned)


if __name__ == "__main__":
	Tabcleaner(sys.argv).run()