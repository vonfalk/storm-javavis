#pragma once
#include "Object.h"
#include "Char.h"
#include "Utils/Exception.h"

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
	 * The string type used in Basic Storm. This string type tries its best to hide the fact that
	 * strings in unicode are complicated. This is done by disallowing low-level operations on the
	 * string (such as refering to characters based on their index), iterators are always working a
	 * logical character at a time, and so on. The interface should be agnostic to the actual
	 * storage of the string (which is currently UTF16).
	 *
	 * Currently, a character is equal to one unicode codepoint, but this may change to be one
	 * character and any extra marks contained. In this case, proper normalization is required.
	 *
	 * Later, we want to ensure that the data in the string class is always normalized, and stays
	 * that way, so to not confuse programmers and users about the relation between multiple
	 * seemingly equivalent strings.
	 */
	class Str : public Object {
		STORM_CLASS;
	public:
		// The value of this 'str' object.
		const String v;

		// From literal ctor.
		Str(const wchar *s);

		// Empty ctor
		STORM_CTOR Str();

		// Copy ctor
		STORM_CTOR Str(Par<Str> copy);

		// Create from char.
		STORM_CAST_CTOR Str(Char ch);

		// Create from char, with repeition.
		STORM_CTOR Str(Char ch, Nat times);

		// Create from regular string.
		Str(const String &s);

		// String empty?
		Bool STORM_FN empty() const;
		Bool STORM_FN any() const;

		// We can add a string length function that uses a cached length computed at creation-time,
		// but is that useful?

		// Concatenation.
		Str *STORM_FN operator +(Par<Str> o);

		// Multiplication.
		Str *STORM_FN operator *(Nat times);

		// Equals.
		virtual Bool STORM_FN equals(Par<Object> o);

		// Hash.
		virtual Nat STORM_FN hash();

		// Convert to int.
		Int STORM_FN toInt() const;

		// Convert to nat.
		Nat STORM_FN toNat() const;

		// Convert to long.
		Long STORM_FN toLong() const;

		// Convert to word.
		Word STORM_FN toWord() const;

		// Convert to float.
		Float STORM_FN toFloat() const;

		// ToS
		virtual Str *STORM_FN toS();

		// Create a string from a literal. There is a reference in Engine to this function.
		static Str *CODECALL createStr(Type *strType, const wchar *str);

		// Iterator in the string.
		class Iter {
			STORM_VALUE;
		public:
			// Create iterator to end.
			STORM_CTOR Iter();

			// Deep copy. No need to do anything, strings are immutable!
			void STORM_FN deepCopy(Par<CloneEnv> env);

			// Advance.
			Iter &STORM_FN operator ++();
			Iter STORM_FN operator ++(int dummy);

			// Compare.
			Bool STORM_FN operator ==(const Iter &o) const;
			Bool STORM_FN operator !=(const Iter &o) const;

			// Get the value.
			Char operator *() const;
			Char STORM_FN v() const;

			// For C++: get the index of the character this iterator is referring to.
			Nat charIndex() const;

			// For C++: set the index of the character this iterator is referring to. Be careful not
			// to set it outside the size of the string, or in the middle of a surrogate pair.
			void charIndex(Nat i);

		private:
			friend class Str;

			// Create iterator to start.
			Iter(Par<Str> str);

			// String we're referring to.
			Auto<Str> owner;

			// First index of the character we're referring to.
			Nat index;

			// At the end?
			bool atEnd() const;
		};

		// Begin and end.
		Iter STORM_FN begin();
		Iter STORM_FN end();

	protected:
		virtual void output(wostream &to) const;

	private:
		// Char to string.
		static String toString(const Char &ch);
	};


	// Remove a fixed length of whitespace in front of each line in the string. This function either
	// removes n spaces or n tabs, not both. Think of this operation as removing one level of
	// indentation in a block of code. Empty lines are ignored.
    // TODO: Maybe this is too specific to be a public API? Whenever we have some string
	// manipulation, this can be done inside Storm itself.
	Str *STORM_FN removeIndent(Par<Str> src);

	// Add one level of indentation to the whole string 'src'.
	Str *STORM_FN indent(Par<Str> src);
	Str *STORM_FN indent(Par<Str> src, Par<Str> prepend);

	// Remove leading and trailing empty lines from a string.
	Str *STORM_FN trimBlankLines(Par<Str> src);
}
