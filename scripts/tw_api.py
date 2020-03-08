#!/usr/bin/env python3
# coding: utf-8
from socket import socket, AF_INET, SOCK_DGRAM
import sys
import threading
import time

import random

NUM_MASTERSERVERS = 4
MASTERSERVER_PORT = 8283


TIMEOUT = 2
BUFFER_SIZE = 4096
NUM_RETRIES =  3


# retries after waiting a specific time if the simple retry method failed.
# SLEEP_SECS is multiplied with 2 on every retry
FORCE_SLEEP = True
SLEEP_SECS = 2.0
NUM_SLEEP_RETRIES = 2

# src/mastersrv/mastersrv.h
PACKET_GETLIST = b"\xff\xff\xff\xffreq2"
PACKET_LIST = b"\xff\xff\xff\xfflis2"

PACKET_GETINFO = b"\xff\xff\xff\xffgie3"
PACKET_INFO = b"\xff\xff\xff\xffinf3"


# see CNetBase::SendControlMsgWithToken
def pack_control_msg_with_token(token_srv,token_cl):
	NET_PACKETFLAG_CONTROL = 1
	NET_CTRLMSG_TOKEN = 5
	NET_TOKENREQUEST_DATASIZE = 512
	b = [0]*(4 + 3 + NET_TOKENREQUEST_DATASIZE)
	# Header
	b[0] = (NET_PACKETFLAG_CONTROL<<2)&0xfc
	b[3] = (token_srv >> 24) & 0xff
	b[4] = (token_srv >> 16) & 0xff
	b[5] = (token_srv >> 8) & 0xff
	b[6] = (token_srv) & 0xff
	# Data
	b[7] = NET_CTRLMSG_TOKEN
	b[8] = (token_cl >> 24) & 0xff
	b[9] = (token_cl >> 16) & 0xff
	b[10] = (token_cl >> 8) & 0xff
	b[11] = (token_cl) & 0xff
	return bytes(b)


def unpack_control_msg_with_token(msg):
	b = list(msg)
	token_cl = (b[3] << 24) + (b[4] << 16) + (b[5] << 8) + (b[6])
	token_srv = (b[8] << 24) + (b[9] << 16) + (b[10] << 8) + (b[11])
	return token_cl,token_srv


# CNetBase::SendPacketConnless
def header_connless(token_srv, token_cl):
	NET_PACKETFLAG_CONNLESS = 8
	NET_PACKETVERSION = 1
	b = [0]*9
	# Header
	b[0] = ((NET_PACKETFLAG_CONNLESS<<2)&0xfc) | (NET_PACKETVERSION&0x03)
	b[1] = (token_srv >> 24) & 0xff
	b[2] = (token_srv >> 16) & 0xff
	b[3] = (token_srv >> 8) & 0xff
	b[4] = (token_srv) & 0xff
	# ResponseToken
	b[5] = (token_cl >> 24) & 0xff
	b[6] = (token_cl >> 16) & 0xff
	b[7] = (token_cl >> 8) & 0xff
	b[8] = (token_cl) & 0xff
	return bytes(b)


# CVariableInt::Unpack from src/engine/shared/compression.cpp
def unpack_int(b):
	l = list(b[:5])
	i = 0
	Sign = (l[i]>>6)&1
	res = l[i] & 0x3F

	for _ in (0,):
		if not (l[i]&0x80):
			break
		i+=1
		res |= (l[i]&(0x7F))<<(6)

		if not (l[i]&0x80):
			break
		i+=1
		res |= (l[i]&(0x7F))<<(6+7)

		if not (l[i]&0x80):
			break
		i+=1
		res |= (l[i]&(0x7F))<<(6+7+7)

		if not (l[i]&0x80):
			break
		i+=1
		res |= (l[i]&(0x7F))<<(6+7+7+7)

	i += 1
	res ^= -Sign
	return res, b[i:]


class Server_Info(threading.Thread):

	def __init__(self, address):
		self.address = address
		self.info = None
		self.finished = False
		threading.Thread.__init__(self, target = self.run)

	def  __str__(self):
		return str(self.info)
	
	def __getitem__(self, key):
		return self.info[key]

	def run(self):
		self.info = get_server_info(self.address)
		self.finished = True


