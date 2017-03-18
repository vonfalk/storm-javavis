#include "stdafx.h"
#include "Platform.h"

#ifdef WINDOWS
#ifdef X64
#error "Revise the atomics for 64-bit Windows!"
#endif

// Check alignment of value.
static inline bool aligned(volatile void *v) {
	UINT_PTR i = (UINT_PTR)v;
	return (i & 0x3) == 0;
}

#define check_aligned(v) \
	assert(aligned(&v), toHex((void *)&v) + L" is not properly aligned");

size_t atomicIncrement(volatile size_t &v) {
	check_aligned(v);
	return (size_t)InterlockedIncrement((volatile LONG *)&v);
}

size_t atomicDecrement(volatile size_t &v) {
	check_aligned(v);
	return (size_t)InterlockedDecrement((volatile LONG *)&v);
}

size_t atomicCAS(volatile size_t &v, size_t compare, size_t exchange) {
	check_aligned(v);
	return (size_t)InterlockedCompareExchangePointer((void *volatile*)&v, (void *)exchange, (void *)compare);
}

void *atomicCAS(void *volatile &v, void *compare, void *exchange) {
	check_aligned(v);
	return InterlockedCompareExchangePointer(&v, exchange, compare);
}


size_t atomicRead(volatile size_t &v) {
	check_aligned(v);
	// Volatile reads are atomic on X86/X64 as long as they are aligned.
	return v;
}

void *atomicRead(void *volatile &v) {
	check_aligned(v);
	// Volatile reads are atomic on X86/X64 as long as they are aligned.
	return v;
}

void atomicWrite(volatile size_t &v, size_t value) {
	check_aligned(v);
	// Volatile writes are atomic on X86/X64 as long as they are aligned.
	v = value;
}

void atomicWrite(void *volatile &v, void *value) {
	check_aligned(v);
	// Volatile writes are atomic on X86/X64 as long as they are aligned.
	v = value;
}

#ifdef X86

size_t unalignedAtomicRead(volatile size_t &v) {
	volatile size_t *addr = &v;
	nat result;
	__asm {
		mov eax, 0;
		mov ecx, addr;
		lock add eax, [ecx];
	}
	return result;
}

void unalignedAtomicWrite(volatile size_t &v, size_t value) {
	volatile size_t *addr = &v;
	__asm {
		mov eax, value;
		mov ecx, addr;
		lock xchg [ecx], eax;
	}
}

#endif

#else
#error "atomicIncrement and atomicDecrement are only supported on Windows for now"
#endif
