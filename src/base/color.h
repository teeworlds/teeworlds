
#ifndef BASE_COLOR_H
#define BASE_COLOR_H

#include "vmath.h"

/*
	Title: Color handling
*/

/*
	Function: HueToRgb
		Converts Hue to RGB
*/
inline float HueToRgb(float v1, float v2, float h)
{
	if(h < 0.0f) h += 1;
	if(h > 1.0f) h -= 1;
	if((6.0f * h) < 1.0f) return v1 + (v2 - v1) * 6.0f * h;
	if((2.0f * h) < 1.0f) return v2;
	if((3.0f * h) < 2.0f) return v1 + (v2 - v1) * ((2.0f/3.0f) - h) * 6.0f;
	return v1;
}

/*
	Function: HslToRgb
		Converts HSL to RGB
*/
inline vec3 HslToRgb(vec3 HSL)
{
	if(HSL.s == 0.0f)
		return vec3(HSL.l, HSL.l, HSL.l);
	else
	{
		float v2 = HSL.l < 0.5f ? HSL.l * (1.0f + HSL.s) : (HSL.l+HSL.s) - (HSL.s*HSL.l);
		float v1 = 2.0f * HSL.l - v2;

		return vec3(HueToRgb(v1, v2, HSL.h + (1.0f/3.0f)), HueToRgb(v1, v2, HSL.h), HueToRgb(v1, v2, HSL.h - (1.0f/3.0f)));
	}
}

/*
	Function: HexToRgba
		Converts Hex to Rgba

	Remarks: Hex should be RGBA8
*/
inline vec4 HexToRgba(int hex)
{
	vec4 c;
	c.r = ((hex >> 24) & 0xFF) / 255.0f;
	c.g = ((hex >> 16) & 0xFF) / 255.0f;
	c.b = ((hex >> 8) & 0xFF) / 255.0f;
	c.a = (hex & 0xFF) / 255.0f;

	return c;
}

/*
	Function: HsvToRgb
		Converts Hsv to Rgb
*/
inline vec3 HsvToRgb(vec3 hsv)
{
	int h = int(hsv.x * 6.0f);
	float f = hsv.x * 6.0f - h;
	float p = hsv.z * (1.0f - hsv.y);
	float q = hsv.z * (1.0f - hsv.y * f);
	float t = hsv.z * (1.0f - hsv.y * (1.0f - f));

	vec3 rgb = vec3(0.0f, 0.0f, 0.0f);

	switch(h % 6)
	{
	case 0:
		rgb.r = hsv.z;
		rgb.g = t;
		rgb.b = p;
		break;

	case 1:
		rgb.r = q;
		rgb.g = hsv.z;
		rgb.b = p;
		break;

	case 2:
		rgb.r = p;
		rgb.g = hsv.z;
		rgb.b = t;
		break;

	case 3:
		rgb.r = p;
		rgb.g = q;
		rgb.b = hsv.z;
		break;

	case 4:
		rgb.r = t;
		rgb.g = p;
		rgb.b = hsv.z;
		break;

	case 5:
		rgb.r = hsv.z;
		rgb.g = p;
		rgb.b = q;
		break;
	}

	return rgb;
}

#endif
