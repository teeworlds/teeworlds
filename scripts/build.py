import imp, optparse, os, re, shutil, sys, zipfile
from optparse import OptionParser
if sys.version_info[0] == 2:
	import urllib
	url_lib = urllib
elif sys.version_info[0] == 3:
	import urllib.request
	url_lib = urllib.request
match = re.search('(.*)/', sys.argv[0])
if match != None:
	os.chdir(match.group(1))
os.chdir('../')

url_bam = "http://github.com/matricks/bam/zipball/master"
url_teeworlds = "http://github.com/teeworlds/teeworlds/zipball/master"
release_type = "server_release client_release"

arguments = OptionParser()
arguments.add_option("-b", "--url_bam", dest = "url_bam")
arguments.add_option("-t", "--url_teeworlds", dest = "url_teeworlds")
arguments.add_option("-r", "--release_type", dest = "release_type")
(options, arguments) = arguments.parse_args()
if options.url_bam == None:
	options.url_bam = url_bam
if options.url_teeworlds == None:
	options.url_teeworlds = url_teeworlds
if options.release_type == None:
	options.release_type = release_type

bam = options.url_bam[7:].split("/")
version_bam = re.search(r"\d\.\d\.\d", bam[len(bam)-1])
if version_bam:
	version_bam = version_bam.group(0)
else:
	version_bam = "trunk"
teeworlds = options.url_teeworlds[7:].split("/")
version_teeworlds = re.search(r"\d\.\d\.\d", teeworlds[len(teeworlds)-1])
if version_teeworlds:
	version_teeworlds = version_teeworlds.group(0)
else:
	version_teeworlds = "trunk"

bam_execution_path = ""
if version_bam < "0.3.0":
	bam_execution_path = "src%s" % os.sep

name = "teeworlds"

flag_clean = True
flag_download = True
flag_unpack = True
flag_build_bam = True
flag_build_teeworlds = True
flag_make_release = True

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
work_dir = root_dir + "scripts/work"

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
	list = "\n"
	for name in z.namelist():
		list += "%s\n" % name
		try: os.makedirs(where+"/"+os.path.dirname(name))
		except: pass

		try:
			f = open(where+"/"+name, "wb")
			f.write(z.read(name))
			f.close()
		except: pass

	z.close()

	directory = "[^/\n]*?/"
	part_1 = "(?<=\n)"
	part_2 = "[^/\n]+(?=\n)"
	regular_expression = r"%s%s" % (part_1, part_2)
	main_directory = re.search(regular_expression, list)
	while main_directory == None:
		part_1 += directory
		regular_expression = r"%s%s" % (part_1, part_2)
		main_directory = re.search(regular_expression, list)
	main_directory = re.search(r".*/", "./%s" % main_directory.group(0))
	return main_directory.group(0)

def bail(reason):
	print(reason)
	os.chdir(work_dir)
	sys.exit(-1)

def clean():
	print("*** cleaning ***")
	try: shutil.rmtree(work_dir)
	except: pass

def file_exists(file):
	try:
		open(file).close()
		return True
	except:
		return False;

def search_object(directory, object):
	directory = os.listdir(directory)
	for entry in directory:
		match = re.search(object, entry)
		if match != None:
			return entry

# clean
if flag_clean:
	clean()

# make dir
try: os.mkdir(work_dir)
except: pass

# change dir
os.chdir(work_dir)

# download
if flag_download:
	print("*** downloading bam source package ***")
	src_package_bam = fetch_file(options.url_bam)
	if src_package_bam:
		if version_bam == 'trunk':
			version = re.search(r"-[^-]*?([^-]*?)\.[^.]*$", src_package_bam)
			if version:
				version_bam = version.group(1)
	else:
		bail("couldn't find bam source package and couldn't download it")

	print("*** downloading %s source package ***" % name)
	src_package_teeworlds = fetch_file(options.url_teeworlds)
	if src_package_teeworlds:
		if version_teeworlds == 'trunk':
			version = re.search(r"-[^-]*?([^-]*?)\.[^.]*$", src_package_teeworlds)
			if version:
				version_teeworlds = version.group(1)
	else:
		bail("couldn't find %s source package and couldn't download it" % name)

