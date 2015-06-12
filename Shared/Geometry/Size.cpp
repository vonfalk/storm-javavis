#include "stdafx.h"
#include "Size.h"
#include "Str.h"

namespace storm {
	namespace geometry {

		Size::Size() : w(0), h(0) {}

		Size::Size(Int wh) : w(wh), h(wh) {}

		Size::Size(Int w, Int h) : w(w), h(h) {}

		Bool Size::valid() const {
			return w >= 0 && h >= 0;
		}

		wostream &operator <<(wostream &to, const Size &s) {
			return to << L"(" << s.w << L", " << s.h << L")";
		}

		Str *toS(EnginePtr e, Size s) {
			return CREATE(Str, e.v, ::toS(s));
		}

		Size STORM_FN operator +(Size a, Size b) {
			return Size(a.w + b.w, a.h + b.h);
		}

		Size STORM_FN operator -(Size a, Size b) {
			return Size(a.w - b.w, a.h - b.h);
		}

		Size STORM_FN operator *(Int s, Size a) {
			return Size(a.w * s, a.h * s);
		}

		Size STORM_FN operator *(Size a, Int s) {
			return Size(a.w * s, a.h * s);
		}

	}
}
