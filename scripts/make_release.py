import shutil, optparse, os, re, sys, zipfile, subprocess
os.chdir(os.path.dirname(os.path.realpath(sys.argv[0])) + "/..")
import twlib

arguments = optparse.OptionParser(usage="usage: %prog VERSION PLATFORM [options]\n\nVERSION  - Version number\nPLATFORM - Target platform (f.e. linux86, linux86_64, osx, src, win32)")
arguments.add_option("-l", "--url-languages", default = "http://github.com/teeworlds/teeworlds-translation/archive/master.zip", help = "URL from which the teeworlds language files will be downloaded")
arguments.add_option("-m", "--url-maps", default = "http://github.com/teeworlds/teeworlds-maps/archive/master.zip", help = "URL from which the teeworlds maps files will be downloaded")
arguments.add_option("-s", "--source-dir", help = "Source directory which is used for building the package")
(options, arguments) = arguments.parse_args()
if len(sys.argv) != 3:
	print("wrong number of arguments")
	print(sys.argv[0], "VERSION PLATFORM")
	sys.exit(-1)
if options.source_dir != None:
	if os.path.exists(options.source_dir) == False:
		print("Source directory " + options.source_dir + " doesn't exist")
		exit(1)
	os.chdir(options.source_dir)

#valid_platforms = ["win32", "osx", "linux86", "linux86_64", "src"]

name = "teeworlds"
version = sys.argv[1]
platform = sys.argv[2]
exe_ext = ""
use_zip = 0
use_gz = 1
use_dmg = 0
use_bundle = 0
include_data = True
include_exe = True
include_src = False

#if not options.platform in valid_platforms:
#	print("not a valid platform")
#	print(valid_platforms)
#	sys.exit(-1)

if platform == "src":
	include_exe = False
	include_src = True
	use_zip = 1
elif platform == 'win32' or platform == 'win64':
	exe_ext = ".exe"
	use_zip = 1
	use_gz = 0
elif platform == 'osx':
	use_dmg = 1
	use_gz = 0
	use_bundle = 1

def unzip(filename, where):
	try:
		z = zipfile.ZipFile(filename, "r")
	except:
		return False
	for name in z.namelist():
		z.extract(name, where)
	z.close()
	return z.namelist()[0]

def copydir(src, dst, excl=[]):
	for root, dirs, files in os.walk(src, topdown=True):
		if "/." in root or "\\." in root:
			continue
		for name in dirs:
			if name[0] != '.':
				os.mkdir(os.path.join(dst, root, name))
		for name in files:
			if name[0] != '.':
				shutil.copy(os.path.join(root, name), os.path.join(dst, root, name))

def copyfiles(src, dst):
	dir = os.listdir(src)
	for files in dir:
		shutil.copy(os.path.join(src, files), os.path.join(dst, files))

def clean():
	print("*** cleaning ***")
	try:
		shutil.rmtree(package_dir)
		shutil.rmtree(languages_dir)
		shutil.rmtree(maps_dir)
		os.remove(src_package_languages)
		os.remove(src_package_maps)
	except: pass
	
package = "%s-%s-%s" %(name, version, platform)
package_dir = package

print("cleaning target")
shutil.rmtree(package_dir, True)
os.mkdir(package_dir)

print("download and extract languages")
src_package_languages = twlib.fetch_file(options.url_languages)
if not src_package_languages:
	print("couldn't download languages")
	sys.exit(-1)
languages_dir = unzip(src_package_languages, ".")
if not languages_dir:
	print("couldn't unzip languages")
	sys.exit(-1)

print("download and extract maps")
src_package_maps = twlib.fetch_file(options.url_maps)
if not src_package_maps:
	print("couldn't download maps")
	sys.exit(-1)
maps_dir = unzip(src_package_maps, ".")
if not maps_dir:
	print("couldn't unzip maps")
	sys.exit(-1)

