#pragma once

#ifdef X64

struct Reg64 {
	size_t rbx; // @0
	size_t r12; // @8
	size_t r13; // @16
	size_t r14; // @24
	size_t r15; // @32
};

inline bool operator ==(const Reg64 &a, const Reg64 &b) {
	return a.rbx == b.rbx
		&& a.r12 == b.r12
		&& a.r13 == b.r13
		&& a.r14 == b.r14
		&& a.r15 == b.r15;
}

inline bool operator !=(const Reg64 &a, const Reg64 &b) {
	return !(a == b);
}

inline wostream &operator <<(wostream &to, const Reg64 &r) {
	return to << L"rbx: " << (void *)r.rbx
			  << L", r12: " << (void *)r.r12
			  << L", r13: " << (void *)r.r13
			  << L", r14: " << (void *)r.r14
			  << L", r15: " << (void *)r.r15;
}

extern "C" size_t checkCall(const void *fn, size_t param, Reg64 *regs);

#endif
