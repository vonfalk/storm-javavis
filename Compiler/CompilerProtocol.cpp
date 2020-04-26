#include "stdafx.h"
#include "CompilerProtocol.h"
#include "Core/Io/Url.h"

namespace storm {

	CompilerProtocol::CompilerProtocol() : libName(null) {}

	CompilerProtocol::CompilerProtocol(Str *libName) : libName(libName) {}

	Bool CompilerProtocol::partEq(Str *a, Str *b) {
		return *a == *b;
	}

	Nat CompilerProtocol::partHash(Str *a) {
		return a->hash();
	}

	Str *CompilerProtocol::format(Url *url) {
#ifdef DEBUG
		StrBuf *to = new (this) StrBuf();
#ifdef WINDOWS
		const wchar *sep = S("\\");
#else
		const wchar *sep = S("/");
#endif

		Url *root = dbgRootUrl(engine());
		if (libName)
			root = *root / libName;
		*to << root->format();

		Array<Str *> *parts = url->getParts();
		for (Nat i = 0; i < parts->count(); i++) {
			*to << sep << parts->at(i);
		}

		return to->toS();
#else
		throw new (this) ProtocolNotSupported(S("format"), toS());
#endif
	}

	void CompilerProtocol::toS(StrBuf *to) const {
#ifdef DEBUG
		Url *root = dbgRootUrl(engine());
		if (libName)
			root = root->pushDir(libName);
		*to << root;
#else
		*to << S("source for ");
		if (libName)
			*to << libName;
		else
			*to << S("compiler");
		*to << S(":");
#endif
	}

	Bool CompilerProtocol::operator ==(const Protocol &o) const {
		const CompilerProtocol *other = as<const CompilerProtocol>(&o);
		if (!other)
			return false;

		if (libName == other->libName)
			return true;

		if (libName == null || other->libName == null)
			return false;

		return *libName == *other->libName;
	}

}