print("adding files")
shutil.copy("readme.md", package_dir)
shutil.copy("license.txt", package_dir)
shutil.copy("storage.cfg", package_dir)

if include_data and not use_bundle:
	os.mkdir(os.path.join(package_dir, "data"))
	copydir("data", package_dir)
	copyfiles(languages_dir, package_dir+"/data/languages")
	copyfiles(maps_dir, package_dir+"/data/maps")
	if platform[:3] == "win":
		shutil.copy("other/config_directory.bat", package_dir)
		shutil.copy("SDL.dll", package_dir)
		shutil.copy("freetype.dll", package_dir)

if include_exe and not use_bundle:
	shutil.copy(name+exe_ext, package_dir)
	shutil.copy(name+"_srv"+exe_ext, package_dir)
	
if include_src:
	for p in ["src", "scripts", "datasrc", "other", "objs"]:
		os.mkdir(os.path.join(package_dir, p))
		copydir(p, package_dir)
	shutil.copy("bam.lua", package_dir)
	shutil.copy("configure.lua", package_dir)

if use_bundle:
	bins = [name, name+'_srv', 'serverlaunch']
	platforms = ('x86', 'x86_64', 'ppc')
	for bin in bins:
		to_lipo = []
		for p in platforms:
			fname = bin+'_'+p
			if os.path.isfile(fname):
				to_lipo.append(fname)
		if to_lipo:
			args = ["lipo", "-create", "-output", bin]
			args.extend(to_lipo)
			subprocess.check_call(args)

	# create Teeworlds appfolder
	clientbundle_content_dir = os.path.join(package_dir, "Teeworlds.app/Contents")
	clientbundle_bin_dir = os.path.join(clientbundle_content_dir, "MacOS")
	clientbundle_resource_dir = os.path.join(clientbundle_content_dir, "Resources")
	clientbundle_framework_dir = os.path.join(clientbundle_content_dir, "Frameworks")
	os.mkdir(os.path.join(package_dir, "Teeworlds.app"))
	os.mkdir(clientbundle_content_dir)
	os.mkdir(clientbundle_bin_dir)
	os.mkdir(clientbundle_resource_dir)
	os.mkdir(clientbundle_framework_dir)
	os.mkdir(os.path.join(clientbundle_resource_dir, "data"))
	copydir("data", clientbundle_resource_dir)
	copyfiles(languages_dir, clientbundle_resource_dir+"/data/languages")
	copyfiles(maps_dir, clientbundle_resource_dir+"/data/maps")
	shutil.copy("other/icons/Teeworlds.icns", clientbundle_resource_dir)

	lines = subprocess.check_output(["otool", "-L", name + exe_ext]).splitlines()
	libs = []
	for line in lines:
		m = re.search("/.*libfreetype.*dylib", line)
		if m:
			libfreetype_full = m.group(0)
			libfreetype_short = re.search("(?<=/)libfreetype.*dylib", libfreetype_full).group(0)
			libfreetype = {'full': libfreetype_full, 'short': libfreetype_short}
			shutil.copy(libfreetype['full'], clientbundle_framework_dir)
			libs.append(libfreetype)

		m = re.search("/.*libpng.*dylib", line)
		if m:
			libpng_full = m.group(0)
			libpng_short = re.search("(?<=/)libpng.*dylib", libpng_full).group(0)
			libpng = {'full': libpng_full, 'short': libpng_short}
			libs.append(libpng)

	bins = [name+exe_ext]

	for lib in libs:
		bins.append(os.path.join(clientbundle_framework_dir, lib['short']))
		shutil.copy(lib['full'], clientbundle_framework_dir)

	for bin in bins:
		for lib in libs:
			subprocess.check_call(["install_name_tool", "-change", lib['full'], "@rpath/" + lib['short'], bin])

	shutil.copy(name+exe_ext, clientbundle_bin_dir)
	shutil.copytree("/Library/Frameworks/SDL.framework", os.path.join(clientbundle_framework_dir, "SDL.framework"), True)
	file(os.path.join(clientbundle_content_dir, "Info.plist"), "w").write("""
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple Computer//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
	<key>CFBundleDevelopmentRegion</key>
	<string>English</string>
	<key>CFBundleExecutable</key>
	<string>teeworlds</string>
	<key>CFBundleIconFile</key>
	<string>Teeworlds</string>
	<key>CFBundleInfoDictionaryVersion</key>
	<string>6.0</string>
	<key>CFBundlePackageType</key>
	<string>APPL</string>
	<key>CFBundleSignature</key>
	<string>????</string>
	<key>CFBundleVersion</key>
	<string>%s</string>
</dict>
</plist>
	""" % (version))
	file(os.path.join(clientbundle_content_dir, "PkgInfo"), "w").write("APPL????")

	# create Teeworlds Server appfolder
	serverbundle_content_dir = os.path.join(package_dir, "Teeworlds Server.app/Contents")
	serverbundle_bin_dir = os.path.join(serverbundle_content_dir, "MacOS")
	serverbundle_resource_dir = os.path.join(serverbundle_content_dir, "Resources")
	os.mkdir(os.path.join(package_dir, "Teeworlds Server.app"))
	os.mkdir(serverbundle_content_dir)
	os.mkdir(serverbundle_bin_dir)
	os.mkdir(serverbundle_resource_dir)
	os.mkdir(os.path.join(serverbundle_resource_dir, "data"))
	os.mkdir(os.path.join(serverbundle_resource_dir, "data/maps"))
	copyfiles(maps_dir, serverbundle_resource_dir+"/data/maps")
	shutil.copy("other/icons/Teeworlds_srv.icns", serverbundle_resource_dir)
	shutil.copy(name+"_srv"+exe_ext, serverbundle_bin_dir)
	shutil.copy("serverlaunch"+exe_ext, serverbundle_bin_dir + "/"+name+"_server")
	file(os.path.join(serverbundle_content_dir, "Info.plist"), "w").write("""
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple Computer//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
	<key>CFBundleDevelopmentRegion</key>
	<string>English</string>
	<key>CFBundleExecutable</key>
	<string>teeworlds_server</string>
	<key>CFBundleIconFile</key>
	<string>Teeworlds_srv</string>
	<key>CFBundleInfoDictionaryVersion</key>
	<string>6.0</string>
	<key>CFBundlePackageType</key>
	<string>APPL</string>
	<key>CFBundleSignature</key>
	<string>????</string>
	<key>CFBundleVersion</key>
	<string>%s</string>
</dict>
</plist>
	""" % (version))
	file(os.path.join(serverbundle_content_dir, "PkgInfo"), "w").write("APPL????")

if use_zip:
	print("making zip archive")
	zf = zipfile.ZipFile("%s.zip" % package, 'w', zipfile.ZIP_DEFLATED)
	
	for root, dirs, files in os.walk(package_dir, topdown=True):
		for name in files:
			n = os.path.join(root, name)
			zf.write(n, n)
	#zf.printdir()
	zf.close()
	
if use_gz:
	print("making tar.gz archive")
	subprocess.check_call(["tar", "czf", package+".tar.gz", package_dir])

if use_dmg:
	print("making disk image")
	if os.path.isfile(package+".dmg"): os.remove(package+".dmg")
	if os.path.isfile(package+"_temp.dmg"): os.remove(package+"_temp.dmg")
	subprocess.check_call(["hdiutil", "create", "-srcfolder", package_dir, "-volname", "Teeworlds", "-quiet", package+"_temp"])
	subprocess.check_call(["hdiutil", "convert", package+"_temp.dmg", "-format", "UDBZ", "-o", package+".dmg", "-quiet"])
	os.remove(package+"_temp.dmg")

clean()
	
print("done")
