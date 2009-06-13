/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#ifndef ENGINE_IF_GFX_H
#define ENGINE_IF_GFX_H

/*
	Title: Graphics
*/

/*
	Section: Structures
*/

/*
	Structure: FONT
*/
struct FONT;

/*
	Structure: IMAGE_INFO
*/
typedef struct
{
	/* Variable: width
		Contains the width of the image */
	int width;
	
	/* Variable: height
		Contains the height of the image */
	int height;
	
	/* Variable: format
		Contains the format of the image. See <Image Formats> for more information. */
	int format;

	/* Variable: data
		Pointer to the image data. */
	void *data;
} IMAGE_INFO;

/*
	Structure: VIDEO_MODE
*/
typedef struct 
{
	int width, height;
	int red, green, blue;
} VIDEO_MODE;

/*
	Section: Functions
*/

/*
	Group: Quads
*/

/*
	Function: gfx_quads_begin
		Begins a quad drawing session.
		
	Remarks:
		This functions resets the rotation, color and subset.
		End the session by using <gfx_quads_end>.
		You can't change texture or blending mode during a session.
		
	See Also:
		<gfx_quads_end>
*/
void gfx_quads_begin();

/*
	Function: gfx_quads_end
		Ends a quad session.
		
	See Also:
		<gfx_quads_begin>
*/
void gfx_quads_end();

/*
	Function: gfx_quads_setrotation
		Sets the rotation to use when drawing a quad.
		
	Arguments:
		angle - Angle in radians.
		
	Remarks:
		The angle is reset when <gfx_quads_begin> is called.
*/
void gfx_quads_setrotation(float angle);

/*
	Function: gfx_quads_setsubset
		Sets the uv coordinates to use.
		
	Arguments:
		tl_u - Top-left U value.
		tl_v - Top-left V value.
		br_u - Bottom-right U value.
		br_v - Bottom-right V value.
		
	Remarks:
		O,0 is top-left of the texture and 1,1 is bottom-right.
		The color is reset when <gfx_quads_begin> is called.
*/
void gfx_quads_setsubset(float tl_u, float tl_v, float br_u, float br_v);

/*
	Function: gfx_quads_setsubset_free
		TODO
		
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
void gfx_quads_setsubset_free(
	float x0, float y0,
	float x1, float y1,
	float x2, float y2,
	float x3, float y3);

/*
	Function: gfx_quads_drawTL
		Draws a quad by specifying the top-left point.
		
	Arguments:
		x - X coordinate of the top-left corner.
		y - Y coordinate of the top-left corner.
		width - Width of the quad.
		height - Height of the quad.
		
	Remarks:
		Rotation still occurs from the center of the quad.
		You must call <gfx_quads_begin> before calling this function.

	See Also:
		<gfx_quads_draw, gfx_quads_draw_freeform>
*/
void gfx_quads_drawTL(float x, float y, float width, float height);

/*
	Function: gfx_quads_draw
		Draws a quad by specifying the center point.
		
	Arguments:
		x - X coordinate of the center.
		y - Y coordinate of the center.
		width - Width of the quad.
		height - Height of the quad.

	Remarks:
		You must call <gfx_quads_begin> before calling this function.

	See Also:
		<gfx_quads_drawTL, gfx_quads_draw_freeform>
*/
void gfx_quads_draw(float x, float y, float w, float h);

/*
	Function: gfx_quads_draw_freeform
		Draws a quad by specifying the corner points.
	
	Arguments:
		x0, y0 - Coordinates of the upper left corner.
		x1, y1 - Coordinates of the upper right corner.
		x2, y2 - Coordinates of the lower left corner. // TODO: DOUBLE CHECK!!!
		x3, y3 - Coordinates of the lower right corner. // TODO: DOUBLE CHECK!!!
	
	See Also:
		<gfx_quads_draw, gfx_quads_drawTL>
*/
void gfx_quads_draw_freeform(
	float x0, float y0,
	float x1, float y1,
	float x2, float y2,
	float x3, float y3);

