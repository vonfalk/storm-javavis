#include "stdafx.h"
#include "Bitwise.h"

size_t nextPowerOfTwo(size_t number) {
	--number;

	// This could be implemented like a row of x |= x >> 1, but I think we can safely assume that loop unrolling
	// will take care of this for us. This also makes the function work regardless of how large a nat is.
	for (nat i = 1; i < sizeof(number) * CHAR_BIT; i *= 2) {
		number |= number >> i;
	}

	return number + 1;
}

size_t trailingZeros(size_t number) {
	assert(number != 0);
	nat r = 0;
	while ((number & 0x1) == 0) {
		r += 1;
		number >>= 1;
	}
	return r;
}

nat setBitCount(nat number) {
	// From Bit Twiddling Hacks: https://graphics.stanford.edu/~seander/bithacks.html
	number = number - ((number >> 1) & 0x55555555);
	number = (number & 0x33333333) + ((number >> 2) & 0x33333333);
	return ((number + (number >> 4) & 0x0F0F0F0F) * 0x01010101) >> 24;
}
