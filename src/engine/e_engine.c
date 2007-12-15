/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#include <string.h>
#include <stdio.h>

#include <engine/e_system.h>
#include <engine/e_interface.h>
#include <engine/e_config.h>

static char application_save_path[512] = {0};

const char *engine_savepath(const char *filename, char *buffer, int max)
{
	sprintf(buffer, "%s/%s", application_save_path, filename);
	return buffer;
}

void engine_init(const char *appname, int argc, char **argv)
{
	/* init the network */
	net_init();

	/* create storage location */
	{
		char path[1024] = {0};
		fs_storage_path(appname, application_save_path, sizeof(application_save_path));
		if(fs_makedir(application_save_path) == 0)
		{		
			strcpy(path, application_save_path);
			strcat(path, "/screenshots");
			fs_makedir(path);
		}
	}
	
	/* reset the config */
	config_reset();
	
	/* load the configuration */
	{
		int i;
		const char *config_filename = "default.cfg";
		char buf[1024];
		for(i = 1; i < argc; i++)
		{
			if(argv[i][0] == '-' && argv[i][1] == 'f' && argv[i][2] == 0 && argc - i > 1)
			{
				config_filename = argv[i+1];
				i++;
			}
		}

		config_load(engine_savepath(config_filename, buf, sizeof(buf)));
	}
	
	/* search arguments for overrides */
	{
		int i;
		for(i = 1; i < argc; i++)
			config_set(argv[i]);
	}
}

void engine_writeconfig()
{
	char buf[1024];
	config_save(engine_savepath("default.cfg", buf, sizeof(buf)));
}
