#pragma once

// Helper macro for defining operators & and | for bitmasks declared as enums.
#define BITMASK_OPERATORS(type)											\
	inline type operator &(type a, type b) { return type(nat(a) & nat(b)); } \
	inline type operator |(type a, type b) { return type(nat(a) | nat(b)); } \
	inline type operator &=(type &a, type b) { return a = a & b; } \
	inline type operator |=(type &a, type b) { return a = a | b; } \
	inline type operator ~(type a) { return type(~nat(a)); }
