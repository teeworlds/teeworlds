

template<class T>
class quaternion_base
{
public:
	T k[4];

	/*void Unit()
	{
		k[0] = 0;
		k[1] = 0;
		k[2] = 0;
		k[3] = 1;
	}*/
	
	inline quaternion_base(){}

	inline quaternion_base(T x, T y, T z, T w)
	{
		k[0] = x;
		k[1] = y;
		k[2] = z;
		k[3] = w;
	}
	
	inline quaternion_base(vector3_base<T> axis, T angle)
	{
		T sin_angle = sin(angle * (T)0.5);
		T cos_angle = cos(angle * (T)0.5);
		k[0] = axis.x * sin_angle;
		k[1] = axis.y * sin_angle;
		k[2] = axis.z * sin_angle;
		k[3] = cos_angle;
	}
	

	T magnitude()
	{
		return sqrt(k[0] * k[0] + k[1] * k[1] + k[2] * k[2] + k[3] * k[3]);
	}

	matrix4_base<T> create_matrix() const
	{
		matrix4_base<T> mat;
		
		T xx = k[0] * k[0];
		T xy = k[0] * k[1];
		T xz = k[0] * k[2];
		T xw = k[0] * k[3];
		T yy = k[1] * k[1];
		T yz = k[1] * k[2];
		T yw = k[1] * k[3];
		T zz = k[2] * k[2];
		T zw = k[2] * k[3];
		
		mat.k[0][0] = 1 - 2 * (yy + zz);
		mat.k[0][1] = 2 * (xy + zw);
		mat.k[0][2] = 2 * (xz - yw);
		mat.k[0][3] = 0;
		
		mat.k[1][0] = 2 * (xy - zw);
		mat.k[1][1] = 1 - 2 * (xx + zz);
		mat.k[1][2] = 2 * (yz + xw);
		mat.k[1][3] = 0;

		mat.k[2][0] = 2 * (xz + yw);
		mat.k[2][1] = 2 * (yz - xw);
		mat.k[2][2] = 1 - 2 * (xx + yy);
		mat.k[2][3] = 0;

		mat.k[3][0] = 0;
		mat.k[3][1] = 0;
		mat.k[3][2] = 0;
		mat.k[3][3] = 1;
	}

