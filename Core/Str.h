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
		Str(const wchar *s);

#ifdef POSIX
		// If wchar_t is a different size, allow creation from literals with wchar_t as well.
		Str(const wchar_t *s);
#endif

		// Create from a substring of a c-string.
		Str(const wchar *from, const wchar *to);

		// Create a string from a buffer.
		Str(GcArray<wchar> *data);

		// Create from a single char or series of chars.
		STORM_CTOR Str(Char ch);
		STORM_CTOR Str(Char ch, Nat count);

		// Empty?
		Bool STORM_FN empty() const;
		Bool STORM_FN any() const;

		// Concatenation.
		Str *STORM_FN operator +(Str *o) const;
		Str *operator +(const wchar *o) const;

		// Multiplication.
		Str *STORM_FN operator *(Nat times) const;

		// Equals (TODO: new interface).
		virtual Bool STORM_FN equals(Object *o) const;

		// Hash.
		virtual Nat STORM_FN hash() const;

		// Is this an integer?
		Bool STORM_FN isInt() const;

		// Interpret as numbers.
		Int STORM_FN toInt() const;
		Nat STORM_FN toNat() const;
		Long STORM_FN toLong() const;
		Word STORM_FN toWord() const;
		Float STORM_FN toFloat() const;

		// Interpret as hexadecimal numbers.
		Nat STORM_FN hexToNat() const;
		Word STORM_FN hexToWord() const;

		// Escape/unescape characters. Any unknown escape sequences are kept as they are.
		Str *unescape() const;
		Str *unescape(Char extra) const;
		Str *escape() const;
		Str *escape(Char extra) const;

		// Starts/ends with?
		Bool STORM_FN startsWith(const Str *s) const;
		Bool STORM_FN endsWith(const Str *s) const;
		Bool startsWith(const wchar *s) const;
		Bool endsWith(const wchar *s) const;

		// Deep copy (nothing needs to be done really).
		virtual void STORM_FN deepCopy(CloneEnv *env);

		// To string.
		virtual Str *STORM_FN toS() const;
		virtual void STORM_FN toS(StrBuf *buf) const;

		// Get a c-string.
		const wchar *c_str() const;

		// Get an UTF-8 encoded c-string allocated on the GC heap.
		const char *utf8_str() const;

		// Convert to/from cr-lf line endings. Returns the same string if possible.
		const Str *STORM_FN toCrLf() const;
		const Str *STORM_FN fromCrLf() const;

		// Compare to c-string.
		Bool operator ==(const wchar *s) const;
		Bool operator !=(const wchar *s) const;

		// Peek at the length of the underlying representation.
		Nat peekLength() const;

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
			Iter STORM_FN operator ++(int dummy);

			// Compare.
			Bool STORM_FN operator ==(const Iter &o) const;
			Bool STORM_FN operator !=(const Iter &o) const;

			// Get the value.
			Char operator *() const;
			Char STORM_FN v() const;

			// Peek at the raw offset.
			inline Nat offset() const { return pos; }

		private:
			friend class Str;

			// Create iterator to start.
			Iter(const Str *str, Nat pos);

			// String we're referring to.
			const Str *owner;
			Nat pos;

			// At the end?
			bool atEnd() const;
		};

		// Begin and end.
		Iter STORM_FN begin() const;
		Iter STORM_FN end() const;

		// Get an iterator to a specific position.
		Iter posIter(Nat pos) const;

		// Substring.
		Str *STORM_FN substr(Iter from) const;
		Str *STORM_FN substr(Iter from, Iter to) const;

		// Remove characters from the middle of the string.
		Str *STORM_FN remove(Iter from, Iter to) const;

		// Insert an entire string at a given position.
		Str *STORM_FN insert(Iter pos, Str *str) const;

	private:
		friend class Iter;
		friend class StrBuf;

		// Data we're storing. Always null-terminated or null.
		GcArray<wchar> *data;

		// Number of characters in 'data'.
		inline size_t charCount() const { return data->count - 1; }

		// Concatenation constructor.
		Str(const Str *a, const Str *b);
		Str(const Str *a, const wchar *b);

		// Repetition constructor.
		Str(const Str *a, Nat times);

		// Create from two substrings of a c-string.
		Str(const wchar *fromA, const wchar *toA, const wchar *fromB, const wchar *toB);

		// Create by inserting a string at a specific position.
		Str(const Str *into, const Iter &pos, const Str *insert);

		// Allocate 'data'.
		void allocData(nat count);

		// Convert an iterator to a pointer.
		const wchar *toPtr(const Iter &i) const;

		// Validate this string.
		void validate() const;
	};

	// Remove the indentation from a string.
	Str *STORM_FN removeIndentation(Str *str);

	// Remove leading and trailing empty lines from a string.
	Str *STORM_FN trimBlankLines(Str *src);
}
