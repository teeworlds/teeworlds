import imp, os, sys, shutil, zipfile, re
from optparse import OptionParser
imp.load_source("_compatibility", "../datasrc/_compatibility.py")
import _compatibility
url_lib = _compatibility._import(("urllib","urllib.request"))
exec("import %s" % url_lib)
exec("url_lib = %s" % url_lib)

# Initializing configuration file and command line parameter
file = open("configuration.txt", "rb")
content = file.readlines()
file.close()
if len(content) < 2:
	print("currupt configuration file")
	sys.exit(-1)
	_compatibility._input("Press enter to exit\n")
arguments = OptionParser()
arguments.add_option("-b", "--url_bam", dest = "url_bam")
arguments.add_option("-t", "--url_teeworlds", dest = "url_teeworlds")
(options, arguments) = arguments.parse_args()
if options.url_bam == None:
	options.url_bam = content[0].decode().partition(" ")[2].rstrip()
if options.url_teeworlds == None:
	options.url_teeworlds = content[1].decode().partition(" ")[2].rstrip()

bam = options.url_bam[7:].split("/")
domain_bam = bam[0]
version_bam = re.search(r"\d\.\d\.\d", bam[len(bam)-1])
if version_bam:
	version_bam = version_bam.group(0)
else:
	version_bam = "trunk"
teeworlds = options.url_teeworlds[7:].split("/")
domain_teeworlds = teeworlds[0]
version_teeworlds = re.search(r"\d\.\d\.\d", teeworlds[len(teeworlds)-1])
if version_teeworlds:
	version_teeworlds = version_teeworlds.group(0)
else:
	version_teeworlds = "trunk"

bam_execution_path = ""
if version_bam < "0.3.0":
	bam_execution_path = "src%s" % os.sep

name = "teeworlds"

flag_download = True
flag_clean = True

if os.name == "nt":
	platform = "win32"
else:
	# get name
	osname = os.popen("uname").readline().strip().lower()
	if osname == "darwin":
		osname = "osx"
		
	# get arch
	machine = os.popen("uname -m").readline().strip().lower()
	arch = "unknown"
	
	if machine[0] == "i" and machine[2:] == "86":
		arch = "x86"
	elif machine == "x86_64":
		arch = "x86_64"
	elif "power" in machine.lower():
		arch = "ppc"
		
	platform = osname + "_" + arch

print("%s-%s-%s" % (name, version_teeworlds, platform))

root_dir = os.getcwd() + "/work"
src_dir = ""

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
		pass
	return False

def unzip(filename, where):
	z = zipfile.ZipFile(filename, "r")
	for name in z.namelist():
		try: os.makedirs(where+"/"+os.path.dirname(name))
		except: pass
		
		try:
			f = open(where+"/"+name, "wb")
			f.write(z.read(name))
			f.close()
		except: pass
		
	z.close()

def path_exist(d):
	try: os.stat(d)
	except: return False
	return True

def bail(reason):
	print(reason)
	os.chdir(root_dir)
	sys.exit(-1)
	_compatibility._input("Press enter to exit\n")

# clean
if flag_clean:
	print("*** cleaning ***")
	try: shutil.rmtree("work")
	except: pass
	
# make dir
try: os.mkdir("work")
except: pass

# change dir
os.chdir(root_dir)

# download
if flag_download:
	print("*** downloading bam source package ***")
	src_package_bam = fetch_file(options.url_bam)
	if not src_package_bam:
			bail("couldn't find source package and couldn't download it")
		
	print("*** downloading %s source package ***" % name)
	src_package_teeworlds = fetch_file(options.url_teeworlds)
	if not src_package_teeworlds:
		bail("couldn't find source package and couldn't download it")

# unpack
print("*** unpacking source ***")
unzip(src_package_bam, ".")
unzip(src_package_teeworlds, ".")
src_dir_bam = src_package_bam.split(".")
src_dir_bam = src_package_bam[:(len(src_dir_bam[len(src_dir_bam)-1])+1)*-1]
src_dir_teeworlds = src_package_teeworlds.split(".")
src_dir_teeworlds = src_package_teeworlds[:(len(src_dir_teeworlds[len(src_dir_teeworlds)-1])+1)*-1]

# build bam
if 1:
	print("*** building bam ***")
	os.chdir(src_dir_bam)
	bam_cmd = "./bam"
	if os.name == "nt":
		if os.system("make_win32_msvc.bat") != 0:
			bail("failed to build bam")
		bam_cmd = "bam"
	else:
		if os.system("sh make_unix.sh") != 0:
			bail("failed to build bam")
	os.chdir(root_dir)

# build the game
if 1:
	print("*** building %s ***" % name)
	if os.name == "nt":
		winreg_lib = _compatibility._import(("_winreg","winreg"))
		exec("import %s" % winreg_lib)
		exec("winreg_lib = %s" % winreg_lib)
		try:
			key = winreg_lib.OpenKey(winreg_lib.HKEY_LOCAL_MACHINE,"SOFTWARE\Microsoft\VisualStudio\SxS\VS7")
			try:
				vsinstalldir = winreg_lib.QueryValueEx(key,"10.0")[0]
			except:
				try:
					vsinstalldir = winreg_lib.QueryValueEx(key,"9.0")[0]
				except:
					try:
						vsinstalldir = winreg_lib.QueryValueEx(key,"8.0")[0]
					except:
						bail("failed to build %s" % name)
			winreg_lib.CloseKey(key)
			file = open("build.bat", "wb")
			file.write(('call "%sVC%svcvarsall.bat"\ncd %s\n..%s%s%s%s%s server_release client_release' % (vsinstalldir, os.sep, src_dir_teeworlds, os.sep, src_dir_bam, os.sep, bam_execution_path, bam_cmd)).encode())
			file.close()
			command = os.system("build.bat")
		except:
			bail("failed to build %s" % name)
	else:
		os.chdir(src_dir_teeworlds)
		command = os.system("../%s/%sbam server_release client_release" % (src_dir_bam, bam_execution_path))
	if command != 0:
		bail("failed to build %s" % name)
	os.chdir(root_dir)

# make release
if 1:
	print("*** making release ***")
	os.chdir(src_dir_teeworlds)
	command = "..%s..%smake_release.py %s %s" % (os.sep, os.sep, version_teeworlds, platform)
	if os.name != "nt":
		command = "python %s" % command
	if os.system(command) != 0:
		bail("failed to make a relase of %s" % name)
	final_output = "FAIL"
	for f in os.listdir("."):
		if version_teeworlds in f and platform in f and name in f and (".zip" in f or ".tar.gz" in f):
			final_output = f
	os.chdir(root_dir)
	shutil.copy("%s/%s" % (src_dir_teeworlds, final_output), "../"+final_output)

print("*** all done ***")
_compatibility._input("Press enter to exit\n")
