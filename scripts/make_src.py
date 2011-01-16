import os, shutil, zipfile, sys

if len(sys.argv) <= 1:
	print "%s VERSION [SVN TREE]" % sys.argv[0]
	sys.exit(-1)

version = sys.argv[1]
svn_tree = "tags/release-%s" % version

if len(sys.argv) > 2:
	svn_tree = sys.argv[2]

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
#if 1:
#	os.system("svn export http://stalverk80.se/svn/bam bam")
#	z = zipfile.ZipFile("../bam.zip", "w")
#	for root, dirs, files in os.walk("bam"):
#		for f in files:
#			z.write(root+"/"+ f)
#	z.close()

if 1:
	os.system("svn export svn://svn.teeworlds.com/teeworlds/%s teeworlds" % svn_tree)
	os.chdir("teeworlds")
	os.system("python scripts/make_release.py %s src" % version)
	os.chdir(root_dir)
	for f in os.listdir("teeworlds"):
		if "teeworlds" in f and "src" in f and (".zip" in f or ".tar.gz" in f):
			shutil.copy("teeworlds/"+f, "../" + f)
