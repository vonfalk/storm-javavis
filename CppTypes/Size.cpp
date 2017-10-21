#include "stdafx.h"
#include "Size.h"

Size::Size() {}

Size::Size(nat s) : s32(s), s64(s) {}

Size::Size(nat s32, nat s64) : s32(s32), s64(s64) {}

Size::Size(nat size32, nat align32, nat size64, nat align64) : s32(size32, align32), s64(size64, align64) {}

Size Size::sPtr = Size(4, 8);
Size Size::sChar = Size(1);
Size Size::sByte = Size(1);
Size Size::sInt = Size(4);
Size Size::sNat = Size(4);
Size Size::sLong = Size(8);
Size Size::sWord = Size(8);

Size Size::sFloat = Size(4);
Size Size::sDouble = Size(8);

nat Size::current() const {
	switch (sizeof(void *)) {
	case 4:
		return roundUp(s32.size, s32.align);
	case 8:
		return roundUp(s64.size, s64.align);
	default:
		assert(false, "Only 32 and 64-bit platforms are supported now.");
		return 0;
	}
}

Size Size::align() const {
	Size s;
	s.s32.align = s32.align;
	s.s64.align = s64.align;
	return s;
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

Size &Size::operator *=(nat o) {
	s32 *= o;
	s64 *= o;
	return *this;
}

Size Size::operator *(nat o) const {
	Size t = *this;
	t *= o;
	return t;
}

bool Size::operator ==(const Size &o) const {
	return s32.size == o.s32.size && s64.size == o.s64.size;
}

bool Size::operator !=(const Size &o) const {
	return !(*this == o);
}

wostream &operator <<(wostream &to, const Size &s) {
	if (s.s32.size == s.s64.size && s.s32.align == s.s64.align)
		to << s.s32;
	else
		to << s.s32 << "/" << s.s64;
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


Offset::Offset() : o32(0), o64(0) {}

Offset::Offset(int s) : o32(s), o64(s) {}

Offset::Offset(Size s) : o32(int(s.s32.size)), o64(int(s.s64.size)) {}

Offset::Offset(int o32, int o64) : o32(o32), o64(o64) {}

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
		assert(false, "Only 32 and 64-bit platforms are supported now.");
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

Offset &Offset::operator *=(int o) {
	o32 *= o;
	o64 *= o;
	return *this;
}

Offset Offset::operator *(int o) const {
	Offset z = *this;
	z *= o;
	return z;
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
		to << toHex(s.o32, true);
	else
		to << toHex(s.o32, true) << "/" << toHex(s.o64, true);
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
	if (sign) {
		if (*this < Offset())
			o << L"-";
		else
			o << L"+";
	}
	o << abs();
	return o.str();
}

Offset Offset::abs() const {
	return Offset(::abs(o32), ::abs(o64));
}