/*
	Function: gfx_quads_text
		TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
void gfx_quads_text(float x, float y, float size, float r, float g, float b, float a, const char *text);

/*
	Group: Lines
*/

/*
	Function: gfx_lines_begin
		TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
void gfx_lines_begin();

/*
	Function: gfx_lines_draw
		TODO
		
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
void gfx_lines_draw(float x0, float y0, float x1, float y1);

/*
	Function: gfx_minimize
		Minimizes the window.
		
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
void gfx_minimize();

/*
	Function: gfx_minimize
		Maximizes the window.
		
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
void gfx_maximize();

/*
	Function: gfx_lines_end
		TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
void gfx_lines_end();

/*
	Group: Text
*/


/*
	Function: gfx_text
		TODO
	
	Arguments:
		arg1 - desc
	
	Returns:
		returns the number of lines written
	See Also:
		<other_func>
*/
void gfx_text(void *font, float x, float y, float size, const char *text, int max_width);

/*
	Function: gfx_text_width
		TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
float gfx_text_width(void *font, float size, const char *text, int length);

/*
	Function: gfx_text_color
		TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
void gfx_text_color(float r, float g, float b, float a);

/*
	Function: gfx_text_set_default_font
		TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
void gfx_text_set_default_font(struct FONT *font);

/*
	Group: Other
*/

/*
	Function: gfx_get_video_modes
		Fetches a list of all the available video modes.
		
	Arguments:
		list - An array to recive the modes. Must be able to contain maxcount number of modes.
		maxcount - The maximum number of modes to fetch.
	
	Returns:
		The number of video modes that was fetched.
*/
int gfx_get_video_modes(VIDEO_MODE *list, int maxcount);

/* image loaders */

/*
	Function: gfx_load_png
		Loads a PNG image from disk.
	
	Arguments:
		img - Pointer to an structure to be filled out with the image information.
		filename - Filename of the image to load.
	
	Returns:
		Returns non-zero on success and zero on error.
	
	Remarks:
		The caller are responsible for cleaning up the allocated memory in the IMAGE_INFO structure.
	
	See Also:
		<gfx_load_texture>
*/
int gfx_load_png(IMAGE_INFO *img, const char *filename);

/* textures */
/*
	Function: gfx_load_texture
		Loads a texture from a file. TGA and PNG supported.
	
	Arguments:
		filename - Null terminated string to the file to load.
		store_format - What format to store on gfx card as.
		flags - controls how the texture is uploaded
	
	Returns:
		An ID to the texture. -1 on failure.

	See Also:
		<gfx_unload_texture, gfx_load_png>
*/
int gfx_load_texture(const char *filename, int store_format, int flags);

/*
	Function: gfx_load_texture_raw
		Loads a texture from memory.
	
	Arguments:
		w - Width of the texture.
		h - Height of the texture.
		data - Pointer to the pixel data.
		format - Format of the pixel data.
		store_format - The format to store the texture on the graphics card.
		flags - controls how the texture is uploaded
	
	Returns:
		An ID to the texture. -1 on failure.
		
	Remarks:
		The pixel data should be in RGBA format with 8 bit per component.
		So the total size of the data should be w*h*4.
		
	See Also:
		<gfx_unload_texture>
*/
int gfx_load_texture_raw(int w, int h, int format, const void *data, int store_format, int flags);

/*
	Function: gfx_texture_set
		Sets the active texture.
	
	Arguments:
		id - ID to the texture to set.
*/
void gfx_texture_set(int id);

/*
	Function: gfx_unload_texture
		Unloads a texture.
	
	Arguments:
		id - ID to the texture to unload.
		
	See Also:
		<gfx_load_texture_tga>, <gfx_load_texture_raw>
		
	Remarks:
		NOT IMPLEMENTED
*/
int gfx_unload_texture(int id);

/*
	Function: gfx_clear
		Clears the screen with the specified color.
	
	Arguments:
		r - Red component.
		g - Green component.
		b - Red component.
	
	Remarks:
		The value of the components are given in 0.0 - 1.0 ranges.
*/
void gfx_clear(float r, float g, float b);

