
#include "localization.hpp"

static unsigned str_hash(const char *str)
{
	unsigned hash = 5381;
	for(; *str; str++)
		hash = ((hash << 5) + hash) + (*str); /* hash * 33 + c */
	return hash;
}

const char *localize(const char *str)
{
	const char *new_str = localization.find_string(str_hash(str));
	//dbg_msg("", "no localization for '%s'", str);
	return new_str ? new_str : str;
}

LOC_CONSTSTRING::LOC_CONSTSTRING(const char *str)
{
	default_str = str;
	hash = str_hash(default_str);
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
	s.hash = str_hash(org_str);
	s.replacement = new_str;
	strings.add(s);
	
	current_version++;
}

const char *LOCALIZATIONDATABASE::find_string(unsigned hash)
{
	array<STRING>::range r = ::find(strings.all(), hash);
	if(r.empty())
		return 0;
	return r.front().replacement;
}
