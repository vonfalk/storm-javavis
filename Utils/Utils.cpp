#include "stdafx.h"
#include "Platform.h"

// Check alignment of value.
#if defined(X86)
static inline bool alignedPtr(volatile void *v) {
	size_t i = (size_t)v;
	return (i & 0x3) == 0;
}
static inline bool alignedNat(volatile void *v) {
	size_t i = (size_t)v;
	return (i & 0x3) == 0;
}
#elif defined(X64)
static inline bool alignedPtr(volatile void *v) {
	size_t i = (size_t)v;
	return (i & 0x7) == 0;
}
static inline bool alignedNat(volatile void *v) {
	size_t i = (size_t)v;
	return (i & 0x3) == 0;
}
#endif

#define check_aligned_ptr(v) \
	assert(alignedPtr(&v), toHex((void *)&v) + L" is not properly aligned");
#define check_aligned_nat(v) \
	assert(alignedNat(&v), toHex((void *)&v) + L" is not properly aligned");

#ifdef WINDOWS
#include <intrin.h>
#pragma intrinsic(_ReadWriteBarrier)
#ifdef X64
#error "Revise the atomics for 64-bit Windows!"

nat atomicIncrement(volatile nat &v) {
	check_aligned_nat(v);
	return (size_t)InterlockedIncrement((volatile LONG *)&v);
}

nat atomicDecrement(volatile nat &v) {
	check_aligned_nat(v);
	return (size_t)InterlockedDecrement((volatile LONG *)&v);
}

size_t atomicIncrement(volatile size_t &v) {
	check_aligned_ptr(v);
	return (size_t)InterlockedIncrement64((volatile LONG64 *)&v);
}

size_t atomicDecrement(volatile size_t &v) {
	check_aligned_ptr(v);
	return (size_t)InterlockedDecrement64((volatile LONG64 *)&v);
}

size_t atomicAnd(volatile size_t &v, size_t with) {
	check_aligned_ptr(v);
	return (size_t)InterlockedAnd64((volatile LONG64 *)&v, with);
}

size_t atomicOr(volatile size_t &v, size_t with) {
	check_aligned_ptr(v);
	return (size_t)InterlockedOr64((volatile LONG64 *)&v, with);
}

size_t atomicXor(volatile size_t &v, size_t with) {
	check_aligned_ptr(v);
	return (size_t)InterlockedXor64((volatile LONG64 *)&v, with);
}

nat atomicRead(volatile nat &v) {
	check_aligned_nat(v);
	_ReadWriteBarrier();
	// Volatile reads are atomic on X86/X64 as long as they are aligned.
	return v;
}

void atomicWrite(volatile nat &v, nat value) {
	check_aligned_nat(v);
	_ReadWriteBarrier();
	// Volatile writes are atomic on X86/X64 as long as they are aligned.
	v = value;
}

#else

size_t atomicIncrement(volatile size_t &v) {
	check_aligned_ptr(v);
	return (size_t)InterlockedIncrement((volatile LONG *)&v);
}

size_t atomicDecrement(volatile size_t &v) {
	check_aligned_ptr(v);
	return (size_t)InterlockedDecrement((volatile LONG *)&v);
}

size_t atomicAnd(volatile size_t &v, size_t with) {
	check_aligned_ptr(v);
	return (size_t)_InterlockedAnd((volatile LONG *)&v, with);
}

size_t atomicOr(volatile size_t &v, size_t with) {
	check_aligned_ptr(v);
	return (size_t)_InterlockedOr((volatile LONG *)&v, with);
}

size_t atomicXor(volatile size_t &v, size_t with) {
	check_aligned_ptr(v);
	return (size_t)_InterlockedXor((volatile LONG *)&v, with);
}

#endif


size_t atomicCAS(volatile size_t &v, size_t compare, size_t exchange) {
	check_aligned_ptr(v);
	return (size_t)InterlockedCompareExchangePointer((void *volatile*)&v, (void *)exchange, (void *)compare);
}

void *atomicCAS(void *volatile &v, void *compare, void *exchange) {
	check_aligned_ptr(v);
	return InterlockedCompareExchangePointer(&v, exchange, compare);
}


size_t atomicRead(volatile size_t &v) {
	check_aligned_ptr(v);
	_ReadWriteBarrier();
	// Volatile reads are atomic on X86/X64 as long as they are aligned.
	return v;
}

void *atomicRead(void *volatile &v) {
	check_aligned_ptr(v);
	_ReadWriteBarrier();
	// Volatile reads are atomic on X86/X64 as long as they are aligned.
	return v;
}

void atomicWrite(volatile size_t &v, size_t value) {
	check_aligned_ptr(v);
	_ReadWriteBarrier();
	// Volatile writes are atomic on X86/X64 as long as they are aligned.
	v = value;
}

void atomicWrite(void *volatile &v, void *value) {
	check_aligned_ptr(v);
	_ReadWriteBarrier();
	// Volatile writes are atomic on X86/X64 as long as they are aligned.
	v = value;
}

#ifdef X86

