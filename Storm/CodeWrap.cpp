#include "stdafx.h"
#include "CodeWrap.h"
#include "Shared/Str.h"

namespace storm {

	wrap::Ref::Ref(const code::Ref &r) : v(r) {}


	wostream &wrap::operator <<(wostream &to, const Ref &r) {
		return to << "ref " << r.v.targetName();
	}

	Str *wrap::toS(EnginePtr e, Ref r) {
		return CREATE(Str, e.v, ::toS(r));
	}

}
