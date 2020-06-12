import shutil, optparse, os, re, sys, zipfile
from distutils.dir_util import copy_tree
os.chdir(os.path.dirname(os.path.realpath(sys.argv[0])) + "/..")
import twlib

arguments = optparse.OptionParser(usage="usage: %prog VERSION PLATFORM [options]\n\nVERSION  - Version number\nPLATFORM - Target platform (f.e. linux_x86, linux_x86_64, osx, src, win32, win64)")
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

valid_platforms = ["win32", "win64", "osx", "linux_x86", "linux_x86_64", "src"]

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

if not platform in valid_platforms:
	print("not a valid platform")
	print(valid_platforms)
	sys.exit(-1)

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

def shell(cmd):
	if os.system(cmd) != 0:
		clean()
		print("Non zero exit code on: os.system(%s)" % cmd)
		sys.exit(1)

package = "%s-%s-%s" %(name, version, platform)
package_dir = package

source_package_dir = "build/"
if platform == 'win32' or platform == 'linux_x86':
	source_package_dir += "x86/release/"
else:
	source_package_dir += "x86_64/release/"

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
	copy_tree(source_package_dir+"data", package_dir+"/data")
	copy_tree(languages_dir, package_dir+"/data/languages")
	copy_tree(maps_dir, package_dir+"/data/maps")
	if platform[:3] == "win":
		shutil.copy("other/config_directory.bat", package_dir)
		shutil.copy(source_package_dir+"SDL2.dll", package_dir)
		shutil.copy(source_package_dir+"freetype.dll", package_dir)

if include_exe and not use_bundle:
	shutil.copy(source_package_dir+name+exe_ext, package_dir)
	shutil.copy(source_package_dir+name+"_srv"+exe_ext, package_dir)
	
if include_src:
	for p in ["src", "scripts", "datasrc", "other", "objs"]:
		os.mkdir(os.path.join(package_dir, p))
		copydir(p, package_dir)
	shutil.copy("bam.lua", package_dir)
	shutil.copy("configure.lua", package_dir)

if use_bundle:
	bins = [name, name+'_srv', 'serverlaunch']
	platforms = ('x86_64')
	for bin in bins:
		to_lipo = []
		for p in platforms:
			fname = bin+'_'+p
			if os.path.isfile(fname):
				to_lipo.append(fname)
		if to_lipo:
			shell("lipo -create -output "+bin+" "+" ".join(to_lipo))

	# create Teeworlds appfolder
	clientbundle_content_dir = os.path.join(package_dir, "Teeworlds.app/Contents")
	clientbundle_bin_dir = os.path.join(clientbundle_content_dir, "MacOS")
	clientbundle_resource_dir = os.path.join(clientbundle_content_dir, "Resources")
	clientbundle_framework_dir = os.path.join(clientbundle_content_dir, "Frameworks")
	binary_path = clientbundle_bin_dir + "/" + name+exe_ext
	freetypelib_path = clientbundle_framework_dir + "/libfreetype.6.dylib"
	os.mkdir(os.path.join(package_dir, "Teeworlds.app"))
	os.mkdir(clientbundle_content_dir)
	os.mkdir(clientbundle_bin_dir)
	os.mkdir(clientbundle_resource_dir)
	os.mkdir(clientbundle_framework_dir)
	copy_tree(source_package_dir+"data", clientbundle_resource_dir+"/data")
	copy_tree(languages_dir, clientbundle_resource_dir+"/data/languages")
	copy_tree(maps_dir, clientbundle_resource_dir+"/data/maps")
	shutil.copy("other/icons/Teeworlds.icns", clientbundle_resource_dir)
	shutil.copy(source_package_dir+name+exe_ext, clientbundle_bin_dir)
	shell("install_name_tool -change /usr/local/opt/freetype/lib/libfreetype.6.dylib @executable_path/../Frameworks/libfreetype.6.dylib " + binary_path)
	shell("install_name_tool -change /usr/local/opt/sdl2/lib/libSDL2-2.0.0.dylib @executable_path/../Frameworks/libSDL2-2.0.0.dylib  " + binary_path)
	shell("cp /usr/local/opt/freetype/lib/libfreetype.6.dylib " + clientbundle_framework_dir)
	shell("cp /usr/local/opt/libpng/lib/libpng16.16.dylib " + clientbundle_framework_dir)
	shell("cp /usr/local/opt/sdl2/lib/libSDL2-2.0.0.dylib " + clientbundle_framework_dir)
	shell("install_name_tool -change /usr/local/opt/libpng/lib/libpng16.16.dylib @executable_path/../Frameworks/libpng16.16.dylib " + freetypelib_path)
	open(os.path.join(clientbundle_content_dir, "Info.plist"), "w").write("""
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
	<key>CFBundleIdentifier</key>
	<string>com.TeeworldsClient.app</string>
	<key>NSHighResolutionCapable</key>
	<true/>
</dict>
</plist>
	""" % (version))
	open(os.path.join(clientbundle_content_dir, "PkgInfo"), "w").write("APPL????")

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
	os.mkdir(os.path.join(serverbundle_resource_dir, "data/mapres"))
	copy_tree(maps_dir, serverbundle_resource_dir+"/data/maps")
	shutil.copy("other/icons/Teeworlds_srv.icns", serverbundle_resource_dir)
	shutil.copy(source_package_dir+name+"_srv"+exe_ext, serverbundle_bin_dir)
	shutil.copy(source_package_dir+"serverlaunch"+exe_ext, serverbundle_bin_dir + "/"+name+"_server")
	open(os.path.join(serverbundle_content_dir, "Info.plist"), "w").write("""
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
	open(os.path.join(serverbundle_content_dir, "PkgInfo"), "w").write("APPL????")

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
	shell("tar czf %s.tar.gz %s" % (package, package_dir))

if use_dmg:
	print("making disk image")
	shell("rm -f %s.dmg %s_temp.dmg" % (package, package))
	shell("hdiutil create -srcfolder %s -volname Teeworlds -quiet %s_temp" % (package_dir, package))
	shell("hdiutil convert %s_temp.dmg -format UDBZ -o %s.dmg -quiet" % (package, package))
	shell("rm -f %s_temp.dmg" % package)

clean()

print("done")
