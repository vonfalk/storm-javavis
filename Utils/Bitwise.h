#pragma once

/**
 * Bitwise math utilities.
 *
 * Assumes they are called with some *unsigned* type, preferrably 4 or 8 byte ones.
 */

/**
 * Helper class for size-dependent specializations.
 */
template <class T, size_t numSize>
struct BitwiseImpl {};

template <class T>
struct BitwiseImpl<T, 4> {
	static nat trailingZeros(T v) {
		// From Bit Twiddling Hacks: https://graphics.stanford.edu/~seander/bithacks.html
		static const nat vals[] = { 0xAAAAAAAA, 0xCCCCCCCC, 0xF0F0F0F0, 0xFF00FF00, 0xFFFF0000 };
		nat r = (v & vals[0]) != 0;
		for (nat i = 4; i > 0; i--)
			r |= ((v & vals[i]) != 0) << i;
		return r;
	}

	static nat setBitCount(T number) {
		// From Bit Twiddling Hacks: https://graphics.stanford.edu/~seander/bithacks.html
		number = number - ((number >> 1) & 0x55555555);
		number = (number & 0x33333333) + ((number >> 2) & 0x33333333);
		return ((number + (number >> 4) & 0x0F0F0F0F) * 0x01010101) >> 24;
	}
};

template <class T>
struct BitwiseImpl<T, 8> {
	static nat trailingZeros(T v) {
		// From Bit Twiddling Hacks: https://graphics.stanford.edu/~seander/bithacks.html
		static const nat64 vals[] = {
			0xAAAAAAAAAAAAAAAAULL, 0xCCCCCCCCCCCCCCCCULL, 0xF0F0F0F0F0F0F0F0ULL,
			0xFF00FF00FF00FF00ULL, 0xFFFF0000FFFF0000ULL, 0xFFFFFFFF00000000ULL
		};
		nat r = (v & vals[0]) != 0;
		for (nat i = 5; i > 0; i--)
			r |= ((v & vals[i]) != 0) << i;
		return r;
	}

	static nat setBitCount(T number) {
		// From Bit Twiddling Hacks: https://graphics.stanford.edu/~seander/bithacks.html
		number = number - ((number >> 1) & 0x5555555555555555ULL);
		number = (number & 0x3333333333333333ULL) + ((number >> 2) & 0x3333333333333333ULL);
		number = (number + (number >> 4)) & 0x0F0F0F0F0F0F0F0FULL;
		return (number * 0x0101010101010101ULL) >> 56;
	}
};


// Check if a number is a power of two.
template <class T>
inline bool isPowerOfTwo(T number) {
	return (number & (number - 1)) == 0;
}

// Get the next larger power of two.
template <class T>
inline T nextPowerOfTwo(T number) {
	--number;

	for (size_t i = 1; i < sizeof(number) * CHAR_BIT; i *= 2)
		number |= number >> i;

	return number + 1;
}

// Get the number of trailing zeros, assuming "number" is a power of two.
template <class T>
inline nat trailingZeros(T number) {
	return BitwiseImpl<T, sizeof(T)>::trailingZeros(number);
}

// Count the number of set bits.
template <class T>
inline nat setBitCount(T number) {
	return BitwiseImpl<T, sizeof(T)>::setBitCount(number);
}

// Round up to the nearest multiple of "multiple"
// T should be an integer number, preferrably unsigned.
template <class T>
inline T roundUp(T number, T multiple) {
	T remainder = number % multiple;
	if (remainder == 0) return number;
	return number + (multiple - remainder);
}


/**
 * Compile-time computation of powers of two.
 */
template <size_t value, size_t iter = 1>
struct NextPowerOfTwo {
	static const size_t value = NextPowerOfTwo<((value - 1) | ((value - 1) >> iter)) + 1>::value;
};

template <size_t val>
struct NextPowerOfTwo<val, sizeof(size_t) * CHAR_BIT> {
	static const size_t value = val;
};
