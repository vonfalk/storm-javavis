#pragma once
#include "Object.h"
#include "Array.h"

namespace storm {

	STORM_PKG(core);

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
		String v;

		// From literal ctor.
		Str(const wchar *s);

		// Empty ctor
		STORM_CTOR Str();

		// Copy ctor
		STORM_CTOR Str(Par<Str> copy);

		// Create from regular string.
		Str(const String &s);

		// String length.
		Nat STORM_FN count() const;

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

	protected:
		virtual void output(wostream &to) const;
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
