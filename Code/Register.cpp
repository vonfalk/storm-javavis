#include "stdafx.h"
#include "Register.h"
#include "MachineCode.h"

namespace code {

	bool fromBackend(Register r) {
		return r >= 0x100;
	}

	const wchar_t *name(Register r) {
		switch (r) {
			case noReg:
				return L"";
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
				if (fromBackend(r)) {
					const wchar_t *z = machine::name(r);
					if (z) return z;
				}
				assert(false);
				return L"<unknown>";
		}
	}

	nat size(Register r) {
		return (int(r) >> 4) & 0xF;
	}

	Register asSize(Register r, nat size) {
		int v = int(r);
		v &= ~0xF0;
		v |= (size & 0xF) << 4;
		return Register(v);
	}

	//////////////////////////////////////////////////////////////////////////
	// Registers
	//////////////////////////////////////////////////////////////////////////

	Registers::Registers() {}

	Registers::Registers(Register r) {
		regs.insert(r);
	}

	Registers Registers::all() {
		Registers r;
		r += ptrA;
		r += ptrB;
		r += ptrC;
		return r;
	}

	bool Registers::contains(Register r) const {
		return regs.count(asSize(r)) == 1
			|| regs.count(asSize(r, 1)) == 1
			|| regs.count(asSize(r, 4)) == 1
			|| regs.count(asSize(r, 8)) == 1;
	}

	Register Registers::largest(Register r) const {
		assert(sizeof(cpuNat) == 4 || sizeof(cpuNat) == 8);

		if (regs.count(asSize(r, 1)) == 1) return asSize(r, 1);
		if (regs.count(asSize(r, 4)) == 1) return asSize(r, 4);
		if (regs.count(asSize(r, 0)) == 1) return asSize(r, 0); // Works since sizeof(ptr) is either 4 or 8.
		if (regs.count(asSize(r, 8)) == 1) return asSize(r, 8);

		assert(false);
		return noReg;
	}

	Registers &Registers::operator +=(Register r) {
		regs.insert(r);
		return *this;
	}

	Registers &Registers::operator +=(const Registers &r) {
		regs.insert(r.begin(), r.end());
		return *this;
	}

	Registers &Registers::operator -=(Register r) {
		regs.erase(asSize(r));
		regs.erase(asSize(r, 1));
		regs.erase(asSize(r, 4));
		regs.erase(asSize(r, 8));
		return *this;
	}

	Registers &Registers::operator -=(const Registers &r) {
		for (iterator i = r.begin(); i != r.end(); i++) {
			*this -= *i;
		}
		return *this;
	}

	Registers &Registers::operator &=(const Registers &r) {
		set<Register> result;

		for (iterator i = begin(); i != end(); i++) {
			if (r.contains(*i))
				result.insert(*i);
		}

		std::swap(regs, result);

		return *this;
	}

	String Registers::toString() const {
		vector<String> names;
		names.reserve(regs.size());

		for (iterator i = begin(); i != end(); i++) {
			names.push_back(name(*i));
		}

		return join(names, L", ");
	}
}