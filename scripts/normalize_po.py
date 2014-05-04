import polib
import sys

if __name__ == '__main__':
	for filename in sys.argv[1:]:
		po = polib.pofile(open(filename).read())
		entries = list(sorted(sorted(po, key=lambda x: x.msgctxt or ""), key=lambda x: x.msgid))
		po.clear()
		for entry in entries:
			po.append(entry)
		po.save(filename)
