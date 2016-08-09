#pragma once
#include "Utils/Bitwise.h"

namespace code {

	/**
	 * Size assuming a specific pointer size. Used to implement "Size". "Size" is
	 * preferred in other cases.
	 */
	template <nat ptr>
	class ISize {
	public:
		// Initialize to a specific size.
		ISize(nat size = 0) {
			assert((size & sizeMask) == size);
			assert((ptr & alignMaskLow) == ptr);

			this->size = size;
			// Generally, it is not useful to align to more than the machine word size.
			this->align = max(nat(1), min(ptr, size));
		}

		// Initialize to previously obtained values.
		ISize(nat size, nat align) : size(size), align(align) {
			assert((size & sizeMask) == size);
			assert((ptr & alignMaskLow) == ptr);
		}

		// Add another aligned size.
		ISize<ptr> &operator +=(const ISize &o) {
			// Update our alignment requirement.
			align = max(align, o.align);

			// Align and add.
			// Note: our alignment is not always important; consider the following example:
			// int, bool, bool, where the booleans do not have to be aligned to 4 bytes like
			// the int. If we had used 'align' instead of 'o.align' we would have aligned them
			// too strict.
			size = roundUp(size, o.align) + o.size;
			return *this;
		}

		// Multiply this by a positive constant.
		ISize<ptr> &operator *=(nat o) {
			if (o == 0) {
				size = 0;
			} else if (o == 1) {
				// Identity
			} else {
				nat more = (o - 1) * roundUp(size, align);
				size += more;
			}
			return *this;
		}


		// Data.
		struct {
			// Upper 4 bits used to indicate alignment
			nat align : 4;
			// lower 28 for size.
			nat size : 28;
		};

		// Constants.
		enum {
			alignMaskLow = 0x0000000F,
			alignMask    = 0xF0000000,
			sizeMask     = 0x0FFFFFFF,
		};
	};


	template <nat ptr>
	wostream &operator <<(wostream &to, const ISize<ptr> &o) {
		return to << toHex(o.size, true) << L"(" << toHex(o.align, false) << L")";
	}

}
