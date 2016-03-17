#include "stdafx.h"
#include "SyntaxObject.h"
#include "Shared/Str.h"
#include "Shared/Io/Url.h"

namespace storm {

	SStr::SStr(Par<Str> s) : v(s) {}

	SStr::SStr(Par<Str> src, SrcPos pos) : pos(pos), v(src) {}

	SStr::SStr(Par<SStr> s) : pos(pos), v(s->v) {}

	SStr::SStr(const String &str) {
		v = CREATE(Str, engine(), str);
	}

	SStr::SStr(const String &str, const SrcPos &pos) : pos(pos){
		v = CREATE(Str, engine(), str);
	}

	Str *SStr::transform() const {
		return Auto<Str>(v).ret();
	}

	void SStr::output(wostream &to) const {
		to << *v << L"@" << steal(pos.file->name()) << '(' << pos.offset << ')';
	}

	void SStr::deepCopy(Par<CloneEnv> env) {
		pos.deepCopy(env);
		v.deepCopy(env);
	}

	Auto<SStr> sstr(Engine &e, const String &str) {
		return CREATE(SStr, e, str);
	}

	Auto<SStr> sstr(Engine &e, const String &str, const SrcPos &pos) {
		return CREATE(SStr, e, str, pos);
	}

}
