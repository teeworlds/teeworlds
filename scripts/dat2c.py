import sys

data = file(sys.argv[1], "rb").read()

i = 0
print "unsigned char", sys.argv[2], "[] = {"
print str(ord(data[0])),
for d in data[1:]:
	s = ","+str(ord(d))
	print s,
	i += len(s)+1

	if i >= 70:
		print ""
		i = 0
print ""
print "};"
