#include "stdafx.h"
#include "Size.h"
#include "Str.h"
#include "Point.h"

namespace storm {
	namespace geometry {

		Size::Size() : w(0), h(0) {}

		Size::Size(Float wh) : w(wh), h(wh) {}

		Size::Size(Float w, Float h) : w(w), h(h) {}

		Size::Size(Point pt) : w(pt.x), h(pt.y) {}

		Bool Size::valid() const {
			return w >= 0.0f && h >= 0.0f;
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

		Size STORM_FN operator *(Float s, Size a) {
			return Size(a.w * s, a.h * s);
		}

		Size STORM_FN operator *(Size a, Float s) {
			return Size(a.w * s, a.h * s);
		}

		Size STORM_FN operator /(Size a, Float s) {
			return Size(a.w / s, a.h / s);
		}

		Size STORM_FN abs(Size a) {
			return Size(::abs(a.w), ::abs(a.h));
		}

	}
}
