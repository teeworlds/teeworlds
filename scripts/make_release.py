import shutil, os, sys, zipfile

#valid_platforms = ["win32", "linux86", "linux86_64", "src"]

if len(sys.argv) != 3:
	print "wrong number of arguments"
	print sys.argv[0], "VERSION PLATFORM"
	sys.exit(-1)

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

if platform == "src":
	include_data = True
	include_exe = False
	include_src = True
	use_zip = 1
	use_gz = 1

#if not platform in valid_platforms:
#	print "not a valid platform"
#	print valid_platforms
#	sys.exit(-1)

if platform == 'win32':
	exe_ext = ".exe"
	use_zip = 1
	use_gz = 0
if  platform == 'osx':
	use_dmg = 1
	use_gz = 0
	use_bundle = 1

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
				
package = "%s-%s-%s" %(name, version, platform)
package_dir = package

print "cleaning target"
shutil.rmtree(package_dir, True)
os.mkdir(package_dir)

print "adding files"
shutil.copy("readme.txt", package_dir)
shutil.copy("license.txt", package_dir)

if include_data and not use_bundle:
	os.mkdir(os.path.join(package_dir, "data"))
	copydir("data", package_dir)
	if platform[:3] == "win":
		shutil.copy("other/config_directory.bat", package_dir)

if include_exe and not use_bundle:
	shutil.copy(name+exe_ext, package_dir)
	shutil.copy(name+"_srv"+exe_ext, package_dir)
	
if include_src:
	for p in ["src", "scripts", "datasrc", "other", "objs"]:
		os.mkdir(os.path.join(package_dir, p))
		copydir(p, package_dir)
	shutil.copy("default.bam", package_dir)

if use_bundle:
	os.system("lipo -create -output teeworlds_srv teeworlds_srv_ppc teeworlds_srv_x86")
	os.system("lipo -create -output teeworlds teeworlds_ppc teeworlds_x86")
	bundle_content_dir = os.path.join(package_dir, "Teeworlds.app/Contents")
	bundle_bin_dir = os.path.join(bundle_content_dir, "MacOS")
	bundle_resource_dir = os.path.join(bundle_content_dir, "Resources")
	bundle_framework_dir = os.path.join(bundle_content_dir, "Frameworks")
	os.mkdir(os.path.join(package_dir, "Teeworlds.app"))
	os.mkdir(bundle_content_dir)
	os.mkdir(bundle_bin_dir)
	os.mkdir(bundle_resource_dir)
	os.mkdir(bundle_framework_dir)
	os.mkdir(os.path.join(bundle_resource_dir, "data"))
	copydir("data", bundle_resource_dir)
	shutil.copy("other/icons/Teeworlds.icns", bundle_resource_dir)
	shutil.copy(name+exe_ext, bundle_bin_dir)
	shutil.copy(name+"_srv"+exe_ext, bundle_bin_dir)
	os.system("cp -R /Library/Frameworks/SDL.framework " + bundle_framework_dir)
	file(os.path.join(bundle_content_dir, "Info.plist"), "w").write("""
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
	file(os.path.join(bundle_content_dir, "PkgInfo"), "w").write("APPL????")

if use_zip:
	print "making zip archive"
	zf = zipfile.ZipFile("%s.zip" % package, 'w', zipfile.ZIP_DEFLATED)
	
	for root, dirs, files in os.walk(package_dir, topdown=True):
		for name in files:
			n = os.path.join(root, name)
			zf.write(n, n)
	#zf.printdir()
	zf.close()
	
if use_gz:
	print "making tar.gz archive"
	os.system("tar czf %s.tar.gz %s" % (package, package_dir))

if use_dmg:
	print "making disk image"
	os.system("rm -f %s.dmg %s_temp.dmg" % (package, package))
	os.system("hdiutil create -srcfolder %s -volname Teeworlds -quiet %s_temp" % (package_dir, package))
	os.system("hdiutil convert %s_temp.dmg -format UDBZ -o %s.dmg -quiet" % (package, package))
	os.system("rm -f %s_temp.dmg" % package)
	
print "done"
