/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#ifndef BASE_MATH_H
#define BASE_MATH_H

#include <stdlib.h>

template <typename T>
inline T clamp(T val, T min, T max)
{
	if(val < min)
		return min;
	if(val > max)
		return max;
	return val;
}

inline float sign(float f)
{
	return f<0.0f?-1.0f:1.0f;
}

inline int round(float f)
{
	if(f > 0)
		return (int)(f+0.5f);
	return (int)(f-0.5f);
}

template<typename T, typename TB>
inline T mix(const T a, const T b, TB amount)
{
	return a + (b-a)*amount;
}

inline float frandom() { return rand()/(float)(RAND_MAX); }

// float to fixed
inline int f2fx(float v) { return (int)(v*(float)(1<<10)); }
inline float fx2f(int v) { return v*(1.0f/(1<<10)); }

class fxp
{
	int value;
public:
	void set(int v) { value = v; }
	int get() const { return value; }
	fxp &operator = (int v) { value = v<<10; return *this; }
	fxp &operator = (float v) { value = (int)(v*(float)(1<<10)); return *this; }
	operator float() const { return value/(float)(1<<10); }
};

class tune_param
{
	int value;
public:
	void set(int v) { value = v; }
	int get() const { return value; }
	tune_param &operator = (int v) { value = (int)(v*100.0f); return *this; }
	tune_param &operator = (float v) { value = (int)(v*100.0f); return *this; }
	operator float() const { return value/100.0f; }
};

const float pi = 3.1415926535897932384626433f;

template <typename T> inline T min(T a, T b) { return a<b?a:b; }
template <typename T> inline T max(T a, T b) { return a>b?a:b; }

#endif // BASE_MATH_H