# unpack
if flag_unpack:
	print("*** unpacking source ***")
	if hasattr(locals(), 'src_package_bam') == False:
		src_package_bam = search_object(work_dir, r"bam.*?\.")
		if not src_package_bam:
			bail("couldn't find bam source package")
	src_dir_bam = unzip(src_package_bam, ".")
	if not src_dir_bam:
		bail("couldn't unpack bam source package")

	if hasattr(locals(), 'src_package_teeworlds') == False:
		src_package_teeworlds = search_object(work_dir, r"teeworlds.*?\.")
		if not src_package_bam:
			bail("couldn't find %s source package" % name)
	src_dir_teeworlds = unzip(src_package_teeworlds, ".")
	if not src_dir_teeworlds:
		bail("couldn't unpack %s source package" % name)

# build bam
if flag_build_bam:
	print("*** building bam ***")
	os.chdir("%s/%s" % (work_dir, src_dir_bam))
	if os.name == "nt":
		bam_compiled = False
		compiler_bam = ["cl", "gcc"]
		for compiler in compiler_bam:
			if compiler == "cl":
				os.system("make_win32_msvc.bat")
			elif compiler == "gcc":
				os.system("make_win32_mingw.bat")
			if file_exists("%sbam.exe" % bam_execution_path) == True:
				bam_compiled = True
				break
		if bam_compiled == False:
			bail("failed to build bam")
	else:
		os.system("sh make_unix.sh")
		if file_exists("%sbam" % bam_execution_path) == False:
			bail("failed to build bam")
	os.chdir(work_dir)

# build the game
if flag_build_teeworlds:
	print("*** building %s ***" % name)
	if hasattr(locals(), 'src_dir_bam') == False:
		src_dir_bam = search_object(work_dir, r"bam[^\.]*$") + os.sep
		if src_dir_bam:
			directory = src_dir_bam + bam_execution_path
			file = r"bam"
			if os.name == "nt":
				file += r"\.exe"
			if not search_object(directory, file):
				bail("couldn't find bam")
		else:
			bail("couldn't find bam")
	if hasattr(locals(), 'src_dir_teeworlds') == False:
		src_dir_teeworlds = search_object(work_dir, r"teeworlds[^\.]*$")
		if not src_dir_teeworlds:
			bail("couldn't find %s source" % name)
	command = 1
	if os.name == "nt":
		file = open("build.bat", "wb")
		content = "@echo off\n"
		content += "@REM check if we already have the tools in the environment\n"
		content += "if exist \"%VCINSTALLDIR%\" (\ngoto compile\n)\n"
		content += "@REM Check for Visual Studio\n"
		content += "if exist \"%VS100COMNTOOLS%\" (\nset VSPATH=\"%VS100COMNTOOLS%\"\ngoto set_env\n)\n"
		content += "if exist \"%VS90COMNTOOLS%\" (\nset VSPATH=\"%VS90COMNTOOLS%\"\ngoto set_env\n)\n"
		content += "if exist \"%VS80COMNTOOLS%\" (\nset VSPATH=\"%VS80COMNTOOLS%\"\ngoto set_env\n)\n\n"
		content += "echo You need Microsoft Visual Studio 8, 9 or 10 installed\n"
		content += "exit\n"
		content += "@ setup the environment\n"
		content += ":set_env\n"
		content += "call %VSPATH%vsvars32.bat\n\n"
		content += ":compile\n"
		content += 'cd %s\n"%s\\%s%sbam" config\n"%s\\%s%sbam" %s' % (src_dir_teeworlds, work_dir, src_dir_bam, bam_execution_path, work_dir, src_dir_bam, bam_execution_path, options.release_type)
		file.write(content.encode())
		file.close()
		command = os.system("build.bat")
	else:
		os.chdir(src_dir_teeworlds)
		command = os.system("%s/%s%sbam %s" % (work_dir, src_dir_bam, bam_execution_path, options.release_type))
	if command != 0:
		bail("failed to build %s" % name)
	os.chdir(work_dir)

# make release
if flag_make_release:
	print("*** making release ***")
	if hasattr(locals(), 'src_dir_teeworlds') == False:
		src_dir_teeworlds = search_object(work_dir, r"teeworlds[^\.]*$")
		if not src_dir_teeworlds:
			bail("couldn't find %s source" % name)
	os.chdir(src_dir_teeworlds)
	command = '"%s/%s/scripts/make_release.py" %s %s' % (work_dir, src_dir_teeworlds, version_teeworlds, platform)
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
