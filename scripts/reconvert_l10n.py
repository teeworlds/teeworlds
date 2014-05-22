import json
import polib
import sys

JSON_KEY_TRANSL="translated strings"
JSON_KEY_OR="or"
JSON_KEY_TR="tr"
JSON_KEY_CO="context"

if __name__ == '__main__':
	po = polib.pofile(open(sys.argv[1], encoding='utf-8').read())
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
		open(sys.argv[1] + '.json', 'w', encoding='utf-8'),
		ensure_ascii=False,
		indent="\t",
		separators=(',', ': '),
		sort_keys=True,
	)
