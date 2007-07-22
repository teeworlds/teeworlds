#ifndef __CLIENT_H
#define __CLIENT_H

#include <engine/network.h>
// --- client ---
// TODO: remove this class
class client
{
public:
	int info_request_begin;
	int info_request_end;
	
	int snapshot_part;

	int debug_font; // TODO: rfemove this line

	// data to hold three snapshots
	// previous, 

	void send_info();

	void send_entergame();

	void send_error(const char *error);

	void send_input();
	
	void disconnect();
	
	bool load_data();
	
	void debug_render();
	
	void render();
	
	void run(const char *direct_connect_server);
	
	void error(const char *msg);
	
	void serverbrowse_request(int id);
	
	void serverbrowse_update();
	void process_packet(NETPACKET *packet);
	
	void pump_network();	
};

#endif
