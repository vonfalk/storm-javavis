#pragma once
#include "Utils/Math.h"

namespace code {

	/**
	 * Size assuming a specific pointer size. Used to implement "Size". "Size" is
	 * preferred in other cases.
	 */
	template <nat16 ptr>
	class ISize {
	public:
		// Initialize to a specific size.
		ISize(nat16 size = 0) : size(size) {}

		// Add another aligned size.
		ISize<ptr> &operator +=(const ISize &o) {
			if (o.size == 0)
				return *this;

			nat16 align = min(o.size, ptr);
			size = roundUp(size, align) + o.size;
			return *this;
		}

		// Our size. (may be negative, since offsets may be negative).
		nat16 size;
	};


}
