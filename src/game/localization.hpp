#include <base/tl/string.hpp>
#include <base/tl/sorted_array.hpp>

class LOCALIZATIONDATABASE
{
	class STRING
	{
	public:
		unsigned hash;
		
		// TODO: do this as an const char * and put everything on a incremental heap
		string replacement;

		bool operator <(const STRING &other) const { return hash < other.hash; }
		bool operator <=(const STRING &other) const { return hash <= other.hash; }
		bool operator ==(const STRING &other) const { return hash == other.hash; }
	};

	sorted_array<STRING> strings;
	int current_version;
	
public:
	LOCALIZATIONDATABASE();

	bool load(const char *filename);

	int version() { return current_version; }
	
	void add_string(const char *org_str, const char *new_str);
	const char *find_string(unsigned hash);
};

extern LOCALIZATIONDATABASE localization;

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


extern const char *localize(const char *str);
