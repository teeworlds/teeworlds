#include <base/tl/array.hpp>

class LOCALIZATIONDATABASE
{
	class STRING
	{
	public:
		unsigned hash;
		string replacement;
		
		bool operator ==(unsigned h) const { return hash == h; }
		
	};

	array<STRING> strings;
	int current_version;
	
public:
	LOCALIZATIONDATABASE();

	bool load(const char *filename);

	int version() { return current_version; }
	
	void add_string(const char *org_str, const char *new_str);
	const char *find_string(unsigned hash);
};

static LOCALIZATIONDATABASE localization;


class LOC_CONSTSTRING
{
	const char *default_str;
	const char *current_str;
	unsigned hash;
	int version;
public:
	LOC_CONSTSTRING(const char *str);
	void reload();
	
	inline operator const char *()
	{
		if(version != localization.version())
			reload();
		return current_str;
	}
};
