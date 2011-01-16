#from random import choice

import random
import os

masterservers = ["localhost 8300"]

maps = [
	["dm1", "dm2", "dm6"],
	["dm1", "dm2", "dm6"],
	["ctf1", "ctf2", "ctf3"],
]

servernames = [
	"%s playhouse",
	"%s own server",
]

nicks = []
for l in file("scripts/nicks.txt"):
	nicks += l.replace(":port80c.se.quakenet.org 353 matricks_ = #pcw :", "").strip().split()
inick = 0

def get_nick():
	global inick, nicks
	inick = (inick+1)%len(nicks)
	return nicks[inick].replace("`", "\`")
	
for s in xrange(0, 350):
	cmd = "./fake_server_d_d "
	cmd += '-n "%s" ' % (random.choice(servernames) % get_nick())
	for m in masterservers:
		cmd += '-m %s '%m
	
	max = random.randint(2, 16)
	cmd += "-x %d " % max
	
	t = random.randint(0, 2)
	
	cmd += '-a "%s" ' % random.choice(maps[t])
	cmd += '-g %d ' % random.randint(0, 100)
	cmd += '-t %d ' % t # dm, tdm, ctf
	cmd += "-f %d " % random.randint(0, 1) # password protected
		
	for p in xrange(0, random.randint(0, max)):
		cmd += '-p "%s" %d ' % (get_nick(), random.randint(0, 20))
	
	print cmd
	os.popen2(cmd)

