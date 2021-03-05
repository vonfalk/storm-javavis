#include "stdafx.h"
#include "Protocol.h"
#include "Url.h"
#include "Core/Convert.h"
#include "Str.h"
#include "StrBuf.h"
#include "FileStream.h"

#ifdef WINDOWS
#include <Shlwapi.h>
#endif

namespace storm {

	ProtocolNotSupported::ProtocolNotSupported(const wchar *operation, const wchar *protocol)
		: operation(new (engine()) Str(operation)), protocol(new (engine()) Str(protocol)) {
		saveTrace();
	}

	ProtocolNotSupported::ProtocolNotSupported(const wchar *operation, Str *protocol)
		: operation(new (engine()) Str(operation)), protocol(protocol) {
		saveTrace();
	}

	ProtocolNotSupported::ProtocolNotSupported(Str *operation, Str *protocol)
		: operation(operation), protocol(protocol) {
		saveTrace();
	}

	void ProtocolNotSupported::message(StrBuf *to) const {
		*to << operation << S(" is not supported by the protocol ") << protocol;
	}


	Protocol::Protocol() {}

	Bool Protocol::partEq(Str *a, Str *b) {
		return *a == *b;
	}

	Nat Protocol::partHash(Str *part) {
		return part->hash();
	}

	Array<Url *> *Protocol::children(Url *url) {
		throw new (this) ProtocolNotSupported(S("children"), toS());
	}

	IStream *Protocol::read(Url *url) {
		throw new (this) ProtocolNotSupported(S("read"), toS());
	}

	OStream *Protocol::write(Url *url) {
		throw new (this) ProtocolNotSupported(S("write"), toS());
	}

	Bool Protocol::exists(Url *url) {
		throw new (this) ProtocolNotSupported(S("exists"), toS());
	}

	Bool Protocol::createDir(Url *url) {
		throw new (this) ProtocolNotSupported(S("createDir"), toS());
	}

	Str *Protocol::format(Url *url) {
		throw new (this) ProtocolNotSupported(S("format"), toS());
	}

	void Protocol::toS(StrBuf *to) const {
		*to << S("<unknown protocol>");
	}

	Bool Protocol::operator ==(const Protocol &o) const {
		return runtime::typeOf(this) == runtime::typeOf(&o);
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

	Nat FileProtocol::partHash(Str *a) {
		const wchar *str = a->c_str();

		Nat r = 5381;
		for (const wchar *at = str; *at; at++)
			r = ((r << 5) + r) + towlower(*at);

		return r;
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
		return new (url) FileIStream(format(url));
	}

	OStream *FileProtocol::write(Url *url) {
		return new (url) FileOStream(format(url));
	}

	Bool FileProtocol::exists(Url *url) {
		return PathFileExists(format(url)->c_str()) == TRUE;
	}

	Bool FileProtocol::createDir(Url *url) {
		return CreateDirectory(format(url)->c_str(), NULL) == TRUE;
	}

#elif defined(POSIX)

	Bool FileProtocol::partEq(Str *a, Str *b) {
		return *a == *b;
	}

	Nat FileProtocol::partHash(Str *a) {
		return a->hash();
	}

	Str *FileProtocol::format(Url *url) {
		StrBuf *to = new (this) StrBuf();

		Array<Str *> *parts = url->getParts();
		if (parts->count() == 0)
			return to->toS();

		for (Nat i = 0; i < parts->count(); i++) {
			*to << S("/") << parts->at(i);
		}

		// if (url->dir)
		// 	*to << L"/";

		return to->toS();
	}

	Array<Url *> *FileProtocol::children(Url *url) {
		Array<Url *> *result = new (url) Array<Url *>();

		DIR *h = opendir(format(url)->utf8_str());
		if (!h)
			return result;

		dirent *d;
		while ((d = readdir(h)) != null) {
			if (strcmp(d->d_name, "..") == 0 || strcmp(d->d_name, ".") == 0)
				continue;

			Str *name = new (this) Str(toWChar(engine(), d->d_name)->v);
			if (d->d_type == DT_DIR) {
				result->push(url->pushDir(name));
				// TODO: Handle 'DT_UNKNOWN' properly...
			} else {
				result->push(url->push(name));
			}
		}

		closedir(h);

		return result;
	}

	IStream *FileProtocol::read(Url *url) {
		return new (url) FileIStream(format(url));
	}

	OStream *FileProtocol::write(Url *url) {
		return new (url) FileOStream(format(url));
	}

	Bool FileProtocol::exists(Url *url) {
		struct stat s;
		return stat(format(url)->utf8_str(), &s) == 0;
	}

	Bool FileProtocol::createDir(Url *url) {
		return mkdir(format(url)->utf8_str(), 0777) == 0;
	}

#else
#error "Please implement FileProtocol for your OS!"
#endif


	/**
	 * HTTP/HTTPS
	 */

	HttpProtocol::HttpProtocol(Bool secure) : secure(secure) {}

	Bool HttpProtocol::partEq(Str *a, Str *b) {
		return *a == *b;
	}

	Nat HttpProtocol::partHash(Str *a) {
		return a->hash();
	}

	Str *HttpProtocol::format(Url *url) {
		throw new (this) ProtocolNotSupported(S("format"), S("http"));
	}

	Array<Url *> *HttpProtocol::children(Url *url) {
		throw new (this) ProtocolNotSupported(S("children"), S("http"));
	}

	IStream *HttpProtocol::read(Url *url) {
		throw new (this) ProtocolNotSupported(S("read"), S("http"));
	}

	OStream *HttpProtocol::write(Url *url) {
		throw new (this) ProtocolNotSupported(S("write"), S("http"));
	}

	Bool HttpProtocol::exists(Url *url) {
		throw new (this) ProtocolNotSupported(S("exists"), S("http"));
	}

	Bool HttpProtocol::createDir(Url *url) {
		return false;
	}

	void HttpProtocol::toS(StrBuf *to) const {
		if (secure)
			*to << S("https://");
		else
			*to << S("http://");
	}

}
