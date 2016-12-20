#include "stdafx.h"
#include "SStr.h"
#include "Core/CloneEnv.h"
#include "Core/StrBuf.h"

namespace storm {
	namespace syntax {

		SStr::SStr(const wchar *src) {
			v = new (this) Str(src);
		}

		SStr::SStr(const wchar *src, SrcPos pos) : pos(pos) {
			v = new (this) Str(src);
		}

		SStr::SStr(Str *src) : v(src) {}

		SStr::SStr(Str *src, SrcPos pos) : pos(pos), v(src) {}

		Str *SStr::transform() const {
			return v;
		}

		void SStr::deepCopy(CloneEnv *env) {
			cloned(pos, env);
			cloned(v, env);
		}

		void SStr::toS(StrBuf *to) const {
			*to << v << L"@" << pos;
		}

	}
}
