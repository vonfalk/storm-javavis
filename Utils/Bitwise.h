#pragma once

// Math utilities

// Check if a number is a power of two
inline bool isPowerOfTwo(nat number) {
	return (number & (number - 1)) == 0;
}

// Get the next larger power of two
nat nextPowerOfTwo(nat number);

// Get the number of trailing zeros (assumed to be a power of two).
nat trailingZeros(nat number);

// Count the number of set bits.
nat setBitCount(nat number);

// Round up to the nearest multiple of "multiple"
// T should be an integer number, preferrably unsigned.
template <class T>
inline T roundUp(T number, T multiple) {
	T remainder = number % multiple;
	if (remainder == 0) return number;
	return number + (multiple - remainder);
}
