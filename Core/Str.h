#pragma once
#include "Object.h"
#include "Char.h"
#include "GcArray.h"

namespace storm {
	STORM_PKG(core);

	/**
	 * Custom exception.
	 */
	class StrError : public Exception {
	public:
		StrError(const String &msg) : msg(L"String error: " + msg) {}
		virtual String what() const { return msg; }
	private:
		String msg;
	};


	/**
	 * The string type used in Storm, strings are immutable. Treats a string as a sequence of
	 * unicode codepoints, but hides the underlying representation by disallowing low-level access
	 * to the string from Storm.
	 *
	 * Note: we may want to enforce proper normalization of strings to avoid weird results.
	 */
	class Str : public Object {
		STORM_CLASS;
	public:
		// Create an empty string.
		STORM_CTOR Str();

		// Create from a string literal.
		STORM_CTOR Str(const wchar *s);

		// Create a string from a buffer.
		Str(GcArray<wchar> *data);

		// Copy a string.
		STORM_CTOR Str(Str *o);

		// Create from a single char or series of chars.
		STORM_CTOR Str(Char ch);
		STORM_CTOR Str(Char ch, Nat count);

		// Empty?
		Bool STORM_FN empty() const;
		Bool STORM_FN any() const;

		// Concatenation.
		Str *STORM_FN operator +(Str *o) const;

		// Multiplication.
		Str *STORM_FN operator *(Nat times) const;

		// Equals (TODO: new interface).
		virtual Bool STORM_FN equals(Object *o) const;

		// Hash.
		virtual Nat STORM_FN hash() const;

		// Interpret as numbers.
		Int STORM_FN toInt() const;
		Nat STORM_FN toNat() const;
		Long STORM_FN toLong() const;
		Word STORM_FN toWord() const;
		Float STORM_FN toFloat() const;

		// Deep copy (nothing needs to be done really).
		virtual void STORM_FN deepCopy(CloneEnv *env);

		// To string.
		virtual Str *STORM_FN toS() const;
		virtual void STORM_FN toS(StrBuf *buf) const;

		// Get a c-string.
		wchar *c_str() const;

		/**
		 * Iterator (not implemented yet.)
		 */
		class Iter {
			STORM_VALUE;
		public:
			// Create an iterator to end.
			STORM_CTOR Iter();

			// Deep copy. No need to do anything, strings are immutable!
			void STORM_FN deepCopy(CloneEnv *env);

			// Advance.
			Iter &STORM_FN operator ++();
			Iter &STORM_FN operator ++(int dummy);

			// Compare.
			Bool STORM_FN operator ==(const Iter &o) const;
			Bool STORM_FN operator !=(const Iter &o) const;

			// Get the value.
			Char operator *() const;
			Char STORM_FN v() const;

		private:
			friend class Str;

			// Create iterator to start.
			Iter(Str *str);

			// String we're referring to.
			Str *owner;
			Nat index;

			// At the end?
			bool atEnd() const;
		};

		// Begin and end.
		Iter STORM_FN begin();
		Iter STORM_FN end();

	private:
		friend class StrBuf;

		// Data we're storing. Always null-terminated or null.
		GcArray<wchar> *data;

		// Number of characters in 'data'.
		inline size_t charCount() const { return data->count - 1; }

		// Concatenation constructor.
		Str(const Str *a, const Str *b);

		// Repetition constructor.
		Str(const Str *a, Nat times);

		// Allocate 'data'.
		void allocData(nat count);
	};

}