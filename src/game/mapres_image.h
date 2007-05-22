
// loads images from the map to textures
int img_init();

// returns the number of images in the map
int img_num();

// fetches the texture id for the image
int img_get(int index);


class mapres_image
{
public:
	int width;
	int height;
	int image_data;
};
