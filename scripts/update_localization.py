import json
import os
import sys

os.chdir(os.path.dirname(os.path.realpath(sys.argv[0])) + "/..")

content_author = ""

format = "{0:40} {1:8} {2:8} {3:8}".format
SOURCE_EXTS = [".c", ".cpp", ".h"]

JSON_KEY_AUTHORS="authors"
JSON_KEY_TRANSL="translated strings"
JSON_KEY_UNTRANSL="needs translation"
JSON_KEY_OLDTRANSL="old translations"
JSON_KEY_OR="or"
JSON_KEY_TR="tr"

def parse_source():
	l10n = []

	def process_line(line):
		if b"Localize(\"" in line:
			fields = line.split(b"Localize(\"", 1)[1].split(b"\"", 1)
			l10n.append(fields[0].decode())
			process_line(fields[1])

	for root, dirs, files in os.walk("src"):
		for name in files:
			filename = os.path.join(root, name)
			
			if os.sep + "external" + os.sep in filename:
				continue

			if os.path.splitext(filename)[1] in SOURCE_EXTS:
				# HACK: Open source as binary file.
				# Necessary some of teeworlds source files
				# aren't utf-8 yet for some reason
				for line in open(filename, 'rb'):
					process_line(line)

	return l10n

def load_languagefile(filename):
	return json.load(open(filename))

def write_languagefile(outputfilename, l10n_src, old_l10n_data):
	result = {}

	result[JSON_KEY_AUTHORS] = old_l10n_data[JSON_KEY_AUTHORS]

	translations = {}
	for type_ in (
		JSON_KEY_OLDTRANSL,
		JSON_KEY_UNTRANSL,
		JSON_KEY_TRANSL,
	):
		translations.update({
			t[JSON_KEY_OR]: t[JSON_KEY_TR]
			for t in old_l10n_data[type_]
			if t[JSON_KEY_TR]
		})

	num_items = set(translations) | set(l10n_src)
	tsl_items = set(translations) & set(l10n_src)
	new_items = set(translations) - set(l10n_src)
	old_items = set(l10n_src) - set(translations)

	def to_transl(set_):
		return [
			{
				JSON_KEY_OR: x,
				JSON_KEY_TR: translations.get(x, ""),
			}
			for x in sorted(set_)
		]

	result[JSON_KEY_TRANSL] = to_transl(tsl_items)
	result[JSON_KEY_UNTRANSL] = to_transl(new_items)
	result[JSON_KEY_OLDTRANSL] = to_transl(old_items)

	json.dump(
		result,
		open(outputfilename, 'w'),
		ensure_ascii=False,
		indent="\t",
		separators=(',', ': '),
		sort_keys=True,
	)

	print(format(outputfilename, len(num_items), len(new_items), len(old_items)))


if __name__ == '__main__':
	l10n_src = parse_source()
	print(format("filename", *(x.rjust(8) for x in ("total", "new", "old"))))
	for filename in os.listdir("data/languages"):
		try:
			if (os.path.splitext(filename)[1] == ".json"
					and filename != "index.json"):
				filename = "data/languages/" + filename
				write_languagefile(filename, l10n_src, load_languagefile(filename))
		except Exception as e:
			print("Failed on {0}, re-raising for traceback".format(filename))
			raise
