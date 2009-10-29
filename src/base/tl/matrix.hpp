#ifndef TL_FILE_MATRIX_HPP
#define TL_FILE_MATRIX_HPP

/*
 
 Looks like OpenGL 
 
 Column Major

        / m[0][0]  m[1][0]  m[2][0]  m[3][0] \     / v[0] \
        |                                    |     |      |
		| m[0][1]  m[1][1]  m[2][1]  m[3][1] |     | v[1] |
        |                                    |     |      |
 M(v) = | m[0][2]  m[1][2]  m[2][2]  m[3][2] |  X  | v[2] |
        |                                    |     |      |
        \ m[0][3]  m[1][3]  m[2][3]  m[3][3] /     \ v[3] /
 
 v[0] = x
 v[1] = y
 v[2] = z
 v[3] = w or 1
 
   +y
   |
   |
   |_____ +x
   /
  /
 +z
 
 right = +x
 up = +y
 forward = -z

*/

template<class T>
class matrix4_base
{
public:
	// [col][row]
	T m[4][4];

	//
	inline matrix4_base()
	{}

	inline vector3_base<T> get_column3(const int i) const { return vector3_base<T>(m[i][0], m[i][1], m[i][2]); }
	inline vector3_base<T> get_column4(const int i) const { return vector4_base<T>(m[i][0], m[i][1], m[i][2], m[i][3]); }
	inline vector3_base<T> get_row3(const int i) const { return vector3_base<T>(m[0][i], m[1][i], m[2][i]); }
	inline vector4_base<T> get_row4(const int i) const { return vector4_base<T>(m[0][i], m[1][i], m[2][i], m[3][i]); }

	inline vector3_base<T> get_right() const { return get_row3(0); }
	inline vector3_base<T> get_up() const { return get_row3(1); }
	inline vector3_base<T> get_forward() const { return -get_row3(2); }
	inline vector3_base<T> get_translation() const { return get_column3(3); }

	//
	void unit()
	{
		m[0][1] = m[0][2] = m[0][3] = 0;
		m[1][0] = m[1][2] = m[1][3] = 0;
		m[2][0] = m[2][1] = m[2][3] = 0;
		m[3][0] = m[3][1] = m[3][2] = 0;

		m[0][0] = m[1][1] = m[2][2] = m[3][3] = 1;
	}

	//
	vector3_base<T> operator *(const vector3_base<T> &v) const
	{
		vector3_base<T> r(0,0,0);

		r.x += v.x*m[0][0] + v.y*m[1][0] + v.z*m[2][0] + m[3][0];
		r.y += v.x*m[0][1] + v.y*m[1][1] + v.z*m[2][1] + m[3][1];
		r.z += v.x*m[0][2] + v.y*m[1][2] + v.z*m[2][2] + m[3][2];
		return r;
	}

	//
	vector4_base<T> operator *(const vector4_base<T> &v) const
	{
		vector4_base<T> r(0,0,0,0);

		r.x += v.x*m[0][0] + v.y*m[1][0] + v.z*m[2][0] + v.w*m[3][0];
		r.y += v.x*m[0][1] + v.y*m[1][1] + v.z*m[2][1] + v.w*m[3][1];
		r.z += v.x*m[0][2] + v.y*m[1][2] + v.z*m[2][2] + v.w*m[3][2];
		r.w += v.x*m[0][3] + v.y*m[1][3] + v.z*m[2][3] + v.w*m[3][3];
		return r;
	}
	//
	matrix4_base operator *(const matrix4_base &other) const
	{
		matrix4_base r;

		for(int i = 0; i < 4; i++)
			for(int j = 0; j < 4; j++)
			{
				r.m[i][j] = 0;
				for(int a = 0; a < 4; a++)
					r.m[i][j] += m[a][j] * other.m[i][a];
			}
		return r;
	}
	
	
	//
	// THIS PART IS KINDA UGLY BECAUSE MAT4 IS NOT IMMUTABLE
	//
	
	inline void set_row(const vector3_base<T>& v, const int row)
	{
		m[0][row] = v.x;
		m[1][row] = v.y;
		m[2][row] = v.z;
	}

	inline void set_column(const vector3_base<T>& v, const int col)
	{
		m[col][0] = v.x;
		m[col][1] = v.y;
		m[col][2] = v.z;
	}

	inline void set_translation(const vector3_base<T>& v) { set_column(v,3); }	

	//
	void rot_x(T angle)
	{
		T sina = (T)sin(angle);
		T cosa = (T)cos(angle);

		unit();
		m[1][1] = cosa; m[2][1] =-sina;
		m[1][2] = sina; m[2][2] = cosa;
	}

	//
	void rot_y(T angle)
	{
		T sina = (T)sin(-angle);
		T cosa = (T)cos(-angle);

		unit();
		m[0][0] = cosa; m[2][0] =-sina;
		m[0][2] = sina; m[2][2] = cosa;
	}

	//
	void rot_z(T angle)
	{
		T sina = (T)sin(angle);
		T cosa = (T)cos(angle);

		unit();
		m[0][0] = cosa; m[1][0] =-sina;
		m[0][1] = sina; m[1][1] = cosa;
	}
};

typedef matrix4_base<float> mat4;

#endif