def get_server_info(address):

	try:
		sock = socket(AF_INET, SOCK_DGRAM)
		sock.settimeout(TIMEOUT)
		
		# function definition
		def send_token(sock, address, timeout=TIMEOUT):
			token = random.randrange(0x100000000)

			# Token request
			sock.sendto(pack_control_msg_with_token(-1,token),address)

			# send and receive
			sock.settimeout(timeout)
			data, _ = sock.recvfrom(BUFFER_SIZE)

			# calculate expected token
			token_cl, token_srv = unpack_control_msg_with_token(data)

			# return, whether the correct token was received
			return token_cl == token, token_cl, token_srv
		

		retries = NUM_RETRIES
		token_success, token_cl, token_srv = send_token(sock, address)

		while not token_success and retries > 0:
			retries -= 1
			token_success, token_cl, token_srv = send_token(sock, address, sock.gettimeout() * 2)
		
		sock.settimeout(TIMEOUT) # reset to default value

		if retries == 0:
			raise ValueError(f"Failed to retrieve token from: {address}")
		
		# function definition
		def send_header(sock, address, token_cl, token_srv, timeout=TIMEOUT, sleep_secs = None):
			# Get info request
			sock.sendto(header_connless(token_srv, token_cl) + PACKET_GETINFO + b'\x00', address)
			sock.settimeout(timeout)
			if isinstance(sleep_secs, int):
				time.sleep(sleep_secs)
			data, _ = sock.recvfrom(BUFFER_SIZE)
			
			head = 	header_connless(token_cl, token_srv) + PACKET_INFO + b'\x00'

			received_head = data[:len(head)] # get header
			data = data[len(head):] # skip header

			return received_head == head, data # we don't need the header
		

		data_success, data = send_header(sock, address, token_cl, token_srv)
		retries = NUM_RETRIES

		while not data_success and retries > 0:
			retries -= 1
			data_success, data = send_header(sock, address, token_cl, token_srv, sock.gettimeout() * 2)

		sock.settimeout(TIMEOUT) # reset to default value

		# fast way didn't work, let's try the slow way
		if FORCE_SLEEP and not data_success and retries == 0:
			retries = NUM_SLEEP_RETRIES
			sleep_secs = SLEEP_SECS / 2.0

			while not data_success and retries > 0:
				retries -= 1
				sleep_secs *= 2
				data_success, data = send_header(sock, address, token_cl, token_srv, sock.gettimeout() * 2.0, sleep_secs=sleep_secs)
			if not data_success and retries == 0:
				raise ValueError(f"Failed to retrieve server info from {address}")
		
		elif not data_success and retries == 0:
			raise ValueError(f"Failed to retrieve server info from {address}")

		slots = data.split(b"\x00", maxsplit=5)

		server_info = {}
		server_info["address"] = address
		server_info["version"] = slots[0].decode()
		server_info["name"] = slots[1].decode()
		server_info["hostname"] = slots[2].decode()
		server_info["map"] = slots[3].decode()
		server_info["gametype"] = slots[4].decode()
		data = slots[5]

		# these integers should fit in one byte each
		server_info["flags"], server_info["skill"] = tuple(data[:2])
		data = data[2:]

		server_info["num_players"], data = unpack_int(data)
		server_info["max_players"], data = unpack_int(data)
		server_info["num_clients"], data = unpack_int(data)
		server_info["max_clients"], data = unpack_int(data)
		server_info["players"] = []

		for _ in range(server_info["num_clients"]):
			player = {}
			slots = data.split(b"\x00", maxsplit=2)
			player["name"] = slots[0].decode()
			player["clan"] = slots[1].decode()
			data = slots[2]
			player["country"], data = unpack_int(data)
			player["score"], data = unpack_int(data)
			player["player"], data = unpack_int(data)
			server_info["players"].append(player)

		return server_info
	except AssertionError as e:
		print(*e.args)
	except OSError as e: # Timeout
		#print('> Server %s did not answer' % (address,))
		pass
	except ValueError as e:
		print(e)
	except Exception as e:
		print(e)
		# print('> Server %s did something wrong here' % (address,))
		# import traceback
		# traceback.print_exc()
	finally:
		sock.close() # socket is closed in any case
	return None


