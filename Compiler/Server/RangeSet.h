#pragma once
#include "Range.h"
#include "Core/Object.h"
#include "Core/Array.h"

namespace storm {
	namespace server {
		STORM_PKG(core.lang.server);

		/**
		 * Represents a set of ranges. The ranges are automatically merged into their minimal form
		 * and ordered in increasing order. This means that the exposed array interface always
		 * contains non-overlapping, non-touching ranges.
		 */
		class RangeSet : public Object {
			STORM_CLASS;
		public:
			// Create an empty set.
			STORM_CTOR RangeSet();

			// Create a set with one element.
			STORM_CTOR RangeSet(Range range);

			// Deep copy.
			void STORM_FN deepCopy(CloneEnv *env);

			// Element access.
			inline Range STORM_FN operator [](Nat id) const { return parts->at(id); }
			inline Range at(Nat id) const { return parts->at(id); }
			inline Nat STORM_FN count() const { return parts->count(); }
			inline Bool STORM_FN empty() const { return parts->empty(); }
			inline Bool STORM_FN any() const { return parts->any(); }

			// Add another range. Note: that adding a new range may actually cause the total number
			// of entries in the set to decrease.
			void STORM_FN insert(Range range);
			void STORM_FN insert(RangeSet *from);

			// Remove a range.
			void STORM_FN remove(Range range);

			// See if a specific position is contained in the set.
			Bool STORM_FN has(Nat point) const;

			// Get the smallest range covering a point. Returns an empty range if the point is not
			// in this set.
			Range STORM_FN cover(Nat point) const;

			// Find the covering range of another range.
			Range STORM_FN cover(Range range) const;

			// Find the range closest to a point.
			Range STORM_FN nearest(Nat point) const;

			// To string.
			virtual void STORM_FN toS(StrBuf *to) const;

		private:
			// Storage.
			Array<Range> *parts;

			// Find the index of the first part with a larger value than 'x'. Returns 'parts->count()' if none exists.
			Nat findUpper(Nat x) const;

			// See if two ranges intersects (this includes touching edges).
			static Bool intersects(Range a, Range b);
		};

	}
}
