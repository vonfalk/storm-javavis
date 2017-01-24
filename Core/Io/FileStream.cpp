#include "stdafx.h"
#include "FileStream.h"
#include "Core/Str.h"
#include "Core/StrBuf.h"

namespace storm {

#ifdef WINDOWS

	static OSHandle openFile(Str *name, bool input) {
		// TODO: Use overlapped flag.
		return CreateFile(name->c_str(),
						input ? GENERIC_READ : GENERIC_WRITE,
						FILE_SHARE_READ | FILE_SHARE_WRITE,
						NULL,
						OPEN_EXISTING,
						FILE_ATTRIBUTE_NORMAL,
						NULL);
	}

	static void copyFilePtr(OSHandle to, OSHandle from) {
		LARGE_INTEGER pos;
		pos.QuadPart = 0;
		SetFilePointerEx(from.v(), pos, &pos, FILE_CURRENT);
		SetFilePointerEx(to.v(), pos, NULL, FILE_BEGIN);
	}

	static OSHandle copyFile(OSHandle h, Str *name, bool input) {
		if (h) {
			OSHandle r = openFile(name, input);
			copyFilePtr(r, h);
			return r;
		} else {
			return OSHandle();
		}
	}

	IFileStream::IFileStream(Str *name) : OSRIStream(openFile(name, true)), name(name) {}

	IFileStream::IFileStream(const IFileStream &o) : OSRIStream(copyFile(o.handle, o.name, true)), name(o.name) {}

	void IFileStream::toS(StrBuf *to) const {
		*to << L"File input from " << name;
	}


	OFileStream::OFileStream(Str *name) : OSOStream(openFile(name, false)), name(name) {}

	OFileStream::OFileStream(const OFileStream &o) : OSOStream(copyFile(o.handle, o.name, false)), name(o.name) {}

	void OFileStream::toS(StrBuf *to) const {
		*to << L"File output to " << name;
	}


#else
#error "Please implement file IO for your platform!"
#endif

}
