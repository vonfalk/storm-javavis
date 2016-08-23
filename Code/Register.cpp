#include "stdafx.h"
#include "Register.h"

namespace code {

	const wchar *name(Register r) {
		switch (r) {
		case noReg:
			return L"<none>";
		case ptrStack:
			return L"ptrStack";
		case ptrFrame:
			return L"ptrFrame";
		case ptrA:
			return L"ptrA";
		case ptrB:
			return L"ptrB";
		case ptrC:
			return L"ptrC";
		case al:
			return L"al";
		case bl:
			return L"bl";
		case cl:
			return L"cl";
		case eax:
			return L"eax";
		case ebx:
			return L"ebx";
		case ecx:
			return L"ecx";
		case rax:
			return L"rax";
		case rbx:
			return L"rbx";
		case rcx:
			return L"rcx";
		default:
			TODO(L"Add support for backend-specific registers.");
			assert(false);
			return L"<unknown>";
		}
	}

	Size size(Register r) {
		nat v = (nat)r;
		switch (v) {
		case 0:
			return Size::sPtr;
		case 1:
			return Size::sByte;
		case 4:
			return Size::sNat;
		case 8:
			return Size::sLong;
		default:
			assert(false, L"Unknown size for register: " + ::toS(name(r)));
			return Size::sPtr;
		}
	}

	Register asSize(Register r, Size size) {
		nat s = 0;
		if (size == Size::sPtr) {
			s = 0;
		} else if (size == Size::sByte) {
			s = 1;
		} else if (size == Size::sNat) {
			s = 4;
		} else if (size == Size::sLong) {
			s = 8;
		} else {
			return noReg;
		}

		nat v = (nat)r;
		return Register((v & ~0x800) | s << 8);
	}

}
