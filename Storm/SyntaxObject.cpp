#include "stdafx.h"
#include "SyntaxObject.h"
#include "Shared/Str.h"

namespace storm {

	SObject::SObject() : pos() {}

	SObject::SObject(SrcPos pos) : pos(pos) {}

	SStr::SStr(Par<Str> s) : v(s) {}

	SStr::SStr(Par<Str> src, SrcPos pos) : SObject(pos), v(src) {}

	SStr::SStr(Par<SStr> s) : SObject(pos), v(s->v) {}

	SStr::SStr(const String &str) {
		v = CREATE(Str, engine(), str);
	}

	SStr::SStr(const String &str, const SrcPos &pos) : SObject(pos){
		v = CREATE(Str, engine(), str);
	}

	void SStr::output(wostream &to) const {
		to << *v << L"@" << pos;
	}
}
