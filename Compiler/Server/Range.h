#pragma once
#include "Core/StrBuf.h"
#include "Compiler/Syntax/TokenColor.h"

namespace storm {
	namespace server {
		STORM_PKG(core.lang.server);

		/**
		 * Represents a range in a file.
		 */
		class Range {
			STORM_VALUE;
		public:
			STORM_CTOR Range();
			STORM_CTOR Range(Nat from, Nat to);

			Nat from;
			Nat to;

			// Is this range empty?
			inline Bool STORM_FN empty() const { return from == to; }

			// Does this range intersect another range?
			Bool STORM_FN intersects(Range other) const;

			// Deep copy.
			void STORM_FN deepCopy(CloneEnv *env);
		};

		StrBuf &STORM_FN operator <<(StrBuf &to, Range r);

		// Compute the union of two ranges.
		Range STORM_FN merge(Range a, Range b);

		/**
		 * Represents a a region to be colored in a file.
		 */
		class ColoredRange {
			STORM_VALUE;
		public:
			STORM_CTOR ColoredRange(Range r, syntax::TokenColor c);

			Range range;

			// TODO: Use strings or symbols here?
			syntax::TokenColor color;
		};

		StrBuf &STORM_FN operator <<(StrBuf &to, ColoredRange r);

	}
}
