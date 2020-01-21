#include "stdafx.h"
#include "Exception.h"
#include "StrBuf.h"

namespace storm {

	NException::NException() : stackTrace(engine()) {}

	NException::NException(const NException &o) : stackTrace(o.stackTrace) {}

	void NException::deepCopy(CloneEnv *env) {}

	Str *NException::message() const {
		StrBuf *b = new (this) StrBuf();
		message(b);
		return b->toS();
	}

	void NException::toS(StrBuf *to) const {
		message(to);
		if (stackTrace.any()) {
			*to << S("\n");
			stackTrace.format(to);
		}
	}


	AbstractFnCalled::AbstractFnCalled(const wchar *name) : RuntimeError(name) {}

	AbstractFnCalled::AbstractFnCalled(Str *name) : RuntimeError(name) {}

}
