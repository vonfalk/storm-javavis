#include "stdafx.h"
#include "FileStream.h"
#include "Core/Str.h"
#include "Core/StrBuf.h"

namespace storm {

#if defined(WINDOWS)

	static os::Handle openFile(Str *name, bool input) {
		// TODO: Allow duplicating this handle so that we can clone duplicate it later?

		return CreateFile(name->c_str(),
						input ? GENERIC_READ : GENERIC_WRITE,
						FILE_SHARE_READ | FILE_SHARE_WRITE,
						NULL,
						input ? OPEN_EXISTING : CREATE_ALWAYS,
						FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
						NULL);
	}

	static void copyFilePtr(os::Handle to, os::Handle from) {
		LARGE_INTEGER pos;
		pos.QuadPart = 0;
		SetFilePointerEx(from.v(), pos, &pos, FILE_CURRENT);
		SetFilePointerEx(to.v(), pos, NULL, FILE_BEGIN);
	}

	static os::Handle copyFile(os::Handle h, Str *name, bool input) {
		if (h) {
			os::Handle r = openFile(name, input);
			copyFilePtr(r, h);
			return r;
		} else {
			return os::Handle();
		}
	}

#elif defined(POSIX)

	static os::Handle openFile(Str *name, bool input) {
		int flags = O_CLOEXEC;
		if (input)
			flags |= O_RDONLY;
		else
			flags |= O_CREAT | O_TRUNC | O_WRONLY;
		return ::open(name->utf8_str(), flags, 0666);
	}

	static os::Handle copyFile(os::Handle h, Str *name, bool input) {
		UNUSED(name);
		UNUSED(input);

		return dup(h.v());
	}

#else
#error "Please implement file IO for your platform!"
#endif

	IFileStream::IFileStream(Str *name) : HandleRIStream(openFile(name, true)), name(name) {}

	IFileStream::IFileStream(const IFileStream &o) : HandleRIStream(copyFile(o.handle, o.name, true)), name(o.name) {}

	void IFileStream::toS(StrBuf *to) const {
		*to << L"File input from " << name;
	}


	OFileStream::OFileStream(Str *name) : HandleOStream(openFile(name, false)), name(name) {}

	OFileStream::OFileStream(const OFileStream &o) : HandleOStream(copyFile(o.handle, o.name, false)), name(o.name) {}

	void OFileStream::toS(StrBuf *to) const {
		*to << L"File output to " << name;
	}

}