	/*
	void CreateDOOM(T x, T y, T z)
	{
		k[0] = x;
		k[1] = y;
		k[2] = z;
		T Term = 1 - (x * x) - (y * y) - (z * z);
		if (Term < 0)
			k[3] = 0;
		else
			k[3] = -sqrt(Term);

		Normalize();
	}
	
	T DotProd(const TQuaternion<T>& Quat) const
	{
		return (k[0] * Quat.k[0] + k[1] * Quat.k[1] + k[2] * Quat.k[2] + k[3] * Quat.k[3]);
	}

	void Interpolate(const TQuaternion<T>& Quat, TQuaternion& Dest, T Scale)
	{
		T Separation = k[0] * Quat.k[0] + k[1] * Quat.k[1] + k[2] * Quat.k[2] + k[3] * Quat.k[3];
		T Factor1,Factor2;

		if (Separation > 1)
			Separation = 1;
		if (Separation < -1)
			Separation = -1;
		Separation = acos(Separation);
		if (Separation == 0 || Separation == pi)
		{
			Factor1 = 1;
			Factor2 = 0;
		}
		else
		{
			Factor1 = sin((1 - Scale)*Separation) / sin(Separation);
			Factor2 = sin(Scale * Separation) / sin(Separation);
		}
		
		Dest.k[0] = k[0] * Factor1 + Quat.k[0] * Factor2;
		Dest.k[1] = k[1] * Factor1 + Quat.k[1] * Factor2;
		Dest.k[2] = k[2] * Factor1 + Quat.k[2] * Factor2;
		Dest.k[3] = k[3] * Factor1 + Quat.k[3] * Factor2;
	}

	void Slerp(const TQuaternion<T>& Quat, TQuaternion& Dest, T Scale) const
	{
		T Sq1,Sq2;
		T Dot = DotProd(Quat);

		TQuaternion Temp;

		if (Dot < 0.0f)
		{
			Dot = -Dot;
			Temp.k[0] = -Quat.k[0];
			Temp.k[1] = -Quat.k[1];
			Temp.k[2] = -Quat.k[2];
			Temp.k[3] = -Quat.k[3];
		}
		else
		{
			Temp = Quat;
		}

		if ((1.0 + Dot) > 0.00001)
		{
			if ((1.0 - Dot) > 0.00001)
			{
				T om = (T)acos(Dot);
				T rsinom = (T)(1.0f / sin(om));
				Sq1 = (T)sin(((T)1.0 - Scale) * om) * rsinom;
				Sq2 = (T)sin(Scale * om) * rsinom;
			}
			else
			{
				Sq1 = (T)(1.0 - Scale);
				Sq2 = Scale;
			}
			Dest.k[0] = Sq1 * k[0] + Sq2 * Temp[0];
			Dest.k[1] = Sq1 * k[1] + Sq2 * Temp[1];
			Dest.k[2] = Sq1 * k[2] + Sq2 * Temp[2];
			Dest.k[3] = Sq1 * k[3] + Sq2 * Temp[3];
		}
		else
		{
			Sq1 = (T)sin(((T)1.0 - Scale) * (T)0.5 * pi);
			Sq2 = (T)sin(Scale * (T)0.5 * pi);

			Dest.k[0] = Sq1 * k[0] + Sq2 * Temp[1];
			Dest.k[1] = Sq1 * k[1] + Sq2 * Temp[0];
			Dest.k[2] = Sq1 * k[2] + Sq2 * Temp[3];
			Dest.k[3] = Sq1 * k[3] + Sq2 * Temp[2];
		}
	}*/

	// perators
	T& operator [] (int i) { return k[i]; }

	// quaternion multiply
	quaternion_base operator *(const quaternion_base& other) const
	{
		// (w1 dot w2 - v1 dot v2, w1 dot v2 + w2 dot v1 + v1 cross v2)
		quaternion_base r;
		r.k[0] = k[3] * other.k[0] + k[0] * other.k[3] + k[1] * other.k[2] - k[2] * other.k[1];
		r.k[1] = k[3] * other.k[1] + k[1] * other.k[3] + k[2] * other.k[0] - k[0] * other.k[2];
		r.k[2] = k[3] * other.k[2] + k[2] * other.k[3] + k[0] * other.k[1] - k[1] * other.k[0];
		r.k[3] = k[3] * other.k[3] - k[0] * other.k[0] - k[1] * other.k[1] - k[2] * other.k[2];

		return normalize(r);
	}
	
	/*
	bool operator == (const quaternion_base<T>& t) const { return ((k[0] == t.k[0]) && (k[1] == t.k[1]) && (k[2] == t.k[2]) && (k[3] == t.k[3])); }
	bool operator != (const quaternion_base<T>& t) const { return ((k[0] != t.k[0]) || (k[1] != t.k[1]) || (k[2] != t.k[2]) || (k[3] != t.k[3])); }

	void operator = (const quaternion_base<T>& other) { k[0] = other.k[0]; k[1] = other.k[1]; k[2] = other.k[2]; k[3] = other.k[3]; }
	*/
	
	/*void operator *= (const TQuaternion<T>& t) 				{ TQuaternion Temp = Multiply(t); *this = Temp; }
	TQuaternion operator * (const TQuaternion<T>& t)		const	{ return Multiply(t); }
	*/
};

template<typename T>
inline quaternion_base<T> normalize(const quaternion_base<T> &v)
{
	T factor = 1.0f/v.magnitude();
	return quaternion_base<T>(v.k[0]*factor, v.k[1]*factor,v. k[2]*factor,v. k[3]*factor);
}


