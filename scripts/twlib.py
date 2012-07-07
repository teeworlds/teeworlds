import sys
if sys.version_info[0] == 2:
	import urllib
	url_lib = urllib
elif sys.version_info[0] == 3:
	import urllib.request
	url_lib = urllib.request

def fetch_file(url):
	try:
		print("trying %s" % url)
		local = dict(url_lib.urlopen(url).info())["content-disposition"].split("=")[1]
		url_lib.urlretrieve(url, local)
		return local
	except:
		return False
