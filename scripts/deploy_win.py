import zipfile
import os, os.path
from distutils.file_util import copy_file

# A bit of dir trickery to make sure we're referring to the right dir
# this makes it possible to run the script both from the teewars root and
# the scripts subdir
if os.getcwd().find("scripts") > -1:
    dir = os.path.abspath("..")
else:
    dir = os.getcwd()

data_dir = "%s\\%s" % (dir, 'data')
exe_file = "%s\\%s" % (dir, 'teewars.exe')
zip_file = "%s\\%s" % (dir, 'teewars.zip')

ns = os.listdir(data_dir)
try:
    ns.remove('.svn')
except:
    pass
zf = zipfile.ZipFile(zip_file, 'w', zipfile.ZIP_DEFLATED)
zf.write(exe_file, 'teewars.exe')
for n in ns:
    zf.write(os.path.join(data_dir, n), "%s\\%s" % ('data', n))

print "Data written to zip-file:\n"
zf.printdir()
zf.close()

