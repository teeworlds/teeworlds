#include "../vmath.h"

// do this better and nicer
typedef struct 
{
	int org_texture;
	int color_texture;
	char name[31];
	const char term[1];
} skin;

vec4 skin_get_color(int v);
void skin_init();
int skin_num();
const skin *skin_get(int index);
int skin_find(const char *name);
