#include "stdafx.h"
#include "Protocol.h"
#include "Shared/Str.h"
#include "Url.h"
#include "FileStream.h"

#ifdef WINDOWS
#include <Shlwapi.h>
#endif

namespace storm {

	Protocol::Protocol() {}

	Protocol::Protocol(Par<Protocol> o) {}

	Bool Protocol::partEq(Par<Str> a, Par<Str> b) {
		return a->v == b->v;
	}

	ArrayP<Url> *Protocol::children(Par<Url> url) {
		throw ProtocolNotSupported(L"children", ::toS(*this));
	}

	IStream *Protocol::read(Par<Url> url) {
		throw ProtocolNotSupported(L"read", ::toS(*this));
	}

	OStream *Protocol::write(Par<Url> url) {
		throw ProtocolNotSupported(L"write", ::toS(*this));
	}

	Bool Protocol::exists(Par<Url> url) {
		throw ProtocolNotSupported(L"exists", ::toS(*this));
	}


	/**
	 * File protocol.
	 */

	FileProtocol::FileProtocol() {}

	FileProtocol::FileProtocol(Par<FileProtocol> o) {}

	void FileProtocol::output(wostream &to) const {
		to << L"file";
	}

#ifdef WINDOWS

	Bool FileProtocol::partEq(Par<Str> a, Par<Str> b) {
		return a->v.compareNoCase(b->v) == 0;
	}

	String FileProtocol::format(Par<Url> url) {
		std::wostringstream to;

		Auto<ArrayP<Str>> parts = url->getParts();
		if (parts->count() == 0)
			return to.str();

		const String &first = parts->at(0)->v;
		if (first.size() == 2 && first[1] == ':') {
			to << first;
		} else {
			to << L"\\\\" << first;
		}

		for (nat i = 1; i < parts->count(); i++) {
			to << L"\\" << parts->at(i)->v;
		}

		return to.str();
	}

	ArrayP<Url> *FileProtocol::children(Par<Url> url) {
		Auto<ArrayP<Url>> result = CREATE(ArrayP<Url>, this);
		String search = format(url) + L"\\*";

		WIN32_FIND_DATA data;
		HANDLE h = FindFirstFile(search.c_str(), &data);
		if (h == INVALID_HANDLE_VALUE)
			return result.ret();

		do {
			const wchar *name = data.cFileName;
			if (wcscmp(name, L".") == 0 || wcscmp(name, L"..") == 0)
				continue;

			if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				result->push(url->pushDir(name));
			} else {
				result->push(url->push(name));
			}
		} while (FindNextFile(h, &data));

		FindClose(h);

		return result.ret();
	}

	IStream *FileProtocol::read(Par<Url> url) {
		return CREATE(IFileStream, this, format(url));
	}

	OStream *FileProtocol::write(Par<Url> url) {
		throw ProtocolNotSupported(L"write", ::toS(*this));
	}

	Bool FileProtocol::exists(Par<Url> url) {
		return PathFileExists(format(url).c_str()) == TRUE;
	}

#else

	Bool FileProtocol::partEq(Par<Str> a, Par<Str> b) {
		return a->v == b->v;
	}

#error "Please implement FileProtocol for your OS!"
#endif

}
