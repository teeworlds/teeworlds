/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#include "../g_vmath.h"

// do this better and nicer
typedef struct 
{
	int org_texture;
	int color_texture;
	char name[31];
	char term[1];
	vec3 blood_color;
} skin;

vec4 skin_get_color(int v);
void skin_init();
int skin_num();
const skin *skin_get(int index);
int skin_find(const char *name);


vec3 hsl_to_rgb(vec3 in);
