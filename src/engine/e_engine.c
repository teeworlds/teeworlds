/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#include <string.h>
#include <stdio.h>

#include <engine/e_system.h>
#include <engine/e_interface.h>
#include <engine/e_config.h>
#include <engine/e_console.h>

static char application_save_path[512] = {0};

const char *engine_savepath(const char *filename, char *buffer, int max)
{
	sprintf(buffer, "%s/%s", application_save_path, filename);
	return buffer;
}

void engine_init(const char *appname, int argc, char **argv)
{
	dbg_msg("engine", "running on %s-%s-%s", CONF_FAMILY_STRING, CONF_PLATFORM_STRING, CONF_ARCH_STRING);
#ifdef CONF_ARCH_ENDIAN_LITTLE
	dbg_msg("engine", "arch is little endian");
#elif defined(CONF_ARCH_ENDIAN_BIG)
	dbg_msg("engine", "arch is big endian");
#else
	dbg_msg("engine", "unknown endian");
#endif

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

	/* init console */
	console_init();
	
	/* reset the config */
	config_reset();
	
	/* load the configuration */
	{
		int i;
		int abs = 0;
		const char *config_filename = "default.cfg";
		char buf[1024];
		for(i = 1; i < argc; i++)
		{
			if(argv[i][0] == '-' && argv[i][1] == 'f' && argv[i][2] == 0 && argc - i > 1)
			{
				config_filename = argv[i+1];
				abs = 1;
				i++;
			}
		}

		if(abs)
			config_load(config_filename);
		else
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


static int perf_tick = 1;
static PERFORMACE_INFO *current = 0;

void perf_init()
{
}

void perf_next()
{
	perf_tick++;
	current = 0;
}

void perf_start(PERFORMACE_INFO *info)
{
	if(info->tick != perf_tick)
	{
		info->parent = current;
		info->first_child = 0;
		info->next_child = 0;
		
		if(info->parent)
		{
			info->next_child = info->parent->first_child;
			info->parent->first_child = info;
		}
		
		info->tick = perf_tick;
		info->biggest = 0;
		info->total = 0;
	}
	
	current = info;
	current->start = time_get();
}

void perf_end()
{
	if(!current)
		return;
		
	current->last_delta = time_get()-current->start;
	current->total += current->last_delta;
	
	if(current->last_delta > current->biggest)
		current->biggest = current->last_delta;
	
	current = current->parent;
}

static void perf_dump_imp(PERFORMACE_INFO *info, int indent)
{
	char buf[512] = {0};
	int64 freq = time_freq();
	int i;
	
	for(i = 0; i < indent; i++)
		buf[i] = ' ';
	
	sprintf(&buf[indent], "%-20s %8.2f %8.2f", info->name, info->total*1000/(float)freq, info->biggest*1000/(float)freq);
	dbg_msg("perf", "%s", buf);
	
	info = info->first_child;
	while(info)
	{
		perf_dump_imp(info, indent+2);
		info = info->next_child;
	}
}

void perf_dump(PERFORMACE_INFO *top)
{
	perf_dump_imp(top, 0);
}
