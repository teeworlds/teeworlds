/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#ifndef BASE_MATH_H
#define BASE_MATH_H

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

template<typename T, typename TB>
inline T mix(const T a, const T b, TB amount)
{
	return a + (b-a)*amount;
}
const float pi = 3.1415926535897932384626433f;

template <typename T> inline T min(T a, T b) { return a<b?a:b; }
template <typename T> inline T max(T a, T b) { return a>b?a:b; }

#endif // BASE_MATH_H
