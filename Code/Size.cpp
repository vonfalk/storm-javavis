#include "stdafx.h"
#include "Size.h"

namespace code {

	Size::Size() {}

	Size::Size(nat s) : s32(nat16(s)), s64(nat16(s)) {}

	Size::Size(nat s32, nat s64) : s32(nat16(s32)), s64(nat16(s64)) {}

	Size Size::sPtr = Size(4, 8);
	Size Size::sChar = Size(1);
	Size Size::sByte = Size(1);
	Size Size::sInt = Size(4);
	Size Size::sNat = Size(4);
	Size Size::sLong = Size(8);
	Size Size::sWord = Size(8);

	nat Size::currentSize() const {
		switch (sizeof(void *)) {
		case 4:
			return s32.size;
		case 8:
			return s64.size;
		default:
			assert(("Only 32 and 64-bit platforms are supported now.", false));
			return 0;
		}
	}

	Size &Size::operator +=(const Size &o) {
		s32 += o.s32;
		s64 += o.s64;
		return *this;
	}

	Size Size::operator +(const Size &o) const {
		Size t = *this;
		t += o;
		return t;
	}

	bool Size::operator ==(const Size &o) const {
		return s32.size == o.s32.size && s64.size == o.s64.size;
	}

	bool Size::operator !=(const Size &o) const {
		return !(*this == o);
	}

	wostream &operator <<(wostream &to, const Size &s) {
		if (s.s32.size == s.s64.size)
			to << toHex(s.s32.size);
		else
			to << toHex(s.s32.size) << "/" << toHex(s.s64.size);
		return to;
	}

	bool Size::operator <(const Size &o) const {
		return s32.size < o.s32.size;
	}

	bool Size::operator >(const Size &o) const {
		return s32.size > o.s32.size;
	}

	bool Size::operator >=(const Size &o) const {
		return s32.size >= o.s32.size;
	}

	bool Size::operator <=(const Size &o) const {
		return s32.size <= o.s32.size;
	}


	Offset::Offset() {}

	Offset::Offset(int s) : o32(short(s)), o64(short(s)) {}

	Offset::Offset(Size s) : o32(s.s32.size), o64(s.s64.size) {}

	Offset::Offset(int o32, int o64) : o32(short(o32)), o64(short(o64)) {}

	Offset Offset::sPtr = Offset(4, 8);
	Offset Offset::sChar = Offset(1);
	Offset Offset::sByte = Offset(1);
	Offset Offset::sInt = Offset(4);
	Offset Offset::sNat = Offset(4);
	Offset Offset::sLong = Offset(8);
	Offset Offset::sWord = Offset(8);

	int Offset::current() const {
		switch (sizeof(void *)) {
		case 4:
			return o32;
		case 8:
			return o64;
		default:
			assert(("Only 32 and 64-bit platforms are supported now.", false));
			return 0;
		}
	}

	Offset &Offset::operator +=(const Offset &o) {
		o32 += o.o32;
		o64 += o.o64;
		return *this;
	}

	Offset &Offset::operator -=(const Offset &o) {
		o32 -= o.o32;
		o64 -= o.o64;
		return *this;
	}

	Offset &Offset::operator +=(const Size &o) {
		o32 += o.s32.size;
		o64 += o.s64.size;
		return *this;
	}

	Offset &Offset::operator -=(const Size &o) {
		o32 -= o.s32.size;
		o64 -= o.s64.size;
		return *this;
	}

	Offset Offset::operator -() const {
		return Offset(-o32, -o64);
	}

	bool Offset::operator ==(const Offset &o) const {
		return o32 == o.o32 && o64 == o.o64;
	}

	bool Offset::operator !=(const Offset &o) const {
		return !(*this == o);
	}

	wostream &operator <<(wostream &to, const Offset &s) {
		if (s.o32 == s.o64)
			to << toHex(s.o32);
		else
			to << toHex(s.o32) << "/" << toHex(s.o64);
		return to;
	}

	bool Offset::operator <(const Offset &o) const {
		return o32 < o.o32;
	}

	bool Offset::operator >(const Offset &o) const {
		return o32 > o.o32;
	}

	bool Offset::operator >=(const Offset &o) const {
		return o32 >= o.o32;
	}

	bool Offset::operator <=(const Offset &o) const {
		return o32 <= o.o32;
	}

	String Offset::format(bool sign) const {
		std::wostringstream o;
		if (*this < Offset())
			o << L"-";
		else
			o << L"+";
		o << abs();
		return o.str();
	}

	Offset Offset::abs() const {
		return Offset(::abs(o32), ::abs(o64));
	}

}

code::Offset abs(code::Offset s) {
	return s.abs();
}

