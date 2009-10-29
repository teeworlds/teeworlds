#ifndef TL_FILE_STREAM_HPP
#define TL_FILE_STREAM_HPP

class input_stream
{
public:	
	virtual ~input_stream() {}
	virtual size_t read(void *data, size_t size) = 0;
	virtual size_t size() = 0;
};

class output_stream
{
public:	
	virtual ~output_stream() {}
	virtual size_t write(const void *data, size_t size) = 0;
};


// input wrapping
// RAII style
class file_backend
{
private:
	file_backend(const file_backend &other) { /* no copy allowed */ }
protected:
	IOHANDLE file_handle;
	
	explicit file_backend(const char *filename, int flags)
	{
		file_handle = io_open(filename, flags);
	}
	
	~file_backend()
	{
		if(file_handle)
			io_close(file_handle);
	}
public:
	bool is_open() const { return file_handle != 0; }
};

class file_reader : public input_stream, public file_backend
{
public:	
	explicit file_reader(const char *filename)
	: file_backend(filename, IOFLAG_READ)
	{}
	
	virtual size_t read(void *data, size_t size) { return io_read(file_handle, data, size); }
	virtual size_t size() { return io_length(file_handle); }
};


class file_writer : public output_stream, public file_backend
{
public:	
	explicit file_writer(const char *filename)
	: file_backend(filename, IOFLAG_WRITE)
	{}
	
	virtual size_t write(const void *data, size_t size) { return io_write(file_handle, data, size); }
};

#endif