/*
	Function: gfx_screenaspect
		Returns the aspect ratio between width and height.

	See Also:
		<gfx_screenwidth>, <gfx_screenheight>
*/
float gfx_screenaspect();

/*
	Function: gfx_screenwidth
		Returns the screen width.
	
	See Also:
		<gfx_screenheight>
*/
int gfx_screenwidth();

/*
	Function: gfx_screenheight
		Returns the screen height.
	
	See Also:
		<gfx_screenwidth>
*/
int gfx_screenheight();

/*
	Function: gfx_mapscreen
		Specifies the coordinate system for the screen.
		
	Arguments:
		tl_x - Top-left X
		tl_y - Top-left Y
		br_x - Bottom-right X
		br_y - Bottom-right y
*/
void gfx_mapscreen(float tl_x, float tl_y, float br_x, float br_y);

/*
	Function: gfx_blend_normal
		Set the active blending mode to normal (src, 1-src).

	Remarks:
		This must be used before calling <gfx_quads_begin>.
		This is equal to glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA).
	
	See Also:
		<gfx_blend_additive,gfx_blend_none>
*/
void gfx_blend_normal();

/*
	Function: gfx_blend_additive
		Set the active blending mode to additive (src, one).

	Remarks:
		This must be used before calling <gfx_quads_begin>.
		This is equal to glBlendFunc(GL_SRC_ALPHA, GL_ONE).
	
	See Also:
		<gfx_blend_normal,gfx_blend_none>
*/
void gfx_blend_additive();

/*
	Function: gfx_blend_none
		Disables blending

	Remarks:
		This must be used before calling <gfx_quads_begin>.
	
	See Also:
		<gfx_blend_normal,gfx_blend_additive>
*/
void gfx_blend_none();


/*
	Function: gfx_setcolorvertex
		Sets the color of a vertex.
		
	Arguments:
		i - Index to the vertex.
		r - Red value.
		g - Green value.
		b - Blue value.
		a - Alpha value.
		
	Remarks:
		The color values are from 0.0 to 1.0.
		The color is reset when <gfx_quads_begin> is called.
*/
void gfx_setcolorvertex(int i, float r, float g, float b, float a);

/*
	Function: gfx_setcolor
		Sets the color of all the vertices.
		
	Arguments:
		r - Red value.
		g - Green value.
		b - Blue value.
		a - Alpha value.
		
	Remarks:
		The color values are from 0.0 to 1.0.
		The color is reset when <gfx_quads_begin> is called.
*/
void gfx_setcolor(float r, float g, float b, float a);

/*
	Function: gfx_getscreen
		TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
void gfx_getscreen(float *tl_x, float *tl_y, float *br_x, float *br_y);

/*
	Function: gfx_memory_usage
		TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
int gfx_memory_usage();

/*
	Function: gfx_screenshot
		TODO		
	
	Arguments:
		filename - desc
	
	Returns:

	See Also:
		<other_func>
*/
void gfx_screenshot();

/*
	Function: gfx_screenshot_direct
		TODO		
	
	Arguments:
		filename - desc
	
	Returns:

	See Also:
		<other_func>
*/
void gfx_screenshot_direct(const char *filename);

/*
	Function: gfx_clip_enable
		TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
void gfx_clip_enable(int x, int y, int w, int h);

/*
	Function: gfx_clip_disable
		TODO	
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
void gfx_clip_disable();


enum
{
	TEXTFLAG_RENDER=1,
	TEXTFLAG_ALLOW_NEWLINE=2,
	TEXTFLAG_STOP_AT_END=4
};

typedef struct
{
	int flags;
	int line_count;
	int charcount;
	
	float start_x;
	float start_y;
	float line_width;
	float x, y;
	
	struct FONT *font;
	float font_size;
} TEXT_CURSOR;

void gfx_text_set_cursor(TEXT_CURSOR *cursor, float x, float y, float font_size, int flags);
void gfx_text_ex(TEXT_CURSOR *cursor, const char *text, int length);


struct FONT *gfx_font_load(const char *filename);
void gfx_font_destroy(struct FONT *font);

#endif
