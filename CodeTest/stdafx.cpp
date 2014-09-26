// stdafx.cpp : source file that includes just the standard includes
// AsmTest.pch will be the pre-compiled header
// stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"

// TODO: reference any additional headers you need in STDAFX.H
// and not in this file

int callFn(const void *fnPtr, int p) {
	cpuNat oldEsi = 0xDEADBEEF, oldEdi = 0xFEBA1298, oldEbx = 0x81732A98, oldEbp;
	cpuNat newEsi, newEdi, newEbx, newEbp;
	int rv;
	__asm {
		mov edi, oldEdi;
		mov esi, oldEsi;
		mov ebx, oldEbx;
		mov oldEbp, ebp;
		push p;
		call fnPtr;
		add esp, 4;
		mov rv, eax;
		mov newEbp, ebp;
		mov newEbx, ebx;
		mov newEsi, esi;
		mov newEdi, edi;
	}

	assert(oldEbp == newEbp);
	assert(oldEbx == newEbx);
	assert(oldEsi == newEsi);
	assert(oldEdi == newEdi);

	return rv;
}

int64 callFn(const void *fnPtr, int64 p) {
	cpuNat oldEsi = 0xDEADBEEF, oldEdi = 0xFEBA1298, oldEbx = 0x81732A98, oldEbp;
	cpuNat newEsi, newEdi, newEbx, newEbp;
	
	int ph = p >> 32;
	int pl = p & 0xFFFFFFFF;

	nat rvh;
	nat rvl;
	__asm {
		mov edi, oldEdi;
		mov esi, oldEsi;
		mov ebx, oldEbx;
		mov oldEbp, ebp;
		push pl;
		push ph;
		call fnPtr;
		add esp, 8;
		mov rvl, eax;
		mov rvh, edx;
		mov newEbp, ebp;
		mov newEbx, ebx;
		mov newEsi, esi;
		mov newEdi, edi;
	}

	assert(oldEbp == newEbp);
	assert(oldEbx == newEbx);
	assert(oldEsi == newEsi);
	assert(oldEdi == newEdi);

	nat64 r = (nat64(rvh) << 32) | nat64(rvl);
	return r;
}