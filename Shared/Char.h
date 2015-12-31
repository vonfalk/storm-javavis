#pragma once
#include "EnginePtr.h"

namespace storm {

	STORM_PKG(core);

	/**
	 * This class represents a single character as defined by the string class. Currently one
	 * character is equal to one unicode code point, but this will probably change to mean one code
	 * point following by zero or more combining characters.
	 */
	class Char {
		STORM_VALUE;
	public:
		// Create a character from a code-point.
		STORM_CTOR Char(Int codepoint);

		// Compare.
		Bool STORM_FN operator ==(const Char &o) const;
		Bool STORM_FN operator !=(const Char &o) const;

		// Hash.
		Nat STORM_FN hash() const;

		// Get the value.
		inline Nat getCodePoint() const { return value; }

	private:
		friend class Str;
		friend class StrBuf;
		friend wostream &operator <<(wostream &to, const Char &ch);

		Nat value;
	};

	// To string.
	Str *STORM_ENGINE_FN toS(EnginePtr e, Char ch);
	wostream &operator <<(wostream &to, const Char &ch);

}
