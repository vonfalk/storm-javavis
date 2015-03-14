#pragma once

// Helper macro for defining operators & and | for bitmasks declared as enums.
#define BITMASK_OPERATORS(type)											\
	inline type operator &(type a, type b) { return type(int(a) & int(b)); } \
	inline type operator |(type a, type b) { return type(int(a) | int(b)); }
