#include "stdafx.h"
#include "Protocol.h"
#include "Storm/Lib/Str.h"

namespace storm {

	Protocol::Protocol() {}

	Protocol::Protocol(Par<Protocol> o) {}

	Bool Protocol::partEq(Par<Str> a, Par<Str> b) {
		return a->v == b->v;
	}


	/**
	 * File protocol.
	 */

	FileProtocol::FileProtocol() {}

	FileProtocol::FileProtocol(Par<FileProtocol> o) {}

	Bool FileProtocol::partEq(Par<Str> a, Par<Str> b) {
#ifdef WINDOWS
		return a->v.compareNoCase(b->v) == 0;
#else
		return a->v == b->v;
#endif
	}

	void FileProtocol::output(wostream &to) const {
		to << L"file";
	}

}
