from collections import namedtuple
import os
import re
import shlex
import subprocess

Config = namedtuple('Config', 'install_name_tool otool verbose')

def dylib_regex(name):
	return re.compile(r'\S*{}\S*'.format(re.escape(name)))

class ChangeDylib:
	def __init__(self, config):
		self.config = config
	def _check_call(self, process_args, *args, **kwargs):
		if self.config.verbose >= 1:
			print("EXECUTING {}".format(" ".join(shlex.quote(x) for x in process_args)))
		if not (self.config.verbose >= 2 and "stdout" not in kwargs):
			kwargs["stdout"] = open(os.devnull, 'wb')
		return subprocess.check_call(process_args, *args, **kwargs)
	def _check_output(self, process_args, *args, **kwargs):
		if self.config.verbose >= 1:
			print("EXECUTING {} FOR OUTPUT".format(" ".join(shlex.quote(x) for x in process_args)))
		return subprocess.check_output(process_args, *args, **kwargs)
	def _install_name_tool(self, *args):
		return self._check_call((self.config.install_name_tool,) + args)
	def _otool(self, *args):
		return self._check_output((self.config.otool,) + args)

	def change(self, filename, from_, to):
		lines = self._otool("-L", filename).decode().splitlines()
		regex = dylib_regex(from_)
		matches = sum([regex.findall(l) for l in lines], [])
		if len(matches) != 1:
			if matches:
				raise ValueError("More than one match found for {}: {}".format(from_, matches))
			else:
				raise ValueError("No matches found for {}".format(from_))
		actual_from = matches[0]
		self._install_name_tool("-change", actual_from, to, filename)

def main():
	import argparse
	p = argparse.ArgumentParser(description="Manipulate shared library dependencies for macOS")

	subcommands = p.add_subparsers(help="Subcommand", dest='command', metavar="COMMAND")
	subcommands.required = True

	change = subcommands.add_parser("change", help="Change a shared library dependency to a given value")
	change.add_argument('-v', '--verbose', action='count', help="Verbose output")
	change.add_argument('--tools', nargs=2, help="Paths to the install_name_tool and otool", default=("install_name_tool", "otool"))
	change.add_argument('filename', metavar="FILE", help="Filename of the executable to manipulate")
	change.add_argument('from_', metavar="FROM", help="Fuzzily matched library name to change")
	change.add_argument('to', metavar="TO", help="Exact name that the library dependency should be changed to")
	args = p.parse_args()

	verbose = args.verbose or 0
	install_name_tool, otool = args.tools
	dylib = ChangeDylib(Config(install_name_tool, otool, verbose))
	dylib.change(filename=args.filename, from_=args.from_, to=args.to)

if __name__ == '__main__':
	main()
