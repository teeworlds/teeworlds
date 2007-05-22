#include <baselib/system.h>
#include <baselib/network.h>

#include <stdio.h>
#include "versions.h"

using namespace baselib;

extern int client_main(int argc, char **argv);
extern int editor_main(int argc, char **argv);
extern int server_main(int argc, char **argv);

int main(int argc, char **argv)
{
	// search for server or editor argument
	for(int i = 1; i < argc; i++)
	{
		if(argv[i][0] == '-' && argv[i][1] == 'v' && argv[i][2] == 0)
		{
			printf(TEEWARS_VERSION"\n");
			return 0;
		}
		if(argv[i][0] == '-' && argv[i][1] == 's' && argv[i][2] == 0)
			return server_main(argc, argv);
		else if(argv[i][0] == '-' && argv[i][1] == 'e' && argv[i][2] == 0)
			return editor_main(argc, argv);
	}

	// no specific parameters, start the client
	return client_main(argc, argv);
}
