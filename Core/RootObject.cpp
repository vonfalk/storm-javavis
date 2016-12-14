#include "stdafx.h"
#include "RootObject.h"
#include "Str.h"
#include "StrBuf.h"

namespace storm {

	RootObject::RootObject() {}

	RootObject::RootObject(const RootObject &o) {}

	RootObject::~RootObject() {}

	Str *RootObject::toS() const {
		StrBuf *b = new (this) StrBuf();
		toS(b);
		return b->toS();
	}

	void RootObject::toS(StrBuf *buf) const {
		Type *t = runtime::typeOf(this);
		if (t) {
			*buf << runtime::typeName(t);
		} else {
			*buf << L"<unknown type>";
		}
		*buf << L" @" << (void *)this;
	}

	wostream &operator <<(wostream &to, const RootObject *o) {
		if (o) {
			return to << o->toS()->c_str();
		} else {
			return to << L"<null>";
		}
	}

	wostream &operator <<(wostream &to, const RootObject &o) {
		return operator <<(to, &o);
	}

}
