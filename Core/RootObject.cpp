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
		*buf << L"<TODO: Type> @" << (void *)this;
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
