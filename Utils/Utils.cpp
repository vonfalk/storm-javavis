// stdafx.cpp : source file that includes just the standard includes
// Spel.pch will be the pre-compiled header
// stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"
#include "Platform.h"

#ifdef WINDOWS

// Check alignment of value.
static inline bool aligned(volatile void *v) {
	UINT_PTR i = (UINT_PTR)v;
	return (i & 0x3) == 0;
}

#define check_aligned(v) \
	assert(aligned(&v), toHex((void *)&v) + L" is not properly aligned");

nat atomicIncrement(volatile nat &v) {
	check_aligned(v);
	return (nat)InterlockedIncrement((volatile LONG *)&v);
}

nat atomicDecrement(volatile nat &v) {
	check_aligned(v);
	return (nat)InterlockedDecrement((volatile LONG *)&v);
}

nat atomicCAS(volatile nat &v, nat compare, nat exchange) {
	check_aligned(v);
	return (nat)InterlockedCompareExchange((volatile LONG *)&v, (LONG)exchange, (LONG)compare);
}

nat atomicRead(volatile nat &v) {
	check_aligned(v);
	// Volatile reads are atomic on X86/X64 as long as they are aligned.
	return v;
}

void atomicWrite(volatile nat &v, nat value) {
	check_aligned(v);
	// Volatile writes are atomic on X86/X64 as long as they are aligned.
	v = value;
}

#ifdef X86

nat unalignedAtomicRead(volatile nat &v) {
	volatile nat *addr = &v;
	nat result;
	__asm {
		mov eax, 0;
		mov ecx, addr;
		lock add eax, [ecx];
	}
	return result;
}

void unalignedAtomicWrite(volatile nat &v, nat value) {
	volatile nat *addr = &v;
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
