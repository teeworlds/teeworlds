import shutil, os, re, sys, zipfile
if sys.version_info[0] == 2:
	import urllib
	url_lib = urllib
elif sys.version_info[0] == 3:
	import urllib.request
	url_lib = urllib.request

#valid_platforms = ["win32", "linux86", "linux86_64", "src"]

if len(sys.argv) != 3:
	print("wrong number of arguments")
	print(sys.argv[0], "VERSION PLATFORM")
	sys.exit(-1)

name = "teeworlds"
url_languages = "https://github.com/teeworlds/teeworlds-translation/zipball/master"
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

if platform == "src":
	include_data = True
	include_exe = False
	include_src = True
	use_zip = 1
	use_gz = 1

#if not platform in valid_platforms:
#	print("not a valid platform")
#	print(valid_platforms)
#	sys.exit(-1)

if platform == 'win32':
	exe_ext = ".exe"
	use_zip = 1
	use_gz = 0
if  platform == 'osx':
	use_dmg = 1
	use_gz = 0
	use_bundle = 1

def fetch_file(url):
	try:
		print("trying %s" % url)
		real_url = url_lib.urlopen(url).geturl()
		local = real_url.split("/")
		local = local[len(local)-1].split("?")
		local = local[0]
		url_lib.urlretrieve(real_url, local)
		return local
	except:
		return False

def unzip(filename, where):
	try:
		z = zipfile.ZipFile(filename, "r")
	except:
		return False
	for name in z.namelist():
                if "/data/languages/" in name:
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

def clean():
	print("*** cleaning ***")
	try:
                shutil.rmtree(package_dir)
                shutil.rmtree(languages_dir)
                os.remove(src_package_languages)
	except: pass
	
package = "%s-%s-%s" %(name, version, platform)
package_dir = package

print("cleaning target")
shutil.rmtree(package_dir, True)
os.mkdir(package_dir)

print("download and extract languages")
src_package_languages = fetch_file(url_languages)
if not src_package_languages:
        print("couldn't download languages")
        sys.exit(-1)
languages_dir = unzip(src_package_languages, ".")
if not languages_dir:
        print("couldn't unzip languages")
        sys.exit(-1)

print("adding files")
shutil.copy("readme.txt", package_dir)
shutil.copy("license.txt", package_dir)
shutil.copy("storage.cfg", package_dir)

if include_data and not use_bundle:
	os.mkdir(os.path.join(package_dir, "data"))
	copydir("data", package_dir)
	os.chdir(languages_dir)
	copydir("data", "../"+package_dir)
	os.chdir("..")
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
			os.system("lipo -create -output "+bin+" "+" ".join(to_lipo))

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
	os.chdir(languages_dir)
	copydir("data", "../"+clientbundle_resource_dir)
	os.chdir("..")
	shutil.copy("other/icons/Teeworlds.icns", clientbundle_resource_dir)
	shutil.copy(name+exe_ext, clientbundle_bin_dir)
	os.system("cp -R /Library/Frameworks/SDL.framework " + clientbundle_framework_dir)
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
	os.mkdir(os.path.join(serverbundle_resource_dir, "data/mapres"))
	copydir("data/maps", serverbundle_resource_dir)
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
	os.system("tar czf %s.tar.gz %s" % (package, package_dir))

if use_dmg:
	print("making disk image")
	os.system("rm -f %s.dmg %s_temp.dmg" % (package, package))
	os.system("hdiutil create -srcfolder %s -volname Teeworlds -quiet %s_temp" % (package_dir, package))
	os.system("hdiutil convert %s_temp.dmg -format UDBZ -o %s.dmg -quiet" % (package, package))
	os.system("rm -f %s_temp.dmg" % package)

clean()
	
print("done")
