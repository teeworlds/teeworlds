#include <baselib/vmath.h>

struct mapres_collision
{
	int width;
	int height;
	int data_index;
};

int col_init(int dividor);
int col_check_point(int x, int y);
bool col_intersect_line(baselib::vec2 pos0, baselib::vec2 pos1, baselib::vec2 *out);
