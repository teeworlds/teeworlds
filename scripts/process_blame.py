import sys
user_map = {"kma":"matricks", "teetow":"matricks", "jlha":"matricks", "jdv":"void", "jaf":"serp"}
users = {}
for line in sys.stdin:
	fields = line.split()
	name = user_map.get(fields[1], fields[1])
	users[name] = users.get(name, 0) + 1
	
total = reduce(lambda x,y: x+y, users.values())
for u in users:
	print "%6s %6d %s" % ("%03.2f"%(users[u]*100.0/total), users[u], u)
print "%03.2f %6d %s" % (100, total, "Total")
