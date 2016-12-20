#include "stdafx.h"
#include "Protocol.h"
#include "Url.h"
#include "Str.h"
#include "StrBuf.h"
#include "FileStream.h"

#ifdef WINDOWS
#include <Shlwapi.h>
#endif

namespace storm {

	Protocol::Protocol() {}

	Bool Protocol::partEq(Str *a, Str *b) {
		return a->equals(b);
	}

	Array<Url *> *Protocol::children(Url *url) {
		throw ProtocolNotSupported(L"children", ::toS(*this));
	}

	IStream *Protocol::read(Url *url) {
		throw ProtocolNotSupported(L"read", ::toS(*this));
	}

	OStream *Protocol::write(Url *url) {
		throw ProtocolNotSupported(L"write", ::toS(*this));
	}

	Bool Protocol::exists(Url *url) {
		throw ProtocolNotSupported(L"exists", ::toS(*this));
	}

	Str *Protocol::format(Url *url) {
		throw ProtocolNotSupported(L"format", ::toS(*this));
	}

	void Protocol::toS(StrBuf *to) const {
		*to << L"<unknown protocol>";
	}


	/**
	 * File protocol.
	 */

	FileProtocol::FileProtocol() {}

	void FileProtocol::toS(StrBuf *to) const {
#ifndef WINDOWS
		*to << L"/";
#endif
	}

#ifdef WINDOWS

	Bool FileProtocol::partEq(Str *a, Str *b) {
		return _wcsicmp(a->c_str(), b->c_str()) == 0;
	}

	Str *FileProtocol::format(Url *url) {
		StrBuf *to = new (this) StrBuf();

		Array<Str *> *parts = url->getParts();
		if (parts->count() == 0)
			return to->toS();

		Str *first = parts->at(0);
		if (wcslen(first->c_str()) == 2 && first->c_str()[1] == ':') {
			*to << first;
		} else {
			*to << L"\\\\" << first;
		}

		for (nat i = 1; i < parts->count(); i++) {
			*to << L"\\" << parts->at(i);
		}

		return to->toS();
	}

	Array<Url *> *FileProtocol::children(Url *url) {
		Array<Url *> *result = new (this) Array<Url *>();
		Str *search = *format(url) + new (url) Str(L"\\*");

		WIN32_FIND_DATA data;
		HANDLE h = FindFirstFile(search->c_str(), &data);
		if (h == INVALID_HANDLE_VALUE)
			return result;

		do {
			const wchar *name = data.cFileName;
			if (wcscmp(name, L".") == 0 || wcscmp(name, L"..") == 0)
				continue;

			Str *sName = new (this) Str(name);
			if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				result->push(url->pushDir(sName));
			} else {
				result->push(url->push(sName));
			}
		} while (FindNextFile(h, &data));

		FindClose(h);

		return result;
	}

	IStream *FileProtocol::read(Url *url) {
		return new (url) IFileStream(format(url));
	}

	OStream *FileProtocol::write(Url *url) {
		throw ProtocolNotSupported(L"write", ::toS(*this));
	}

	Bool FileProtocol::exists(Url *url) {
		return PathFileExists(format(url)->c_str()) == TRUE;
	}

#else

	Bool FileProtocol::partEq(Str *a, Str *b) {
		return a->equals(b);
	}

#error "Please implement FileProtocol for your OS!"
#endif

}
