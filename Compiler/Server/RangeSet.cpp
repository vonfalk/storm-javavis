#include "stdafx.h"
#include "RangeSet.h"
#include "Core/CloneEnv.h"

namespace storm {
	namespace server {

		RangeSet::RangeSet() {
			parts = new (this) Array<Range>();
		}

		RangeSet::RangeSet(Range r) {
			parts = new (this) Array<Range>();
			if (!r.empty())
				parts->push(r);
		}

		void RangeSet::deepCopy(CloneEnv *env) {
			cloned(parts, env);
		}

		Bool RangeSet::intersects(Range a, Range b) {
			return a.from <= b.to
				&& b.from <= a.to;
		}

		void RangeSet::insert(Range insert) {
			if (insert.empty())
				return;

			Nat pos = findUpper(insert.from);
			if (pos > 0)
				pos--;

			while (pos < parts->count()) {
				Range at = parts->at(pos);

				if (intersects(at, insert)) {
					// Remove the range in the array and continue to insert the expanded 'insert'.
					insert = merge(at, insert);
					parts->remove(pos);
				} else if (insert.to < at.from) {
					// 'insert' is after 'at'. We have found the correct place!
					break;
				} else {
					// Keep looking...
					pos++;
				}
			}

			if (pos >= parts->count())
				parts->push(insert);
			else
				parts->insert(pos, insert);
		}

		void RangeSet::insert(RangeSet *src) {
			for (Nat i = 0; i < src->count(); i++)
				insert(src->at(i));
		}

		void RangeSet::remove(Range remove) {
			if (remove.empty())
				return;

			Nat pos = findUpper(remove.from);
			if (pos > 0) {
				// left.from <= remove.from.
				Range &left = parts->at(pos - 1);
				if (left.from >= remove.from && left.to <= remove.to) {
					// Remove it.
					parts->remove(--pos);
				} else if (left.from >= remove.from) {
					// Shorten the beginning.
					left.from = remove.to;
				} else if (left.to > remove.from) {
					// Shorten the end.
					left.to = remove.from;
				}
			}

			while (pos < parts->count()) {
				Range &at = parts->at(pos);

				if (remove.to <= at.from) {
					// Done. We're past any sections that will possibly intersect.
					break;
				} else if (at.to <= remove.to) {
					// Remove this one entirely.
					parts->remove(pos);
				} else {
					// Shorten the beginning.
					at.from = remove.to;
				}
			}
		}

		Bool RangeSet::has(Nat point) const {
			Nat pos = findUpper(point);
			if (pos == 0)
				return false;

			Range r = parts->at(--pos);
			return r.contains(point);
		}

		Range RangeSet::cover(Nat point) const {
			Nat pos = findUpper(point);
			if (pos == 0)
				return Range();

			Range r = parts->at(--pos);
			if (r.contains(point))
				return r;
			else
				return Range();
		}

		Range RangeSet::cover(Range range) const {
			Nat pos = findUpper(range.from);
			if (pos > 0)
				pos--;

			while (pos < parts->count()) {
				Range at = parts->at(pos);

				if (intersects(at, range)) {
					range = merge(at, range);
					pos++;
				} else if (range.to < at.from) {
					break;
				} else {
					pos++;
				}
			}

			return range;
		}

		Range RangeSet::nearest(Nat point) const {
			Nat pos = findUpper(point);

			Range result;
			if (pos > 0) {
				result = parts->at(pos - 1);
			}
			if (pos < parts->count()) {
				Range c = parts->at(pos);
				if (result.empty())
					result = c;
				else if (c.distance(point) < result.distance(point))
					result = c;
			}

			return result;
		}

		void RangeSet::toS(StrBuf *to) const {
			parts->toS(to);
		}

		Nat RangeSet::findUpper(Nat v) const {
			Nat from = 0;
			Nat to = parts->count();

			while (from < to) {
				Nat mid = (to - from) / 2 + from;

				if (parts->at(mid).from <= v) {
					from = mid + 1;
				} else {
					to = mid;
				}
			}

			return from;
		}

	}
}
