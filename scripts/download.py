import shutil, os, re, sys, zipfile
from distutils.dir_util import copy_tree
os.chdir(os.path.dirname(os.path.realpath(sys.argv[0])) + "/..")
import twlib

def unzip(filename, where):
	try:
		z = zipfile.ZipFile(filename, "r")
	except:
		return False

	# extract files
	for name in z.namelist():
		z.extract(name, where)
	z.close()
	return z.namelist()[0]

def downloadAll(targets):
	version = "6c4af62b8c9853bfca1166d672a16abdbf9f0d26"
	url = "https://github.com/teeworlds/teeworlds-libs/archive/{}.zip".format(version)

	# download and unzip
	src_package_libs = twlib.fetch_file(url)
	if not src_package_libs:
		print("couldn't download libs")
		sys.exit(-1)
	libs_dir = unzip(src_package_libs, ".")
	if not libs_dir:
		print("couldn't unzip libs")
		sys.exit(-1)
	libs_dir = "teeworlds-libs-{}".format(version)

	if "sdl" in targets:
		copy_tree(libs_dir + "/sdl/", "other/sdl/")
	if "freetype" in targets:
		copy_tree(libs_dir + "/freetype/", "other/freetype/")

	# cleanup
	try:
		shutil.rmtree(libs_dir)
		os.remove(src_package_libs)
	except: pass

def main():
    import argparse
    p = argparse.ArgumentParser(description="Download freetype and SDL library and header files for Windows.")
    p.add_argument("targets", metavar="TARGET", nargs='+', choices=["sdl", "freetype"], help='Target to download. Valid choices are "sdl" and "freetype"')
    args = p.parse_args()

    downloadAll(args.targets)

if __name__ == '__main__':
    main()
