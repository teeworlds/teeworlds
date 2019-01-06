import polib
import json
import sys

JSON_KEY_TRANSL="translated strings"
JSON_KEY_AUTHOR="authors"
JSON_KEY_MODIFY="modified by"
JSON_KEY_OR="or"
JSON_KEY_TR="tr"
JSON_KEY_CO="context"

def reconvert(filename, json_filename):
	"""Converts a l10n file.po into a file.json, keeping author info from a previous file.json if possible"""
	po = polib.pofile(open(filename, encoding='utf-8').read())
	translations = []

	for entry in po:
		if entry.msgstr:
			t_entry = {}
			t_entry[JSON_KEY_OR] = entry.msgid
			t_entry[JSON_KEY_TR] = entry.msgstr
			if entry.msgctxt is not None:
				t_entry[JSON_KEY_CO] = entry.msgctxt
			translations.append(t_entry)

	try:
		previous_l10n = json.load(open(json_filename, encoding='utf-8'), strict=False)
		
		# remove tabs from data
		authors = previous_l10n[JSON_KEY_AUTHOR]
		for i, value in enumerate(authors[JSON_KEY_MODIFY]):
			authors[JSON_KEY_MODIFY][i] = value.replace("\t", "    ")

		result = {JSON_KEY_AUTHOR: authors, JSON_KEY_TRANSL: translations}
		print(" extracted author info from " + json_filename)

	except IOError:
		result = {JSON_KEY_AUTHOR: "", JSON_KEY_TRANSL: translations}
		print(" failed to open " + json_filename + ", skipping author info")

	json.dump(
		result,
		open(json_filename, 'w', encoding='utf-8'),
		ensure_ascii=False,
		indent="\t",
		separators=(',', ': '),
		sort_keys=True,
	)

def normalize(filename):
	"""Normalizes a l10n file.po for better version controlling"""
	po = polib.pofile(open(filename).read())
	entries = list(sorted(sorted(po, key=lambda x: x.msgctxt or ""), key=lambda x: x.msgid))
	po.clear()
	for entry in entries:
		po.append(entry)
	po.save(filename)

def decrement_indent(filename):
	"""decrement indent level of file"""
	with open(filename, encoding='utf-8') as f:
		sanitized_json=f.read().replace('\n\t', '\n')	 
	with open(filename, 'w', encoding='utf-8') as f:
		f.write(sanitized_json)

if __name__ == '__main__':
	"""Normalizes then converts a l10n file.po into a file.json, keeping author info from a previous file.json if possible"""
	for filename in sys.argv[1:]:
		assert(len(filename)>len(".po"))
		json_filename = filename[:-3]+".json"
		normalize(filename)
		print(filename + " -> " + filename + " (normalized)")
		reconvert(filename, json_filename)
		decrement_indent(json_filename)
		print(filename + " -> " + json_filename)
