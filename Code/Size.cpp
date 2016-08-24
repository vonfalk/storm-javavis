#include "stdafx.h"
#include "Size.h"
#include "Utils/Bitwise.h"
#include "Core/StrBuf.h"

namespace code {

	// Helpers for the bitmasks. Stores size+align as: 0xasssssss
	static const Nat sizeMask = 0x0FFFFFFF;
	static const Nat alignMask = 0xF0000000;
	static const Nat alignShift = 28;

	static inline Nat size(Nat in) {
		return in & sizeMask;
	}

	static inline void size(Nat &in, Nat size) {
		in = (in & ~sizeMask) | (size & sizeMask);
	}

	static inline Nat align(Nat in) {
		return (in & alignMask) >> alignShift;
	}

	static inline void align(Nat &in, Nat align, Nat maxAlign) {
		align = min(align, maxAlign);
		in = (in & ~alignMask) | ((align << alignShift) & alignMask);
	}

	static inline void set(Nat &in, Nat size, Nat maxAlign) {
		code::size(in, size);
		align(in, size, maxAlign);
	}

	static inline void add(Nat &in, Nat other, Nat maxAlign) {
		// Update alignment requirement.
		nat a = max(align(in), align(other));
		align(in, a, maxAlign);

		// Alignd and add.
		// Note: our alignment is not always important; consider the following example:
		// int, bool, bool, where the booleans do not have to be aligned to 4 bytes like the int.
		// If we had used 'align' instead of 'align(other)', we would have aligned them too strict.
		nat s = roundUp(size(in), align(other)) + size(other);
		size(in, s);
	}

	static inline void mul(Nat &in, Nat other, Nat maxAlign) {
		if (other == 0) {
			// Nothing.
			size(in, 0);
		} else if (other == 1) {
			// Identity.
		} else {
			nat extra = 0;
			size(extra, (other - 1) * roundUp(size(in), align(in)));
			align(extra, align(in), maxAlign);
			add(in, extra, maxAlign);
		}
	}

	static void output(wostream &to, Int v) {
		to << toHex(size(v)) << L"(" << toHex(align(v)) << L")";
	}

	Size::Size() : s32(0), s64(0) {}

	Size::Size(Nat s) : s32(0), s64(0) {
		set(s32, s, 4);
		set(s64, s, 8);
	}

	Size::Size(nat s32, nat s64) : s32(0), s64(0) {
		set(this->s32, s32, 4);
		set(this->s64, s64, 8);
	}

	Size::Size(nat size32, nat align32, nat size64, nat align64) : s32(0), s64(0) {
		code::size(s32, size32);
		code::align(s32, align32, 4);
		code::size(s64, size64);
		code::align(s64, align64, 8);
	}

	Size Size::sPtr = Size(4, 8);
	Size Size::sChar = Size(1);
	Size Size::sByte = Size(1);
	Size Size::sInt = Size(4);
	Size Size::sNat = Size(4);
	Size Size::sLong = Size(8);
	Size Size::sWord = Size(8);

	Size Size::sFloat = Size(4);

	nat Size::current() const {
		switch (sizeof(void *)) {
		case 4:
			return roundUp(code::size(s32), code::align(s32));
		case 8:
			return roundUp(code::size(s64), code::align(s64));
		default:
			assert(false, "Only 32 and 64-bit platforms are supported now.");
			return 0;
		}
	}

	Size Size::align() const {
		return Size(0, code::align(s32), 0, code::align(s64));
	}

	Size &Size::operator +=(const Size &o) {
		add(s32, o.s32, 4);
		add(s64, o.s64, 8);
		return *this;
	}

	Size Size::operator +(const Size &o) const {
		Size t = *this;
		t += o;
		return t;
	}

	Size &Size::operator *=(nat o) {
		mul(s32, o, 4);
		mul(s64, o, 8);
		return *this;
	}

	Size Size::operator *(nat o) const {
		Size t = *this;
		t *= o;
		return t;
	}

	bool Size::operator ==(const Size &o) const {
		return code::size(s32) == code::size(o.s32) && code::size(s64) == code::size(o.s64);
	}

	bool Size::operator !=(const Size &o) const {
		return !(*this == o);
	}

	wostream &operator <<(wostream &to, const Size &s) {
		if (code::size(s.s32) == code::size(s.s64) && code::size(s.s32) == code::size(s.s32)) {
			output(to, s.s32);
		} else {
			output(to, s.s32);
			to << L"/";
			output(to, s.s64);
		}
		return to;
	}

	StrBuf &operator <<(StrBuf &to, Size s) {
		return to << ::toS(s).c_str();
	}

	bool Size::operator <(const Size &o) const {
		return code::size(s32) < code::size(o.s32);
	}

	bool Size::operator >(const Size &o) const {
		return code::size(s32) > code::size(o.s32);
	}

	bool Size::operator >=(const Size &o) const {
		return code::size(s32) >= code::size(o.s32);
	}

	bool Size::operator <=(const Size &o) const {
		return code::size(s32) <= code::size(o.s32);
	}

	Nat Size::size32() const {
		return roundUp(code::size(s32), code::align(s32));
	}

	Nat Size::size64() const {
		return roundUp(code::size(s64), code::align(s64));
	}

	Nat Size::align32() const {
		return code::size(s32);
	}

	Nat Size::align64() const {
		return code::size(s64);
	}


	Offset::Offset() : o32(0), o64(0) {}

	Offset::Offset(Int s) : o32(s), o64(s) {}

	Offset::Offset(Size s) : o32(Int(size(s.s32))), o64(Int(size(s.s64))) {}

	Offset::Offset(Int o32, Int o64) : o32(o32), o64(o64) {}

	Offset Offset::sPtr = Offset(4, 8);
	Offset Offset::sChar = Offset(1);
	Offset Offset::sByte = Offset(1);
	Offset Offset::sInt = Offset(4);
	Offset Offset::sNat = Offset(4);
	Offset Offset::sLong = Offset(8);
	Offset Offset::sWord = Offset(8);

	Int Offset::current() const {
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

	Offset Offset::operator +(const Offset &o) const {
		Offset t = *this;
		t += o;
		return t;
	}

	Offset Offset::operator -(const Offset &o) const {
		Offset t = *this;
		t -= o;
		return t;
	}

	Offset &Offset::operator +=(const Size &o) {
		o32 += roundUp(size(o.s32), align(o.s32));
		o64 += roundUp(size(o.s64), align(o.s64));
		return *this;
	}

	Offset &Offset::operator -=(const Size &o) {
		o32 -= roundUp(size(o.s32), align(o.s32));
		o64 -= roundUp(size(o.s64), align(o.s64));
		return *this;
	}

	Offset Offset::operator +(const Size &o) const {
		Offset t = *this;
		t += o;
		return t;
	}

	Offset Offset::operator -(const Size &o) const {
		Offset t = *this;
		t -= o;
		return t;
	}

	Offset &Offset::operator *=(Int o) {
		o32 *= o;
		o64 *= o;
		return *this;
	}

	Offset Offset::operator *(Int o) const {
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
		if (s.o32 < 0)
			to << L"+";
		else
			to << L"-";
		if (s.o32 == s.o64)
			to << toHex(abs(s.o32), true);
		else
			to << toHex(abs(s.o32), true) << "/" << toHex(abs(s.o64), true);
		return to;
	}

	StrBuf &operator <<(StrBuf &to, Offset s) {
		return to << ::toS(s).c_str();
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

	Offset Offset::abs() const {
		return Offset(::abs(o32), ::abs(o64));
	}

}

code::Offset abs(code::Offset s) {
	return s.abs();
}

