#!/bin/env python3
# coding: utf-8
from socket import *
import sys
import threading
import time

import random

NUM_MASTERSERVERS = 4
MASTERSERVER_PORT = 8283

TIMEOUT = 2

SERVERTYPE_NORMAL = 0

PACKET_GETLIST = b"\xff\xff\xff\xffreq2"
PACKET_LIST = b"\xff\xff\xff\xfflis2"

PACKET_GETINFO = b"\xff\xff\xff\xffgie3"
PACKET_INFO = b"\xff\xff\xff\xffinf3"

def pack_control_msg_with_token(token_srv,token_cl):
	b = [0,0,1,0,0,5,0,0,0]
	b[0] = (token_srv >> 12) & 0xff
	b[1] = (token_srv >>  4) & 0xff
	b[2] |= (token_srv <<  4) & 0xff
	b[6] |= (token_cl >> 16) & 0x0f
	b[7] |= (token_cl >> 8) & 0xff
	b[8] |= token_cl & 0xff
	return bytes(b)

def unpack_control_msg_with_token(msg):
	b = list(msg)
	token_cl = (b[0] << 12) + (b[1] << 4) + (b[2] >> 4)
	token_srv = (b[6] << 16) + (b[7] << 8) + b[8]
	return token_cl,token_srv

def header_connless(token_srv, token_cl):
	b = [0,0,8,16,0,0]
	b[0] = (token_srv >> 12) & 0xff
	b[1] = (token_srv >>  4) & 0xff
	b[2] |= (token_srv <<  4) & 0xff
	b[3] |= (token_cl >> 16) & 0x0f
	b[4] |= (token_cl >> 8) & 0xff
	b[5] |= token_cl & 0xff
	return bytes(b)

class Server_Info(threading.Thread):

	def __init__(self, address):
		self.address = address
		self.finished = False
		threading.Thread.__init__(self, target = self.run)

	def run(self):
		self.info = None
		self.info = get_server_info(self.address)
		self.finished = True

def unpack_int(b):
	l = list(b[:5])
	i = 0
	Sign = (l[i]>>6)&1;
	res = l[i] & 0x3F;

	for _ in (0,):
		if not (l[i]&0x80):
			break
		i+=1
		res |= (l[i]&(0x7F))<<(6);

		if not (l[i]&0x80):
			break
		i+=1
		res |= (l[i]&(0x7F))<<(6+7);

		if not (l[i]&0x80):
			break
		i+=1
		res |= (l[i]&(0x7F))<<(6+7+7);

		if not (l[i]&0x80):
			break
		i+=1
		res |= (l[i]&(0x7F))<<(6+7+7+7);

	i += 1;
	res ^= -Sign
	return res, b[i:]

def get_server_info(address):
	try:
		sock = socket(AF_INET, SOCK_DGRAM)
		sock.settimeout(TIMEOUT)
		token = random.randrange(0x100000)
		sock.sendto(pack_control_msg_with_token(-1,token),address)
		data, addr = sock.recvfrom(1024)
		token_cl, token_srv = unpack_control_msg_with_token(data)
		assert(token_cl == token)
		sock.sendto(header_connless(token_srv, token_cl) + PACKET_GETINFO + b'\x00', address)
		data, addr = sock.recvfrom(1024)
		head = 	header_connless(token_cl, token_srv) + PACKET_INFO + b'\x00'
		assert(data[:len(head)] == head)
		sock.close()

		data = data[len(head):] # skip header

		slots = data.split(b"\x00", maxsplit=5)

		server_info = {}
		server_info["version"] = slots[0].decode()
		server_info["name"] = slots[1].decode()
		server_info["hostname"] = slots[2].decode()
		server_info["map"] = slots[3].decode()
		server_info["gametype"] = slots[4].decode()
		data = slots[5]
		# these integers should fit in one byte each
		server_info["flags"], server_info["skill"], server_info["num_players"], server_info["max_players"], server_info["num_clients"], server_info["max_clients"] = tuple(data[:6])
		data = data[6:]
		server_info["players"] = []

		for i in range(server_info["num_clients"]):
			player = {}
			slots = data.split(b"\x00", maxsplit=2)
			player["name"] = slots[0].decode()
			player["clan"] = slots[1].decode()
			data = slots[2]
			player["country"], data = unpack_int(data)
			player["score"], data = unpack_int(data)
			is_player, data = unpack_int(data)
			if is_player:
				player["player"] = True
			else:
				player["player"] = False
			server_info["players"].append(player)

		return server_info

	except:
		# import traceback
		# traceback.print_exc()
		sock.close()
		return None


class Master_Server_Info(threading.Thread):

	def __init__(self, address):
		self.address = address
		self.finished = False
		threading.Thread.__init__(self, target = self.run)

	def run(self):
		self.servers = get_list(self.address)
		self.finished = True


def get_list(address):
	servers = []

	try:
		sock = socket(AF_INET, SOCK_DGRAM)
		sock.settimeout(TIMEOUT)

		token = random.randrange(0x100000)
		sock.sendto(pack_control_msg_with_token(-1,token),address)
		data, addr = sock.recvfrom(1024)
		token_cl, token_srv = unpack_control_msg_with_token(data)
		assert(token_cl == token)
		sock.sendto(header_connless(token_srv, token_cl) + PACKET_GETLIST, addr)
		head = 	header_connless(token_cl, token_srv) + PACKET_LIST

		while 1:
			data, addr = sock.recvfrom(1024)
			assert(data[:len(head)] == head)

			data = data[len(head):]
			num_servers = len(data) // 18

			for n in range(0, num_servers):
				if data[n*18:n*18+12] == b"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xff\xff":
					ip = ".".join(map(str, data[n*18+12:n*18+16]))
				else:
					ip = ":".join(map(str, data[n*18:n*18+16]))
				port = ((data[n*18+16])<<8) + data[n*18+17]
				servers += [(ip, port)]

	except:
		# import traceback
		# traceback.print_exc()
		sock.close()

	return servers



master_servers = []

for i in range(1, NUM_MASTERSERVERS+1):
	m = Master_Server_Info(("master%d.teeworlds.com"%i, MASTERSERVER_PORT))
	master_servers.append(m)
	m.start()
	time.sleep(0.001) # avoid issues

servers = []

while len(master_servers) != 0:
	if master_servers[0].finished == True:
		if master_servers[0].servers:
			servers += master_servers[0].servers
		del master_servers[0]
	time.sleep(0.001) # be nice

servers_info = []

print(str(len(servers)) + " servers")

for server in servers:
	s = Server_Info(server)
	servers_info.append(s)
	s.start()
	time.sleep(0.001) # avoid issues

num_players = 0
num_clients = 0

player_names = []
while len(servers_info) != 0:
	if servers_info[0].finished == True:

		if servers_info[0].info:
			num_players += servers_info[0].info["num_players"]
			num_clients += servers_info[0].info["num_clients"]

		del servers_info[0]

	time.sleep(0.001) # be nice

print(str(num_players) + " players and " + str(num_clients-num_players) + " spectators")
