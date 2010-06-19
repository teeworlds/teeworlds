import imp, os, sys, shutil, zipfile, re
from optparse import OptionParser
imp.load_source("_compatibility", "../datasrc/_compatibility.py")
import _compatibility
url_lib = _compatibility._import(("urllib", "urllib.request"))
exec("import %s" % url_lib)
exec("url_lib = %s" % url_lib)

# Initializing configuration file and command line parameter
file = open("configuration.txt", "rb")
content = file.readlines()
file.close()
if len(content) < 3:
	print("currupt configuration file")
	sys.exit(-1)
	_compatibility._input("Press enter to exit\n")
arguments = OptionParser()
arguments.add_option("-b", "--url_bam", dest = "url_bam")
arguments.add_option("-t", "--url_teeworlds", dest = "url_teeworlds")
arguments.add_option("-c", "--windows_compiler", dest = "windows_compiler")
(options, arguments) = arguments.parse_args()
if options.url_bam == None:
	options.url_bam = content[0].decode().partition(" ")[2].rstrip()
if options.url_teeworlds == None:
	options.url_teeworlds = content[1].decode().partition(" ")[2].rstrip()
if options.windows_compiler == None:
	options.windows_compiler = content[2].decode().partition(" ")[2].rstrip()
	options.windows_compiler = options.windows_compiler.split(" ")

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

root_dir = os.getcwd() + os.sep
work_dir = root_dir + "work"
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
	main_directory = False
	for name in z.namelist():
		if main_directory == False and name[-1:] != "/":
			main_directory = name.split("/")
			main_directory = name[:(len(main_directory[len(main_directory)-1])+1)*-1]
			if main_directory == "":
				main_directory = ".%s" % os.sep
		try: os.makedirs(where+"/"+os.path.dirname(name))
		except: pass
		
		try:
			f = open(where+"/"+name, "wb")
			f.write(z.read(name))
			f.close()
		except: pass
		
	z.close()
	return main_directory

def path_exist(d):
	try: os.stat(d)
	except: return False
	return True

def bail(reason):
	print(reason)
	os.chdir(work_dir)
	sys.exit(-1)
	_compatibility._input("Press enter to exit\n")

def clean():
	print("*** cleaning ***")
	try: shutil.rmtree("work")
	except: pass

def file_exists(file):
	try:
		open(file).close()
		return True
	except:
		return False;

# clean
if flag_clean:
	clean()
	
# make dir
try: os.mkdir("work")
except: pass

# change dir
os.chdir(work_dir)

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
src_dir_bam = unzip(src_package_bam, ".")
src_dir_teeworlds = unzip(src_package_teeworlds, ".")

# build bam
if 1:
	print("*** building bam ***")
	os.chdir(src_dir_bam)
	bam_cmd = "./bam"
	if os.name == "nt":
		bam_compiled = False
		for compiler in options.windows_compiler:
			os.system("make_win32_%s.bat" % compiler)
			if file_exists("%sbam.exe" % bam_execution_path) == True:
				bam_compiled = True
				break
		bam_cmd = "bam"
		if bam_compiled == False:
			bail("failed to build bam")
	else:
		os.system("sh make_unix.sh")
		if file_exists("%sbam" % bam_execution_path) == False:
			bail("failed to build bam")
	os.chdir(work_dir)

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
			file.write(('call "%sVC\\vcvarsall.bat"\ncd %s\n"%s\\%s\\%s%s" server_release client_release' % (vsinstalldir, src_dir_teeworlds, work_dir, src_dir_bam, bam_execution_path, bam_cmd)).encode())
			file.close()
			command = os.system("build.bat")
		except:
			bail("failed to build %s" % name)
	else:
		os.chdir(src_dir_teeworlds)
		command = os.system("%s/%s/%sbam server_release client_release" % (work_dir, src_dir_bam, bam_execution_path))
	if command != 0:
		bail("failed to build %s" % name)
	os.chdir(work_dir)

# make release
if 1:
	print("*** making release ***")
	os.chdir(src_dir_teeworlds)
	command = '"%smake_release.py" %s %s' % (root_dir, version_teeworlds, platform)
	if os.name != "nt":
		command = "python %s" % command
	if os.system(command) != 0:
		bail("failed to make a relase of %s" % name)
	final_output = "FAIL"
	for f in os.listdir("."):
		if version_teeworlds in f and platform in f and name in f and (".zip" in f or ".tar.gz" in f):
			final_output = f
	os.chdir(work_dir)
	shutil.copy("%s/%s" % (src_dir_teeworlds, final_output), "../"+final_output)
	os.chdir(root_dir)
	clean()

print("*** all done ***")
_compatibility._input("Press enter to exit\n")
