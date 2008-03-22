import md5, sys, re
m = md5.new()

alphanum = "0123456789abcdefghijklmnopqrstuvwzyxABCDEFGHIJKLMNOPQRSTUVWXYZ_"

def cstrip(lines):
	d = ""	
	for l in lines:
		l = re.sub("#.*", "", l)
		l = re.sub("//.*", "", l)
		d += l + " "
	d = re.sub("\/\*.*?\*/", "", d) # remove /* */ comments
	d = d.replace("\t", " ") # tab to space
	d = re.sub("  *", " ", d) # remove double spaces
	d = re.sub("", "", d) # remove /* */ comments
	
	d = d.strip()
	
	# this eats up cases like 'n {'
	i = 1
	while i < len(d)-2:
		if d[i] == ' ':
			if not (d[i-1] in alphanum and d[i+1] in alphanum):
				d = d[:i] + d[i+1:]
		i += 1
	return d

f = ""
for filename in sys.argv[1:]:
	f += cstrip([l.strip() for l in file(filename)])

print '#define GAME_NETVERSION_HASH "%s"' % md5.new(f).hexdigest().lower()[16:]
