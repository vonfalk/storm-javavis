#pragma once
#include "ISize.h"

namespace code {

	/**
	 * Machine-independent size description. This class allows composition
	 * of primitive types to composite types. For composite types, the
	 * class keeps track of the needed alignment for the type as well.
	 *
	 * This is currently implemented as two separate sizes, one assuming
	 * 32-bit pointers and the other assuming 64-bit pointers. This will
	 * suffice for a while, and may be extended in the future.
	 */
	class Size {
		friend class Offset;
	public:
		// Initialize to zero.
		Size();

		// Initialize to a known size (platform independent).
		explicit Size(nat s);

		// Initialize to previously obtained values.
		Size(nat size32, nat align32, nat size64, nat align64);

		// Get the size for the current platform.
		nat current() const;

		// Get a Size of zero, only with the align of this Size.
		Size align() const;

		// Pointer size.
		static Size sPtr;

		// Integer sizes.
		static Size sChar;
		static Size sByte;
		static Size sInt;
		static Size sNat;
		static Size sLong;
		static Size sWord;

		// Float sizes.
		static Size sFloat;

		// Addition (note that we do not have subtraction).
		Size &operator +=(const Size &o);
		Size operator +(const Size &o) const;

		// Multiplication with positive. Equal to repeated addition.
		Size &operator *=(nat o);
		Size operator *(nat o) const;

		// Equality check.
		bool operator ==(const Size &o) const;
		bool operator !=(const Size &o) const;

		// Greater/lesser?
		bool operator <(const Size &o) const;
		bool operator >(const Size &o) const;
		bool operator >=(const Size &o) const;
		bool operator <=(const Size &o) const;

		// Find out the 32- and 64-bit sizes (for storage).
		inline nat size32() const { return s32.size; }
		inline nat size64() const { return s64.size; }
		inline nat align32() const { return s32.align; }
		inline nat align64() const { return s64.align; }

		// Output.
		friend wostream &operator <<(wostream &to, const Size &s);
	private:
		// Initialize to specific values.
		Size(nat s32, nat s64);

		// 32-bit ptr size.
		ISize<4> s32;

		// 64-bit ptr size.
		ISize<8> s64;
	};

	/**
	 * Output.
	 */
	wostream &operator <<(wostream &to, const Size &s);


	/**
	 * Offset. Differently from size, an offset does not keep alignment!
	 */
	class Offset {
		friend class Size;
	public:
		// Initialize to zero.
		Offset();

		// Initialize to a known size (platform independent).
		explicit Offset(int s);

		// Convert from Size.
		explicit Offset(Size s);

		// Initialize to specific values.
		Offset(int s32, int s64);

		// Get the size for the current platform.
		int current() const;

		// Pointer size.
		static Offset sPtr;

		// Integer sizes.
		static Offset sChar;
		static Offset sByte;
		static Offset sInt;
		static Offset sNat;
		static Offset sLong;
		static Offset sWord;

		// Addition and subtraction
		template <class O>
		Offset operator +(const O &o) const {
			Offset t = *this;
			t += o;
			return t;
		}
		template <class O>
		Offset operator -(const O &o) const {
			Offset t = *this;
			t -= o;
			return t;
		}

		Offset &operator +=(const Offset &o);
		Offset &operator -=(const Offset &o);

		// Addition/subtraction with Size
		Offset &operator +=(const Size &o);
		Offset &operator -=(const Size &o);

		// Multiplication.
		Offset &operator *=(int o);
		Offset operator *(int o) const;

		// Negation. 'add' will still move further from zero.
		Offset operator -() const;

		// Equality check.
		bool operator ==(const Offset &o) const;
		bool operator !=(const Offset &o) const;

		// Greater/lesser?
		bool operator <(const Offset &o) const;
		bool operator >(const Offset &o) const;
		bool operator >=(const Offset &o) const;
		bool operator <=(const Offset &o) const;

		// Formatted string output.
		String format(bool sign) const;

		// Find out the 32- and 64-bit sizes (for storage).
		inline nat v32() const { return o32; }
		inline nat v64() const { return o64; }

		// Output.
		friend wostream &operator <<(wostream &to, const Offset &s);

		Offset abs() const;
	private:
		// 32-bit offset.
		int o32;

		// 64-bit offset.
		int o64;
	};

	/**
	 * Output.
	 */
	wostream &operator <<(wostream &to, const Size &s);

}

// Abs.
code::Offset abs(code::Offset s);

