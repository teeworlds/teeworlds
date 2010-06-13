import _compatibility, os, sys, shutil, zipfile
http_lib = _compatibility._import(("httplib","http.client"))
exec("import %s" % http_lib)
exec("http_lib=%s" % http_lib)

if len(sys.argv) != 2:
	print("wrong number of arguments")
	sys.exit(-1)

name = "teeworlds"
domain = "www.%s.com" % name
version = sys.argv[1]

bam_version = "bam-0.2.0"

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

print("%s-%s-%s" % (name,version, platform))

src_package = "%s-%s-src.zip" % (name, version)

root_dir = os.getcwd() + "/work"
src_dir = ""

def fetch_file(server, url, local):
	try:
		conn = http_lib.HTTPConnection(server)
		print("trying %s%s" % (server, url))
		conn.request("GET", url)
		response = conn.getresponse()
		if response.status != 200:
			return False
		
		f = open(local, "wb")
		f.write(response.read())
		f.close()
		conn.close()
		return True
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
	if not fetch_file(domain, "/trac/bam/browser/releases/%s.zip?format=raw" % bam_version, "bam.zip"):
			bail("couldn't find source package and couldn't download it")
		
	print("*** downloading %s source package ***" % name)
	if not fetch_file(domain, "/files/%s" % src_package, src_package):
		if not fetch_file(domain, "/files/beta/%s" % src_package, src_package):
			bail("couldn't find source package and couldn't download it")

# unpack
print("*** unpacking source ***")
unzip("bam.zip", ".")
unzip(src_package, name)
src_dir = name+"/"+ os.listdir(name+"/")[0]

# build bam
if 1:
	print("*** building bam ***")
	os.chdir(bam_version)
	output = "bam"
	bam_cmd = "./bam"
	if os.name == "nt":
		if os.system("make_win32_msvc.bat") != 0:
			bail("failed to build bam")
		output += ".exe"
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
		exec("winreg_lib=%s" % winreg_lib)
		try:
			key=winreg_lib.OpenKey(winreg_lib.HKEY_LOCAL_MACHINE,"SOFTWARE\Microsoft\VisualStudio\SxS\VS7")
			try:
				vsinstalldir=winreg_lib.QueryValueEx(key,"10.0")[0]
			except:
				try:
					vsinstalldir=winreg_lib.QueryValueEx(key,"9.0")[0]
				except:
					try:
						vsinstalldir=winreg_lib.QueryValueEx(key,"8.0")[0]
					except:
						bail("failed to build %s" % name)
			winreg_lib.CloseKey(key)
			file=open("build.bat","wt")
			file.write('call "%sVC\\vcvarsall.bat"\ncd %s\n..\\..\\%s\\src\\%s server_release client_release' % (vsinstalldir, src_dir, bam_version, bam_cmd))
			file.close()
			command = os.system("build.bat")
		except:
			bail("failed to build %s" % name)
	else:
		os.chdir(src_dir)
		command = os.system("../../"+bam_version+"/src/bam server_release client_release")
	if command != 0:
		bail("failed to build %s" % name)
	os.chdir(root_dir)

# make release
if 1:
	print("*** making release ***")
	os.chdir(src_dir)
	if os.name == "nt":
		command = os.system("..\\..\\..\\make_release.py %s %s" % (version, platform))
	else:
		command = os.system("python ../../../make_release.py %s %s" % (version, platform))
	if command != 0:
		bail("failed to make a relase of %s"%name)
	final_output = "FAIL"
	for f in os.listdir("."):
		if version in f and platform in f and name in f and (".zip" in f or ".tar.gz" in f):
			final_output = f
	os.chdir(root_dir)
	shutil.copy("%s/%s" % (src_dir, final_output), "../"+final_output)

print("*** all done ***")
