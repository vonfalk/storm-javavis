#pragma once

// Math utilities
namespace util {

	// Check if a number is a power of two
	inline bool isPowerOfTwo(nat number) {
		return (number & (number - 1)) == 0;
	}

	// Get the next larger power of two
	nat nextPowerOfTwo(nat number);

	// Get the number of trailing zeros (assumed to be a power of two).
	nat trailingZeros(nat number);

	// Round up to the nearest multiple of "multiple"
	inline nat roundUp(nat number, nat multiple) {
		nat remainder = number % multiple;
		if (remainder == 0) return number;
		return number + (multiple - remainder);
	}
}