#pragma once

struct __declspec(novtable) Wb 
{
	virtual Wb& operator ()(PCSTR fmt, ...) = 0;
};

struct DPrint : public Wb 
{
	virtual Wb& operator ()(PCSTR fmt, ...);
};

class SPrint : public Wb 
{
	PSTR _buf = 0, _ptr = 0;
	ULONG _cch = 0;
public:
	~SPrint()
	{
		if (_buf) LocalFree(_buf);
	}

	PCSTR get_data()
	{
		return _buf;
	}

	size_t get_size()
	{
		return _ptr - _buf; 
	}

	BOOL InitBuf(ULONG cch);

	virtual Wb& operator ()(PCSTR fmt, ...);
};

class VmPrint : public Wb 
{
	PSTR _buf = 0, _ptr = 0;
	SIZE_T _reserve = 0, _commit = 0;
public:
	~VmPrint()
	{
		if (_buf)
		{
			VirtualFree(_buf, 0, MEM_RELEASE);
		}
	}

	PCSTR get_data()
	{
		return _buf;
	}

	size_t get_size()
	{
		return _ptr - _buf; 
	}

	NTSTATUS InitBuf(ULONG cch);

	virtual Wb& operator ()(PCSTR fmt, ...);
};

class VmEPrint : public Wb 
{
	PSTR _buf = 0, _ptr = 0;
	SIZE_T _reserve = 0;
public:
	~VmEPrint()
	{
		if (_buf)
		{
			VirtualFree(_buf, 0, MEM_RELEASE);
		}
	}

	PCSTR get_data()
	{
		return _buf;
	}

	size_t get_size()
	{
		return _ptr - _buf; 
	}

	NTSTATUS InitBuf(ULONG cch);

	LONG Filter(PEXCEPTION_RECORD ExceptionRecord);

	virtual Wb& operator ()(PCSTR fmt, ...);
};

