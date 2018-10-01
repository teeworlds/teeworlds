from collections import namedtuple
import os
import shlex
import subprocess
import tempfile

ConfigDmgtools = namedtuple('Config', 'dmg hfsplus newfs_hfs verbose')
ConfigHdiutil = namedtuple('Config', 'hdiutil verbose')

def chunks(l, n):
	"""
	Yield successive n-sized chunks from l.
	
	From https://stackoverflow.com/a/312464.
	"""
	for i in range(0, len(l), n):
		yield l[i:i + n]

class Dmg:
	def __init__(self, config):
		self.config = config

	def _check_call(self, process_args, *args, **kwargs):
		if self.config.verbose >= 1:
			print("EXECUTING {}".format(" ".join(shlex.quote(x) for x in process_args)))
		if not (self.config.verbose >= 2 and "stdout" not in kwargs):
			kwargs["stdout"] = open(os.devnull, 'wb')
		subprocess.check_call(process_args, *args, **kwargs)

class Dmgtools(Dmg):
	def _mkfs_hfs(self, *args):
		self._check_call((self.config.newfs_hfs,) + args)
	def _hfs(self, *args):
		self._check_call((self.config.hfsplus,) + args)
	def _dmg(self, *args):
		self._check_call((self.config.dmg,) + args)

	def _create_hfs(self, hfs, volume_name, size):
		if self.config.verbose >= 1:
			print("TRUNCATING {} to {} bytes".format(hfs, size))
		with open(hfs, 'wb') as f:
			f.truncate(size)
		self._mkfs_hfs('-v', volume_name, hfs)

	def _symlink(self, hfs, target, link_name):
		self._hfs(hfs, 'symlink', link_name, target)

	def _add(self, hfs, directory):
		self._hfs(hfs, 'addall', directory)

	def _finish(self, hfs, dmg):
		self._dmg('build', hfs, dmg)

	def create(self, dmg, volume_name, directory, symlinks):
		input_size = sum(os.stat(os.path.join(path, f)).st_size for path, dirs, files in os.walk(directory) for f in files)
		output_size = max(input_size * 2, 1024**2)
		hfs = tempfile.mktemp(prefix=dmg + '.', suffix='.hfs')
		self._create_hfs(hfs, volume_name, output_size)
		self._add(hfs, directory)
		for target, link_name in symlinks:
			self._symlink(hfs, target, link_name)
		self._finish(hfs, dmg)
		if self.config.verbose >= 1:
			print("REMOVING {}".format(hfs))
		os.remove(hfs)

class Hdiutil(Dmg):
	def _hdiutil(self, *args):
		self._check_call((self.config.hdiutil,) + args)

	def create(self, dmg, volume_name, directory, symlinks):
		if symlinks:
			raise NotImplementedError("symlinks are not yet implemented")
		self._hdiutil('create', '-volname', volume_name, '-srcdir', directory, dmg)

def main():
	import argparse
	p = argparse.ArgumentParser(description="Manipulate dmg archives")

	subcommands = p.add_subparsers(help="Subcommand", dest='command', metavar="COMMAND")
	subcommands.required = True

	create = subcommands.add_parser("create", help="Create a dmg archive from files or directories")
	create.add_argument('-v', '--verbose', action='count', help="Verbose output")
	createx = create.add_mutually_exclusive_group(required=True)
	createx.add_argument('--dmgtools', nargs=3, help="Paths to the dmg and hfsplus executable (https://github.com/mozilla/libdmg-hfsplus) and the newfs_hfs executable (http://pkgs.fedoraproject.org/repo/pkgs/hfsplus-tools/diskdev_cmds-540.1.linux3.tar.gz/0435afc389b919027b69616ad1b05709/diskdev_cmds-540.1.linux3.tar.gz)")
	createx.add_argument('--hdiutil', help="Path to the hdiutil (only exists for macOS at time of writing)")
	create.add_argument('output', metavar="OUTPUT", help="Filename of the output dmg archive")
	create.add_argument('volume_name', metavar="VOLUME_NAME", help="Name of the dmg archive")
	create.add_argument('directory', metavar="DIR", help="Directory to create the archive from")
	create.add_argument('--symlink', metavar="SYMLINK", nargs=2, action="append", help="Symlink the first argument under the second name")
	args = p.parse_args()

	verbose = args.verbose or 0
	symlinks = args.symlink or []
	if args.dmgtools:
		dmg, hfsplus, newfs_hfs = args.dmgtools
		dmg = Dmgtools(ConfigDmgtools(dmg=dmg, hfsplus=hfsplus, newfs_hfs=newfs_hfs, verbose=verbose))
	elif args.hdiutil:
		dmg = Hdiutil(ConfigHdiutil(hdiutil=args.hdiutil, verbose=verbose))
	else:
		raise RuntimeError("unreachable")
	dmg.create(volume_name=args.volume_name, directory=args.directory, dmg=args.output, symlinks=symlinks)

if __name__ == '__main__':
	main()
