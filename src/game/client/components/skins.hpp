#include <base/vmath.hpp>
#include <game/client/component.hpp>

class SKINS : public COMPONENT
{
public:
	// do this better and nicer
	typedef struct 
	{
		int org_texture;
		int color_texture;
		char name[31];
		char term[1];
		vec3 blood_color;
	} SKIN;

	SKINS();
	
	void init();
	
	vec4 get_color(int v);
	int num();
	const SKIN *get(int index);
	int find(const char *name);
	
private:
	enum
	{
		MAX_SKINS=256,
	};

	SKIN skins[MAX_SKINS];
	int num_skins;

	static void skinscan(const char *name, int is_dir, void *user);
};
