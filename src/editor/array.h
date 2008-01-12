
template <class T>
class array
{
	// 
	//
	void init()
	{
		list = 0;
		clear();
	}

public:
	array()
	{
		init();
	}
	
	//
	array(const array &other)
	{
		init();
		setsize(other.len());
		for(int i = 0; i < len(); i++)
			(*this)[i] = other[i];
	}


	// 
	//
	virtual ~array()
	{
		delete [] list;
		list = 0;
	}

	//
	//
	void deleteall()
	{
		for(int i = 0; i < len(); i++)
			delete list[i];

		clear();
	}


	//
	//
	void clear()
	{
		delete [] list;
		
		list_size = 1;
		list = new T[1];
		num_elements = 0;
	}

	int find(T val)
	{
		for(int i = 0; i < len(); i++)
			if((*this)[i] == val)
				return i;
		return -1;
	}

	bool exist(T val)
	{
		return find(val) != -1;
	}

	//
	// returns the number of elements in the list
	//
	int len() const
	{
		return num_elements;
	}

	//
	// This doesn't conserve the order in the list. Be careful
	//
	void removebyindexfast(int index)
	{
		//ASSUME(_Pos >= 0 && _Pos < num_elements);
		list[index] = list[num_elements-1];
		setsize(len()-1);
	}

	void removefast(const T& _Elem)
	{
		for(int i = 0; i < len(); i++)
			if(list[i] == _Elem)
			{
				removebyindexfast(i);
				return;
			}
	}

	//
	//
	void removebyindex(int index)
	{
		//ASSUME(_Pos >= 0 && _Pos < num_elements);

		for(int i = index+1; i < num_elements; i++)
			list[i-1] = list[i];
		
		setsize(len()-1);
	}

	void insert(int index, const T& element)
	{
		int some_len = len();
		if (index < some_len)
			setsize(some_len+1);
		else
			setsize(index + 1);

		for(int i = num_elements-2; i >= index; i--)
			list[i+1] = list[i];

		list[index] = element;
	}

	bool remove(const T& element)
	{
		for(int i = 0; i < len(); i++)
			if(list[i] == element)
			{
				removebyindex(i);
				return true;
			}
		return false;
	}

	// 
	//
	int add(const T& element)
	{
		//if(num_elements == list_size)
		setsize(len()+1);
		list[num_elements-1] = element;
		return num_elements-1;
	}

	// 
	//
	int add(const T& elem, int index)
	{
		setsize(len()+1);
		
		for(int i = num_elements-1; i > index; i--)
			list[i] = list[i-1];

		list[index] = elem;
		
		//num_elements++;
		return num_elements-1;
	}

	// 
	//
	T& operator[] (int index)
	{
		return list[index];
	}

	const T& operator[] (int index) const
	{
		return list[index];
	}

	//
	//
	T *getptr()
	{
		return list;
	}

	const T *getptr() const
	{
		return list;
	}

	//
	//
	//
	void setsize(int new_len)
	{
		if (list_size < new_len)
			allocsize(new_len);
		num_elements = new_len;
	}

	// removes unnessasary data, returns how many bytes was earned
	int optimize()
	{
		int Before = memoryusage();
		setsize(num_elements);
		return Before - memoryusage();
	}

	// returns how much memory this dynamic array is using
	int memoryusage()
	{
		return sizeof(array) + sizeof(T)*list_size;
	}

	//
	array &operator = (const array &other)
	{
		setsize(other.len());
		for(int i = 0; i < len(); i++)
			(*this)[i] = other[i];
		return *this;
	}		
private:
	void allocsize(int new_len)
	{
		list_size = new_len;
		T *new_list = new T[list_size];
		
		long end = num_elements < list_size ? num_elements : list_size;
		for(int i = 0; i < end; i++)
			new_list[i] = list[i];
		
		delete [] list;
		list = 0;
		num_elements = num_elements < list_size ? num_elements : list_size;
		list = new_list;
	}

	T *list;
	long list_size;
	long num_elements;
};