class Master_Server_Info(threading.Thread):

	def __init__(self, address):
		self.address = address
		self.servers = []
		self.finished = False
		threading.Thread.__init__(self, target = self.run)

	def run(self):
		self.servers = get_list(self.address)
		self.finished = True


def get_list(address):
	servers = []
	answer = False

	try:
		sock = socket(AF_INET, SOCK_DGRAM)
		sock.settimeout(TIMEOUT)

		token = random.randrange(0x100000000)

		# Token request
		sock.sendto(pack_control_msg_with_token(-1,token),address)
		data, addr = sock.recvfrom(BUFFER_SIZE)
		token_cl, token_srv = unpack_control_msg_with_token(data)
		assert token_cl == token, "Master %s send wrong token: %d (%d expected)" % (address, token_cl, token)
		answer = True

		# Get list request
		sock.sendto(header_connless(token_srv, token_cl) + PACKET_GETLIST, addr)
		head = 	header_connless(token_cl, token_srv) + PACKET_LIST

		while 1:
			data, addr = sock.recvfrom(BUFFER_SIZE)
			# Header should keep consistent
			assert data[:len(head)] == head, "Master %s list header mismatch: %r != %r (expected)" % (address, data[:len(head)], head)

			data = data[len(head):]
			num_servers = len(data) // 18

			for n in range(0, num_servers):
				# IPv4
				if data[n*18:n*18+12] == b"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xff\xff":
					ip = ".".join(map(str, data[n*18+12:n*18+16]))
				# IPv6
				else:
					ip = ":".join(map(str, data[n*18:n*18+16]))
				port = ((data[n*18+16])<<8) + data[n*18+17]
				servers += [(ip, port)]

	except AssertionError as e:
		print(*e.args)
	except OSError as e: # Timeout
		if not answer:
			print('> Master %s did not answer' % (address,))
	except Exception as e:
		# import traceback
		# traceback.print_exc()
		print(e)
	finally:
		sock.close()

	return servers


if __name__ == '__main__':
	master_servers = []

	for i in range(1, NUM_MASTERSERVERS+1):
		m = Master_Server_Info(("master%d.teeworlds.com"%i, MASTERSERVER_PORT))
		master_servers.append(m)
		m.start()

	servers = set()

	while len(master_servers) != 0:
		master_servers[0].join()

		if master_servers[0].finished == True:
			if master_servers[0].servers:
				servers.update(master_servers[0].servers)
			del master_servers[0]


	servers_info = []

	print(len(servers), "servers")

	for server in servers:
		s = Server_Info(server)
		servers_info.append(s)
		s.start()

	num_players = 0
	num_clients = 0
	num_botplayers = 0
	num_botspectators = 0

	while len(servers_info) != 0:
		servers_info[0].join()
		
		if servers_info[0].finished == True:
			if servers_info[0].info:
				server_info = servers_info[0].info
				# check num/max validity
				if   server_info["num_players"] > server_info["max_players"] \
					or server_info["num_clients"] > server_info["max_clients"] \
					or server_info["max_players"] > server_info["max_clients"] \
					or server_info["num_players"] < 0 \
					or server_info["num_clients"] < 0 \
					or server_info["max_clients"] < 0 \
					or server_info["max_players"] < 0 \
					or server_info["max_clients"] > 64:
					server_info["bad"] = 'invalid num/max'
					print('> Server %s has %s' % (server_info["address"], server_info["bad"]))
				# check actual purity
				elif server_info["gametype"] in ('DM', 'TDM', 'CTF', 'LMS', 'LTS') \
					and server_info["max_players"] > 16:
					server_info["bad"] = 'too many players for vanilla'
					print('> Server %s has %s' % (server_info["address"], server_info["bad"]))

				else:
					num_players += server_info["num_players"]
					num_clients += server_info["num_clients"]
					for p in servers_info[0].info["players"]:
						if p["player"] == 2:
							num_botplayers += 1
						if p["player"] == 3:
							num_botspectators += 1

			del servers_info[0]


	print('%d players (%d bots) and %d spectators (%d bots)' % (num_players, num_botplayers, num_clients - num_players, num_botspectators))
