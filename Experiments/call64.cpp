// Examine how simple structures are passed on X86-64.
#include <cstdint>
typedef uint32_t nat;
typedef uint16_t nat16;
typedef int16_t int16;
typedef uint64_t uint64;
typedef uint64_t nat64;
typedef int64_t int64;
typedef unsigned char byte;
typedef char16_t wchar;

// Easy case. Everything is aligned to 8-byte boundaries.
struct A {
	int a, b; // Passed in RDI
	int c, d; // Passed in RSI
};

// If one reads the ABI closely, this might be too large to be passed in registers.
struct B {
	// Indeed, this is passed on the stack!
	nat64 a, b, c;
};

// This one should definitely be too large.
struct C {
	// Indeed, this is passed on the stack!
	nat64 a, b, c, d, e;
};

// What happens if a float and an int is located in the same 64-bit word?
struct D {
	nat a; // Passed in RDI
	float b; // Passed in RDI alongside 'a'.
	nat c; // Passed in RSI
	float d; // Passed in RSI alongside 'c'.
};

// Can we mix integers and doubles to increase the limit a bit?
struct E {
	// No. It seems like anything larger than 2 words is not put into registers.
	nat64 a, b;
	double c;
};

// Are floats passed as doubles, or in another fashion?
struct F {
	float a, b; // Passed in XMM0
	float c, d; // Passed in XMM1
};


/**
 * Test code.
 */

void a(A a);
void callA() {
	A v = { 0, 1, 2, 3 };
	a(v);
}

void b(B b);
void callB() {
	B v = { 0, 1, 2 };
	b(v);
}

void c(C c);
void callC() {
	C v = { 0, 1, 2, 3, 4 };
	c(v);
}

void d(D d);
void callD() {
	D v = { 1, 2.0f, 2, 3.0f };
	d(v);
}

void e(E e);
void callE() {
	E v = { 1, 2 };
	e(v);
}

void f(F f);
void callF() {
	F v = { 1, 2, 3, 4 };
	f(v);
}
