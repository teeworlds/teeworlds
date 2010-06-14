import imp, os, sys, shutil, zipfile
from optparse import OptionParser
imp.load_source("_compatibility","../datasrc/_compatibility.py")
import _compatibility
http_lib = _compatibility._import(("httplib","http.client"))
exec("import %s" % http_lib)
exec("http_lib = %s" % http_lib)

# Initializing configuration file and command line parameter
file = open("configuration.txt","rt")
content = file.readlines()
file.close()
if len(content) < 5:
	print("currupt configuration file")
	sys.exit(-1)
arguments = OptionParser()
arguments.add_option("-d", "--domain", dest="domain")
arguments.add_option("-m", "--download_path_bam", dest="download_path_bam")
arguments.add_option("-n", "--download_path_teeworlds", dest="download_path_teeworlds")
arguments.add_option("-b", "--version_bam", dest="version_bam")
arguments.add_option("-t", "--version_teeworlds", dest="version_teeworlds")
(options, arguments) = arguments.parse_args()
if options.domain == None:
	options.domain = content[0].partition(" ")[2].rstrip()
if options.download_path_bam == None:
	options.download_path_bam = content[1].partition(" ")[2].rstrip()
if options.download_path_teeworlds == None:
	options.download_path_teeworlds = content[2].partition(" ")[2].rstrip()
if options.version_bam == None:
	options.version_bam = content[3].partition(" ")[2].rstrip()
if options.version_teeworlds == None:
	options.version_teeworlds = content[4].partition(" ")[2].rstrip()
bam_execution_path = ""
if options.version_bam < "0.3.0":
	bam_execution_path = "src"
if os.name == "nt":
	bam_execution_path += "\\\\"
else:
	bam_execution_path += "/"

name = "teeworlds"
options.version_bam = "bam-%s" % options.version_bam

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

print("%s-%s-%s" % (name,options.version_teeworlds, platform))

src_package = "%s-%s-src.zip" % (name, options.version_teeworlds)

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
	if not fetch_file(options.domain, "/%s/%s.zip?format=raw" % (options.download_path_bam, options.version_bam), "bam.zip"):
			bail("couldn't find source package and couldn't download it")
		
	print("*** downloading %s source package ***" % name)
	if not fetch_file(options.domain, "/%s/%s" % (options.download_path_teeworlds, src_package), src_package):
		bail("couldn't find source package and couldn't download it")

# unpack
print("*** unpacking source ***")
unzip("bam.zip", ".")
unzip(src_package, name)
src_dir = name+"/"+ os.listdir(name+"/")[0]

# build bam
if 1:
	print("*** building bam ***")
	os.chdir(options.version_bam)
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
			file = open("build.bat","wt")
			file.write('call "%sVC\\vcvarsall.bat"\ncd %s\n..\\..\\%s\\%s%s server_release client_release' % (vsinstalldir, src_dir, options.version_bam, bam_execution_path, bam_cmd))
			file.close()
			command = os.system("build.bat")
		except:
			bail("failed to build %s" % name)
	else:
		os.chdir(src_dir)
		command = os.system("../../%s/%sbam server_release client_release" % (options.version_bam, bam_execution_path))
	if command != 0:
		bail("failed to build %s" % name)
	os.chdir(root_dir)

# make release
if 1:
	print("*** making release ***")
	os.chdir(src_dir)
	if os.name == "nt":
		command = os.system("..\\..\\..\\make_release.py %s %s" % (options.version_teeworlds, platform))
	else:
		command = os.system("python ../../../make_release.py %s %s" % (options.version_teeworlds, platform))
	if command != 0:
		bail("failed to make a relase of %s" % name)
	final_output = "FAIL"
	for f in os.listdir("."):
		if options.version_teeworlds in f and platform in f and name in f and (".zip" in f or ".tar.gz" in f):
			final_output = f
	os.chdir(root_dir)
	shutil.copy("%s/%s" % (src_dir, final_output), "../"+final_output)

print("*** all done ***")
