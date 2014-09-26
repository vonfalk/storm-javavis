#include "stdafx.h"
#include "Math.h"

namespace util {

	nat nextPowerOfTwo(nat number) {
		--number;

		// This could be implemented like a row of x |= x >> 1, but I think we can safely assume that loop unrolling
		// will take care of this for us. This also makes the function work regardless of how large a nat is.
		for (nat i = 1; i < sizeof(number) * 8; i *= 2) {
			number |= number >> i;
		}

		return number + 1;
	}

	nat trailingZeros(nat number) {
		ASSERT(number != 0);
		nat r = 0;
		while ((number & 0x1) == 0) {
			r += 1;
			number >>= 1;
		}
		return r;
	}
}