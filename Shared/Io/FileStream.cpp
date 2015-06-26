#include "stdafx.h"
#include "FileStream.h"

namespace storm {

#ifdef WINDOWS

	static HANDLE openInput(const String &name) {
		return CreateFile(name.c_str(),
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

	IFileStream::IFileStream(const String &name) : name(name) {
		assert(sizeof(void *) >= sizeof(HANDLE));
		handle = (void *)openInput(name);
	}

	IFileStream::IFileStream(Par<IFileStream> o) : name(o->name) {
		handle = (void *)openInput(name);
		copyFilePtr((HANDLE)o->handle, (HANDLE)handle);
	}

	IFileStream::~IFileStream() {
		if ((HANDLE)handle != INVALID_HANDLE_VALUE)
			CloseHandle((HANDLE)handle);
	}

	Bool IFileStream::more() {
		if ((HANDLE)handle == INVALID_HANDLE_VALUE)
			return false;

		LARGE_INTEGER len, pos;
		GetFileSizeEx((HANDLE)handle, &len);
		pos.QuadPart = 0;
		SetFilePointerEx((HANDLE)handle, pos, &pos, FILE_CURRENT);

		return pos.QuadPart < len.QuadPart;
	}

	Nat IFileStream::read(Buffer &to) {
		// TODO: read data async and allow other UThreads to do things.
		if ((HANDLE)handle == INVALID_HANDLE_VALUE)
			return 0;

		DWORD out = 0;
		ReadFile((HANDLE)handle, to.dataPtr(), DWORD(to.count()), &out, NULL);

		return out;
	}

	Nat IFileStream::peek(Buffer &to) {
		if ((HANDLE)handle == INVALID_HANDLE_VALUE)
			return 0;

		LARGE_INTEGER pos;
		pos.QuadPart = 0;
		SetFilePointerEx((HANDLE)handle, pos, &pos, FILE_CURRENT);

		Nat r = read(to);

		SetFilePointerEx((HANDLE)handle, pos, NULL, FILE_BEGIN);

		return r;
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

	void IFileStream::output(wostream &to) const {
		to << L"IFile: " << name;
	}

	static HANDLE openOutput(const String &name, bool copy) {
		return CreateFile(name.c_str(),
						GENERIC_WRITE,
						FILE_SHARE_READ | FILE_SHARE_WRITE,
						NULL,
						copy ? OPEN_ALWAYS : CREATE_ALWAYS,
						FILE_ATTRIBUTE_NORMAL,
						NULL);
	}

	OFileStream::OFileStream(const String &name) : name(name) {
		handle = (void *)openOutput(name, false);
	}

	OFileStream::OFileStream(Par<OFileStream> o) : name(o->name) {
		handle = (void *)openOutput(name, true);
		copyFilePtr((HANDLE)o->handle, (HANDLE)handle);
	}

	OFileStream::~OFileStream() {
		if ((HANDLE)handle != INVALID_HANDLE_VALUE)
			CloseHandle((HANDLE)handle);
	}

	void OFileStream::write(const Buffer &d) {
		if ((HANDLE)handle == INVALID_HANDLE_VALUE)
			return;

		DWORD out = 0;
		WriteFile((HANDLE)handle, d.dataPtr(), DWORD(d.count()), &out, NULL);
		// Error check? Naw...
	}

	void OFileStream::output(wostream &to) const {
		to << L"OFile: " << name;
	}

#else
#error "Please implement the FileStream for your OS!"
#endif

}
