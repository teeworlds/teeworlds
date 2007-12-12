import os, shutil, zipfile

version = "0.3.0-test"
svn_tree = "trunk"

# make clean
if 1:
	try: shutil.rmtree("srcwork")
	except: pass
	try: os.mkdir("srcwork")
	except: pass

root_dir = os.getcwd() + "/srcwork"

# change dir
os.chdir(root_dir)

# fix bam
if 1:
	os.system("svn export http://stalverk80.se/svn/bam bam")
	z = zipfile.ZipFile("../bam.zip", "w")
	for root, dirs, files in os.walk("bam"):
		for f in files:
			z.write(root+"/"+ f)
	z.close()

if 1:
	os.system("svn export svn://svn.teewars.com/teewars/%s teewars" % svn_tree)
	os.chdir("teewars")
	os.system("python scripts/make_release.py %s src" % version)
	os.chdir(root_dir)
	for f in os.listdir("teewars"):
		if "teewars" in f and "src" in f and (".zip" in f or ".tar.gz" in f):
			shutil.copy("teewars/"+f, "../" + f)
