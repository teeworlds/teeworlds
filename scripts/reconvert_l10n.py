import json
import polib

JSON_KEY_TRANSL="translated strings"
JSON_KEY_OR="or"
JSON_KEY_TR="tr"
JSON_KEY_CO="context"

def reconvert(filename):
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

	result = {JSON_KEY_TRANSL: translations}

	json.dump(
		result,
		open(filename + '.json', 'w', encoding='utf-8'),
		ensure_ascii=False,
		indent="\t",
		separators=(',', ': '),
		sort_keys=True,
	)

if __name__ == '__main__':
	import sys
	for filename in sys.argv[1:]:
		reconvert(filename)
