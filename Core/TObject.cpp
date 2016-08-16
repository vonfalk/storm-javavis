#include "stdafx.h"
#include "TObject.h"

namespace storm {

	TObject::TObject(Thread *t) : thread(t) {}

	TObject::TObject(TObject *o) : thread(o->thread) {}

	Str *TObject::toS() const {
		return RootObject::toS();
	}

	void TObject::toS(StrBuf *buf) const {
		RootObject::toS(buf);
	}

}
