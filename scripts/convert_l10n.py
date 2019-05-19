import json
import polib
import os
import re
import time
import sys

from collections import defaultdict

os.chdir(os.path.dirname(os.path.realpath(sys.argv[0])) + "/..")

format = "{0:40} {1:8} {2:8} {3:8}".format
SOURCE_EXTS = [".c", ".cpp", ".h"]

JSON_KEY_AUTHORS="authors"
JSON_KEY_TRANSL="translated strings"
JSON_KEY_UNTRANSL="needs translation"
JSON_KEY_OLDTRANSL="old translations"
JSON_KEY_OR="or"
JSON_KEY_TR="tr"

SOURCE_LOCALIZE_RE=re.compile(br'Localize\("(?P<str>([^"\\]|\\.)*)"(, ?"(?P<ctxt>([^"\\]|\\.)*)")?\)')

def parse_source():
	l10n = defaultdict(lambda: [])

	def process_line(line, filename, lineno):
		for match in SOURCE_LOCALIZE_RE.finditer(line):
			str_ = match.group('str').decode()
			ctxt = match.group('ctxt')
			if ctxt is not None:
				ctxt = ctxt.decode()
			l10n[(str_, ctxt)].append((filename, lineno))

	for root, dirs, files in os.walk("src"):
		for name in files:
			filename = os.path.join(root, name)
			
			if os.sep + "external" + os.sep in filename:
				continue

			if os.path.splitext(filename)[1] in SOURCE_EXTS:
				# HACK: Open source as binary file.
				# Necessary some of teeworlds source files
				# aren't utf-8 yet for some reason
				for lineno, line in enumerate(open(filename, 'rb')):
					# let lineno start with 1
					lineno += 1
					# process line
					process_line(line, filename, lineno)

	return l10n

def load_languagefile(filename):
	return json.load(open(filename), strict=False) # accept \t tabs

def write_languagefile(outputfilename, l10n_src, old_l10n_data):
	outputfilename += '.po'

	po = polib.POFile()
	po.metadata = {
		'Project-Id-Version': 'teeworlds-0.7_dev',
		'Report-Msgid-Bugs-To': 'translation@teeworlds.com',
		'POT-Creation-Date': time.strftime("%Y-%m-%d %H:%M%z"),
		'PO-Revision-Date': time.strftime("%Y-%m-%d %H:%M%z"),
		'Language-Team': 'Teeworlds Translations <translation@teeworlds.com>',
		'MIME-Version': '1.0',
		'Content-Type': 'text/plain; charset=utf-8',
		'Content-Transfer-Encoding': '8bit',
	}

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

	all_items = set(translations) | set(l10n_src)
	tsl_items = set(translations) & set(l10n_src)
	old_items = set(translations) - set(l10n_src)
	new_items = set(l10n_src) - set(translations)

	for msg in all_items:
		po.append(polib.POEntry(
			msgid=msg,
			msgstr=translations.get(msg, ""),
			obsolete=(msg in old_items),
			occurrences=l10n_src[msg],
		))
	po.save(outputfilename)

if __name__ == '__main__':
	l10n_src = parse_source()

	po = polib.POFile()
	po.metadata = {
		'Project-Id-Version': 'teeworlds-0.7_dev',
		'Report-Msgid-Bugs-To': 'translation@teeworlds.com',
		'POT-Creation-Date': time.strftime("%Y-%m-%d %H:%M%z"),
		'PO-Revision-Date': 'YEAR-MO-DA HO:MI+ZONE',
		'Language-Team': 'Teeworlds Translations <translation@teeworlds.com>',
		'MIME-Version': '1.0',
		'Content-Type': 'text/plain; charset=utf-8',
		'Content-Transfer-Encoding': '8bit',
	}
	for (msg, ctxt), occurrences in l10n_src.items():
		commenttxt = ctxt
		if(commenttxt):
			commenttxt = 'Context: '+commenttxt
		po.append(polib.POEntry(msgid=msg, msgstr="", occurrences=occurrences, msgctxt=ctxt, comment=commenttxt))
	po.save('datasrc/languages/base.pot')

	for filename in os.listdir("datasrc/languages"):
		try:
			if (os.path.splitext(filename)[1] == ".json"
					and filename != "index.json"):
				filename = "datasrc/languages/" + filename
				write_languagefile(filename, l10n_src, load_languagefile(filename))
		except Exception as e:
			print("Failed on {0}, re-raising for traceback".format(filename))
			raise
