#include "stdafx.h"
#include "Reference.h"
#include "Core/StrBuf.h"
#include "Core/Str.h"

namespace code {

	Reference::Reference(RefSource *to, Content *inside) : owner(inside), to(to) {
		to->refs->put(this);
	}

	Reference::Reference(Ref to, Content *inside) {
		assert(false, L"FIXME");
	}

	void Reference::moved(const void *addr) {}

	void Reference::toS(StrBuf *t) const {
		*t << to;
	}

	Ref::Ref(RefSource *to) : to(to) {}

	Ref::Ref(Reference *ref) : to(ref->to) {}

	Ref::Ref(const Ref &o) : to(o.to) {}

	void Ref::deepCopy(CloneEnv *env) {}

	wostream &operator <<(wostream &to, const Ref &r) {
		return to << r.to->title()->c_str();
	}

	StrBuf &operator <<(StrBuf &to, Ref r) {
		return to << r.to->title();
	}

}
