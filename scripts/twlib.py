import shutil
import sys
if sys.version_info[0] == 2:
	import urllib
	url_lib = urllib
elif sys.version_info[0] == 3:
	import urllib.request
	url_lib = urllib.request

try:
	from distutils.dir_util import copy_tree
except ModuleNotFoundError:
	# distutils was removed in python 3.12
	# https://peps.python.org/pep-0632/
	def copy_tree(src, dst):
		shutil.copytree(src, dst, dirs_exist_ok=True)

def fetch_file(url):
	print("trying %s" % url)
	try:
		local = dict(url_lib.urlopen(url).info())
		if "Content-Disposition" in local:
			key_name = "Content-Disposition"
		elif "content-disposition" in local:
			key_name = "content-disposition"
		else:
			return False
		local = local[key_name].split("=")[1]
		url_lib.urlretrieve(url, local)
		return local
	except IOError:
		return False
