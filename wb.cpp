#include "stdafx.h"

_NT_BEGIN

#include "wb.h"

Wb& DPrint::operator ()(PCSTR fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	vDbgPrintEx(DPFLTR_DEFAULT_ID, DPFLTR_INFO_LEVEL, fmt, args);

	va_end(args);

	return *this;
}

BOOL SPrint::InitBuf(ULONG cch)
{
	if (_buf)
	{
		LocalFree(_buf), _buf = 0, _cch = 0, _ptr = 0;
	}

	if (_buf = (PSTR)LocalAlloc(0, cch))
	{
		_ptr = _buf, _cch = cch;

		return TRUE;
	}

	return FALSE;
}

Wb& SPrint::operator ()(PCSTR fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	int len = _vsnprintf_s(_ptr, _cch, _TRUNCATE, fmt, args);

	if (0 < len)
	{
		_ptr += len, _cch -= len;
	}

	va_end(args);

	return *this;
}

NTSTATUS VmPrint::InitBuf(ULONG cch)
{
	if (_buf)
	{
		VirtualFree(_buf, 0, MEM_RELEASE), _buf = 0, _reserve = 0, _commit = 0, _ptr = 0;
	}

	PVOID BaseAddress = 0;
	SIZE_T RegionSize = cch;

	NTSTATUS status = NtAllocateVirtualMemory(NtCurrentProcess(), &BaseAddress, 0, &RegionSize, MEM_RESERVE, PAGE_READWRITE);

	if (0 > status)
	{
		return status;
	}

	_buf = (PSTR)BaseAddress, _ptr = (PSTR)BaseAddress, _reserve = RegionSize;

	return STATUS_SUCCESS;
}

Wb& VmPrint::operator ()(PCSTR fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	ULONG len = _vscprintf(fmt, args);

	if (0 < (int)len)
	{
		if (len < _reserve)
		{
			if (_commit < len)
			{
				PVOID BaseAddress = _ptr + _commit;
				SIZE_T RegionSize = len;
				if (0 > NtAllocateVirtualMemory(NtCurrentProcess(), &BaseAddress, 0, &RegionSize, MEM_COMMIT, PAGE_READWRITE))
				{
					goto __exit;
				}

				_commit = RtlPointerToOffset(_ptr, (PBYTE)BaseAddress + RegionSize);
			}

			len = _vsnprintf_s(_ptr, _commit, _TRUNCATE, fmt, args);

			if (0 < (int)len)
			{
				_ptr += len, _commit -= len, _reserve -= len;
			}
		}
	}
__exit:
	va_end(args);

	return *this;
}

NTSTATUS VmEPrint::InitBuf(ULONG cch)
{
	if (_buf)
	{
		VirtualFree(_buf, 0, MEM_RELEASE), _buf = 0, _reserve = 0, _ptr = 0;
	}

	PVOID BaseAddress = 0;
	SIZE_T RegionSize = cch;

	NTSTATUS status = NtAllocateVirtualMemory(NtCurrentProcess(), &BaseAddress, 0, &RegionSize, MEM_RESERVE, PAGE_READWRITE);

	if (0 > status)
	{
		return status;
	}

	_buf = (PSTR)BaseAddress, _ptr = (PSTR)BaseAddress, _reserve = RegionSize;

	return STATUS_SUCCESS;
}

LONG VmEPrint::Filter(PEXCEPTION_RECORD ExceptionRecord)
{
	if (ExceptionRecord->ExceptionCode == STATUS_ACCESS_VIOLATION &&
		ExceptionRecord->NumberParameters > 1)
	{
		ULONG_PTR pv = ExceptionRecord->ExceptionInformation[1];
		if (pv - (ULONG_PTR)_ptr < _reserve)
		{
			SIZE_T RegionSize = min(_reserve, 0x100000);
			if (0 <= NtAllocateVirtualMemory(NtCurrentProcess(), (void**)&pv, 0, &RegionSize, MEM_COMMIT, PAGE_READWRITE))
			{
				return EXCEPTION_CONTINUE_EXECUTION;
			}
		}
	}

	return EXCEPTION_EXECUTE_HANDLER;
}

Wb& VmEPrint::operator ()(PCSTR fmt, ...)
{
	if (_reserve)
	{
		va_list args;
		va_start(args, fmt);

		__try 
		{
			int len = _vsnprintf(_ptr, _reserve, fmt, args);

			if (0 < len)
			{
				_ptr += len, _reserve -= len;
			}
			else
			{
				_reserve = 0;
			}
		} 
		__except(Filter((GetExceptionInformation())->ExceptionRecord))
		{
			_reserve = 0;
		}

		va_end(args);
	}

	return *this;
}

_NT_END