size_t unalignedAtomicRead(volatile size_t &v) {
	volatile size_t *addr = &v;
	size_t result;
	__asm {
		mov eax, 0;
		mov ecx, addr;
		lock add eax, [ecx];
		mov result, eax;
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

void shortUnalignedAtomicWrite(volatile nat &v, nat value) {
	volatile nat *addr = &v;
	__asm {
		mov eax, value;
		mov ecx, addr;
		lock xchg [ecx], eax;
	}
}

#else
#error "Implement unaligned atomics for X86-64 as well!"
#endif

#elif defined(GCC)
#define BARRIER asm volatile ("" ::: "memory")

size_t atomicIncrement(volatile size_t &v) {
	check_aligned_ptr(v);
	return __sync_add_and_fetch(&v, 1);
}

size_t atomicDecrement(volatile size_t &v) {
	check_aligned_ptr(v);
	return __sync_sub_and_fetch(&v, 1);
}

size_t atomicAnd(volatile size_t &v, size_t with) {
	check_aligned_ptr(v);
	return __sync_fetch_and_and(&v, with);
}

size_t atomicOr(volatile size_t &v, size_t with) {
	check_aligned_ptr(v);
	return __sync_fetch_and_or(&v, with);
}

size_t atomicXor(volatile size_t &v, size_t with) {
	check_aligned_ptr(v);
	return __sync_fetch_and_xor(&v, with);
}

size_t atomicCAS(volatile size_t &v, size_t compare, size_t exchange) {
	check_aligned_ptr(v);
	return __sync_val_compare_and_swap(&v, compare, exchange);
}

void *atomicCAS(void *volatile &v, void *compare, void *exchange) {
	check_aligned_ptr(v);
	return __sync_val_compare_and_swap(&v, compare, exchange);
}

#ifdef X64
nat atomicIncrement(volatile nat &v) {
	check_aligned_nat(v);
	return __sync_add_and_fetch(&v, 1);
}

nat atomicDecrement(volatile nat &v) {
	check_aligned_nat(v);
	return __sync_sub_and_fetch(&v, 1);
}

nat atomicCAS(volatile nat &v, nat compare, nat exchange) {
	check_aligned_nat(v);
	return __sync_val_compare_and_swap(&v, compare, exchange);
}

nat atomicRead(volatile nat &v) {
	check_aligned_nat(v);
	BARRIER;
	// Volatile reads are atomic on X86/X64 as long as they are aligned.
	return v;
}

void atomicWrite(volatile nat &v, nat value) {
	check_aligned_nat(v);
	BARRIER;
	// Volatile writes are atomic on X86/X64 as long as they are aligned.
	v = value;
}

#endif

#if defined(X86) || defined(X64)

size_t atomicRead(volatile size_t &v) {
	check_aligned_ptr(v);
	BARRIER;
	// Volatile reads are atomic on X86/X64 as long as they are aligned.
	return v;
}

void *atomicRead(void *volatile &v) {
	check_aligned_ptr(v);
	BARRIER;
	// Volatile reads are atomic on X86/X64 as long as they are aligned.
	return v;
}

void atomicWrite(volatile size_t &v, size_t value) {
	check_aligned_ptr(v);
	BARRIER;
	// Volatile writes are atomic on X86/X64 as long as they are aligned.
	v = value;
}

void atomicWrite(void *volatile &v, void *value) {
	check_aligned_ptr(v);
	BARRIER;
	// Volatile writes are atomic on X86/X64 as long as they are aligned.
	v = value;
}

#endif

#if defined(X86)

size_t unalignedAtomicRead(volatile size_t &v) {
	size_t result;
	asm (
		"movl $0, %%eax\n\t"
		"movl %[addr], %%ecx\n\t"
		"lock addl (%%ecx), %%eax\n\t"
		"movl %%eax, %[result]\n\t"
		: [result] "=r"(result)
		: [addr] "r"(&v)
		: "eax", "ecx", "memory");
	return result;
}

void unalignedAtomicWrite(volatile size_t &v, size_t value) {
	asm (
		"movl %[value], %%eax\n\t"
		"movl %[addr], %%ecx\n\t"
		"lock xchgl %%eax, (%%ecx)\n\t"
		:
		: [addr] "r"(&v), [value] "r"(value)
		: "eax", "ecx", "memory");
}

void shortUnalignedAtomicWrite(volatile nat &v, nat value) {
	asm (
		"movl %[value], %%eax\n\t"
		"movl %[addr], %%ecx\n\t"
		"lock xchgl %%eax, (%%ecx)\n\t"
		:
		: [addr] "r"(&v), [value] "r"(value)
		: "eax", "ecx", "memory");
}

#elif defined(X64)

size_t unalignedAtomicRead(volatile size_t &v) {
	size_t result;
	asm (
		"movq $0, %%rax\n\t"
		"movq %[addr], %%rcx\n\t"
		// "lock addq (%%rcx), %%rax\n\t"
		"movq (%%rcx), %%rax\n\t" // TODO: It seems X86 will make sure this is atomic...
		"movq %%rax, %[result]\n\t"
		: [result] "=r"(result)
		: [addr] "r"(&v)
		: "rax", "rcx", "memory");
	return result;
}

void unalignedAtomicWrite(volatile size_t &v, size_t value) {
	asm (
		"movq %[value], %%rax\n\t"
		"movq %[addr], %%rcx\n\t"
		"lock xchgq %%rax, (%%rcx)\n\t"
		:
		: [addr] "r"(&v), [value] "r"(value)
		: "rax", "rcx", "memory");
}

void shortUnalignedAtomicWrite(volatile nat &v, nat value) {
	asm (
		"movl %[value], %%eax\n\t"
		"movq %[addr], %%rcx\n\t"
		"lock xchgl %%eax, (%%rcx)\n\t"
		:
		: [addr] "r"(&v), [value] "r"(value)
		: "rax", "rcx", "memory");
}

#else
#error "Unaligned operations not supported for this architecture yet."
#endif

#else
#error "atomicIncrement and atomicDecrement are only supported on Windows and Unix for now"
#endif
