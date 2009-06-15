
#include "localization.hpp"
#include <base/tl/algorithm.hpp>

extern "C" {
#include <engine/e_linereader.h>
}

const char *localize(const char *str)
{
	const char *new_str = localization.find_string(str_quickhash(str));
	return new_str ? new_str : str;
}

LOC_CONSTSTRING::LOC_CONSTSTRING(const char *str)
{
	default_str = str;
	hash = str_quickhash(default_str);
	version = -1;
}

void LOC_CONSTSTRING::reload()
{
	version = localization.version();
	const char *new_str = localization.find_string(hash);
	current_str = new_str;
	if(!current_str)
		current_str = default_str;
}

LOCALIZATIONDATABASE::LOCALIZATIONDATABASE()
{
	current_version = 0;
}

void LOCALIZATIONDATABASE::add_string(const char *org_str, const char *new_str)
{
	STRING s;
	s.hash = str_quickhash(org_str);
	s.replacement = new_str;
	strings.add(s);
}

bool LOCALIZATIONDATABASE::load(const char *filename)
{
	// empty string means unload
	if(filename[0] == 0)
	{
		strings.clear();
		return true;
	}
	
	LINEREADER lr;
	IOHANDLE io = io_open(filename, IOFLAG_READ);
	if(!io)
		return false;
	
	dbg_msg("localization", "loaded '%s'", filename);
	strings.clear();
	
	linereader_init(&lr, io);
	char *line;
	while((line = linereader_get(&lr)))
	{
		if(!str_length(line))
			continue;
			
		if(line[0] == '#') // skip comments
			continue;
			
		char *replacement = linereader_get(&lr);
		if(!replacement)
		{
			dbg_msg("", "unexpected end of file");
			break;
		}
		
		if(replacement[0] != '=' || replacement[1] != '=' || replacement[2] != ' ')
		{
			dbg_msg("", "malform replacement line for '%s'", line);
			continue;
		}

		replacement += 3;
		localization.add_string(line, replacement);
	}
	
	current_version++;
	return true;
}

const char *LOCALIZATIONDATABASE::find_string(unsigned hash)
{
	STRING s;
	s.hash = hash;
	sorted_array<STRING>::range r = ::find_binary(strings.all(), s);
	if(r.empty())
		return 0;
	return r.front().replacement;
}

LOCALIZATIONDATABASE localization;
