#include "stdafx.h"
#include "Range.h"

namespace storm {
	namespace server {
		using namespace storm::syntax;

		Range::Range() : from(0), to(0) {}

		Range::Range(Nat from, Nat to) {
			this->from = min(from, to);
			this->to = max(from, to);
		}

		Bool Range::intersects(Range other) const {
			return to > other.from
				&& from < other.to;
		}

		Bool Range::contains(Nat pt) const {
			return from <= pt && to > pt;
		}

		static Nat delta(Nat a, Nat b) {
			if (a > b)
				return a - b;
			else
				return b - a;
		}

		Nat Range::distance(Nat pt) const {
			if (contains(pt))
				return 0;
			return min(delta(from, pt), delta(to, pt));
		}

		void Range::deepCopy(CloneEnv *env) {}

		StrBuf &operator <<(StrBuf &to, Range r) {
			return to << L"(" << r.from << L" - " << r.to << L")";
		}

		wostream &operator <<(wostream &to, Range r) {
			return to << L"(" << r.from << L" - " << r.to << L")";
		}

		Range merge(Range a, Range b) {
			if (a.empty())
				return b;
			if (b.empty())
				return a;
			return Range(min(a.from, b.from),
						max(a.to, b.to));
		}

		ColoredRange::ColoredRange(Range r, TokenColor c)
			: range(r), color(c) {}

		StrBuf &operator <<(StrBuf &to, ColoredRange r) {
			return to << r.range << L"#" << name(to.engine(), r.color);
		}

	}
}
