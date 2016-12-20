#include "stdafx.h"
#include "FileStream.h"
#include "Core/Str.h"

namespace storm {

#ifdef WINDOWS

	static HANDLE openInput(const wchar *name) {
		return CreateFile(name,
						GENERIC_READ,
						FILE_SHARE_READ | FILE_SHARE_WRITE,
						NULL,
						OPEN_EXISTING,
						FILE_ATTRIBUTE_NORMAL,
						NULL);
	}

	static void copyFilePtr(HANDLE from, HANDLE to) {
		LARGE_INTEGER pos;
		pos.QuadPart = 0;
		SetFilePointerEx(from, pos, &pos, FILE_CURRENT);
		SetFilePointerEx(to, pos, NULL, FILE_BEGIN);
	}

	IFileStream::IFileStream(Str *name) : name(name) {
		assert(sizeof(void *) >= sizeof(HANDLE));
		handle = (void *)openInput(name->c_str());
	}

	IFileStream::IFileStream(const IFileStream &o) : name(o.name) {
		handle = (void *)openInput(name->c_str());
		copyFilePtr((HANDLE)o.handle, (HANDLE)handle);
	}

	IFileStream::~IFileStream() {
		close();
	}

	void IFileStream::close() {
		if ((HANDLE)handle != INVALID_HANDLE_VALUE)
			CloseHandle((HANDLE)handle);
		handle = (void *)INVALID_HANDLE_VALUE;
	}

	Bool IFileStream::more() {
		if ((HANDLE)handle == INVALID_HANDLE_VALUE)
			return false;

		return tell() < length();
	}

	void IFileStream::seek(Word to) {
		LARGE_INTEGER pos;
		pos.QuadPart = to;
		SetFilePointerEx((HANDLE)handle, pos, NULL, FILE_BEGIN);
	}

	Word IFileStream::tell() {
		LARGE_INTEGER pos;
		pos.QuadPart = 0;
		SetFilePointerEx((HANDLE)handle, pos, &pos, FILE_CURRENT);
		return pos.QuadPart;
	}

	Word IFileStream::length() {
		LARGE_INTEGER len;
		GetFileSizeEx((HANDLE)handle, &len);
		return len.QuadPart;
	}

	RIStream *IFileStream::randomAccess() {
		return this;
	}

	Buffer IFileStream::read(Buffer b, Nat start) {
		start = min(start, b.count());
		b.filled(0);

		// TODO: Read data async and allow other UThreads to do things!
		if ((HANDLE)handle == INVALID_HANDLE_VALUE)
			return b;

		DWORD out = 0;
		ReadFile((HANDLE)handle, b.dataPtr() + start, DWORD(b.count() - start), &out, NULL);
		b.filled(out + start);
		return b;
	}

	Buffer IFileStream::peek(Buffer b, Nat start) {
		b.filled(0);

		// TODO: Read data async and allow other UThreads to do things!
		if ((HANDLE)handle == INVALID_HANDLE_VALUE)
			return b;

		Word pos = tell();
		b = read(b, start);
		seek(pos);
		return b;
	}

#else
#error "Please implement file IO for your platform!"
#endif

}
