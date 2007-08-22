#ifndef BASE_VMATH_H
#define BASE_VMATH_H

//#include <engine/system.h>

// ------------------------------------

template<typename T>
class vector2_base
{
public:
	union { T x,u; };
	union { T y,v; };
	
	vector2_base() {}
	vector2_base(float nx, float ny)
	{
		x = nx;
		y = ny;
	}
	
	vector2_base operator -() const { return vector2_base(-x, -y); }
	vector2_base operator -(const vector2_base &v) const { return vector2_base(x-v.x, y-v.y); }
	vector2_base operator +(const vector2_base &v) const { return vector2_base(x+v.x, y+v.y); }
	vector2_base operator *(const T v) const { return vector2_base(x*v, y*v); }
	
	const vector2_base &operator =(const vector2_base &v) { x = v.x; y = v.y; return *this; }

	const vector2_base &operator +=(const vector2_base &v) { x += v.x; y += v.y; return *this; }
	const vector2_base &operator -=(const vector2_base &v) { x -= v.x; y -= v.y; return *this; }
	const vector2_base &operator *=(const T v) { x *= v; y *= v; return *this;	}

	bool operator ==(const vector2_base &v) const { return x == v.x && y == v.y; } //TODO: do this with an eps instead

	operator const T* () { return &x; }
};


template<typename T>
inline T length(const vector2_base<T> &a)
{
	return sqrtf(a.x*a.x + a.y*a.y);
}

template<typename T>
inline T distance(const vector2_base<T> a, const vector2_base<T> &b)
{
	return length(a-b);
}

template<typename T>
inline T dot(const vector2_base<T> a, const vector2_base<T> &b)
{
	return a.x*b.x + a.y*b.y;
}

template<typename T>
inline vector2_base<T> normalize(const vector2_base<T> &v)
{
	T l = (T)(1.0f/sqrtf(v.x*v.x + v.y*v.y));
	return vector2_base<T>(v.x*l, v.y*l);
}

typedef vector2_base<float> vec2;
typedef vector2_base<bool> bvec2;
typedef vector2_base<int> ivec2;
	
// ------------------------------------
template<typename T>
class vector3_base
{
public:
	union { T x,r; };
	union { T y,g; };
	union { T z,b; };

	vector3_base() {}
	vector3_base(float nx, float ny, float nz)
	{
		x = nx;
		y = ny;
		z = nz;
	}
	
	const vector3_base &operator =(const vector3_base &v) { x = v.x; y = v.y; z = v.z; return *this; }

	vector3_base operator -(const vector3_base &v) const { return vector3_base(x-v.x, y-v.y, z-v.z); }
	vector3_base operator -() const { return vector3_base(-x, -y, -z); }
	vector3_base operator +(const vector3_base &v) const { return vector3_base(x+v.x, y+v.y, z+v.z); }
	vector3_base operator *(const T v) const { return vector3_base(x*v, y*v, z*v); }
	vector3_base operator *(const vector3_base &v) const { return vector3_base(x*v.x, y*v.y, z*v.z); }
	vector3_base operator /(const T v) const { return vector3_base(x/v, y/v, z/v); }
	
	const vector3_base &operator +=(const vector3_base &v) { x += v.x; y += v.y; z += v.z; return *this; }
	const vector3_base &operator -=(const vector3_base &v) { x -= v.x; y -= v.y; z -= v.z; return *this; }
	const vector3_base &operator *=(const T v) { x *= v; y *= v; z *= v; return *this;	}

	bool operator ==(const vector3_base &v) const { return x == v.x && y == v.y && z == v.z; } //TODO: do this with an eps instead

	operator const T* () { return &x; }
};

template<typename T>
inline T length(const vector3_base<T> &a)
{
	return sqrtf(a.x*a.x + a.y*a.y + a.z*a.z);
}

template<typename T>
inline T distance(const vector3_base<T> &a, const vector3_base<T> &b)
{
	return length(a-b);
}

template<typename T>
inline T dot(const vector3_base<T> &a, const vector3_base<T> &b)
{
	return a.x*b.x + a.y*b.y + a.z*b.z;
}

template<typename T>
inline vector3_base<T> normalize(const vector3_base<T> &v)
{
	T l = (T)(1.0f/sqrtf(v.x*v.x + v.y*v.y + v.z*v.z));
	return vector3_base<T>(v.x*l, v.y*l, v.z*l);
}

template<typename T>
inline vector3_base<T> cross(const vector3_base<T> &a, const vector3_base<T> &b)
{
	return vector3_base<T>(
		a.y*b.z - a.z*b.y,
		a.z*b.x - a.x*b.z,
		a.x*b.y - a.y*b.x);
}

typedef vector3_base<float> vec3;
typedef vector3_base<bool> bvec3;
typedef vector3_base<int> ivec3;

// ------------------------------------

template<typename T>
class vector4_base
{
public:
	union { T x,r; };
	union { T y,g; };
	union { T z,b; };
	union { T w,a; };

	vector4_base() {}
	vector4_base(float nx, float ny, float nz, float nw)
	{
		x = nx;
		y = ny;
		z = nz;
		w = nw;
	}
	
	vector4_base operator +(const vector4_base &v) const { return vector4_base(x+v.x, y+v.y, z+v.z, w+v.w); }
	vector4_base operator -(const vector4_base &v) const { return vector4_base(x-v.x, y-v.y, z-v.z, w-v.w); }
	vector4_base operator -() const { return vector4_base(-x, -y, -z, -w); }
	vector4_base operator *(const T v) const { return vector4_base(x*v, y*v, z*v, w*v); }
	
	const vector4_base &operator =(const vector4_base &v) { x = v.x; y = v.y; z = v.z; w = v.w; return *this; }

	const vector4_base &operator +=(const vector4_base &v) { x += v.x; y += v.y; z += v.z; w += v.w; return *this; }
	const vector4_base &operator -=(const vector4_base &v) { x -= v.x; y -= v.y; z -= v.z; w -= v.w; return *this; }
	const vector4_base &operator *=(const T v) { x *= v; y *= v; z *= v; w *= v; return *this;	}

	bool operator ==(const vector4_base &v) const { return x == v.x && y == v.y && z == v.z && w == v.w; } //TODO: do this with an eps instead

	operator const T* () { return &x; }
};

typedef vector4_base<float> vec4;
typedef vector4_base<bool> bvec4;
typedef vector4_base<int> ivec4;

#endif
