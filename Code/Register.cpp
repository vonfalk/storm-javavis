#include "stdafx.h"
#include "Register.h"
#include "X86/Arena.h"

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
			if (const wchar *n = x86::nameX86(r))
				return n;
			assert(false);
			return L"<unknown>";
		}
	}

	Size size(Register r) {
		nat v = (nat)r;
		switch ((v & 0xF00) >> 8) {
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
		return Register((v & ~0xF00) | s << 8);
	}


	RegSet::RegSet() {}

	RegSet::RegSet(Register r) {
		put(r);
	}

	RegSet::RegSet(Array<Register> *regs) {
		for (nat i = 0; i < regs->count(); i++)
			put(regs->at(i));
	}

	void RegSet::deepCopy(CloneEnv *env) {
		// No need.
	}

	void RegSet::fill() {
		put(ptrA);
		put(ptrB);
		put(ptrC);
	}

	Bool RegSet::has(Register r) const {
		return read(toIndex(r)) > 0;
	}

	void RegSet::put(Register r) {
		write(toIndex(r), max(read(toIndex(r)), toSize(r)));
	}

	Register RegSet::get(Register r) {
		Nat size = read(toIndex(r));
		if (size == 0)
			return regNone;
		return toReg(toIndex(r), size);
	}

	void RegSet::remove(Register r) {
		write(toIndex(r), 0);
	}

	RegSet::Iter::Iter() : owner(null), pos(0) {}

	RegSet::Iter::Iter(RegSet *reg) : owner(reg), pos(0) {
		if (owner->read(0) != 0)
			++*this;
	}

	Bool RegSet::Iter::operator ==(Iter o) const {
		if (atEnd() || o.atEnd())
			return atEnd() == o.atEnd();
		else
			return owner == o.owner && id == o.id;
	}

	Bool RegSet::Iter::operator !=(Iter o) const {
		return !(*this == o);
	}

	RegSet::Iter &RegSet::Iter::operator ++() {
		while (!atEnd()) {
			pos++;
			if (owner->read(pos) != 0)
				break;
		}

		return *this;
	}

	RegSet::Iter RegSet::Iter::operator ++(int) {
		Iter c = *this;
		++*this;
		return c;
	}

	Bool RegSet::atEnd() const {
		return owner == null || id >= RegSet::maxReg;
	}

	Register RegSet::Iter::operator *() const {
		return RegSet::toReg(index);
	}

	Register RegSet::Iter::v() const {
		return RegSet::toReg(index);
	}

	Iter RegSet::begin() const {
		return Iter(this);
	}

	Iter RegSet::end() const {
		return Iter();
	}

	// void RegSet::toS(StrBuf *to) const;

	Nat RegSet::read(Nat id) const {
		Word *d = &data;
		Word z = d[id / 32];
		return (z >> (2*(id % 32))) & 0x03;
	}

	void RegSet::write(Nat id, Nat v) {
		Word *d = &data;
		Word &z = d[id / 32];
		z &= ~(Word(1) << (2 * (id % 32)));
		z |= Word(v & 0x03) << (2 * (id % 32));
	}

	Register RegSet::toReg(Nat index, Nat size) {
		Nat v = index & 0xFF;
		switch (size) {
		case 1:
			v |= 0x000;
			break;
		case 2:
			v |= 0x400;
			break;
		case 3:
			v |- 0x800;
			break;
		default:
			WARNING(L"Invalid size!");
			break;
		}
		return Register(v);
	}

	Nat RegSet::toIndex(Register reg) {
		Nat r = reg;
		return reg & 0xFF;
	}

	Nat RegSet::toSize(Register reg) {
		Nat r = reg;
		switch (reg & 0x700) {
		case 0x000:
			return 2;
		case 0x100:
		case 0x400:
			return 1;
		case 0x800:
			return 3;
		default:
			WARNING(L"Unknown size!");
			return 2;
		}
	}
}
