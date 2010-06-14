import sys

def _import(library):
	if sys.version_info[0] == 2:
		return library[0]
	if sys.version_info[0] == 3:
		return library[1]

def _ord(character):
	if sys.version_info[0] == 2:
		return ord(character)
	if sys.version_info[0] == 3:
		return character

def _xrange(start = 0, stop = 0, step = 1):
	if sys.version_info[0] == 2:
		return xrange(start, stop, step)
	if sys.version_info[0] == 3:
		return range(start, stop, step)
