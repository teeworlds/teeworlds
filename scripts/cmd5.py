import hashlib, sys, re

alphanum = "0123456789abcdefghijklmnopqrstuvwzyxABCDEFGHIJKLMNOPQRSTUVWXYZ_"

def cstrip(lines):
	d = b""	
	for l in lines:
		l = re.sub(b"#.*", b"", l)
		l = re.sub(b"//.*", b"", l)
		d += l + b" "
	d = re.sub(b"\/\*.*?\*/", b"", d) # remove /* */ comments
	d = d.replace(b"\t", b" ") # tab to space
	d = re.sub(b"  *", b" ", d) # remove double spaces
	d = re.sub(b"", b"", d) # remove /* */ comments
	
	d = d.strip()
	
	# this eats up cases like 'n {'
	i = 1
	while i < len(d)-2:
		if d[i] == ' ':
			if not (d[i-1] in alphanum and d[i+1] in alphanum):
				d = d[:i] + d[i+1:]
		i += 1
	return d

f = b""
for filename in sys.argv[1:]:
	f += cstrip([l.strip() for l in open(filename, "rb")])

hash = hashlib.md5(f).hexdigest().lower()[16:]
# TODO: refactor hash that is equal to the 0.5 hash, remove when we 
# TODO: remove when we don't need it any more
if hash == "f16c2456fc487748":
	hash = "b67d1f1a1eea234e"
print('#define GAME_NETVERSION_HASH "%s"' % hash)
