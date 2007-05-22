#include <cstdio>
#include <cstdlib>

#include "masterserver.h"

int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		puts("Usage: masterserver <port>    (this will bind the server to the port specified (both udp and tcp).");
		return -1;
	}

	int port = atoi(argv[1]);

	CMasterServer masterServer;
	masterServer.Init(port);

	while (1)
	{
		masterServer.Tick();

		thread_sleep(10);
	}

	masterServer.Shutdown();
	
	return 0;
}
