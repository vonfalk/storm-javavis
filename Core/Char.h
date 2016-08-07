#pragma once
#include "EnginePtr.h"

namespace storm {
	STORM_PKG(core);

	/**
	 * Class representing a single character as defined by the Str class. Currently one character is
	 * equal to one unicode code point.
	 */
	class Char {
		STORM_VALUE;
	public:
		// Nice to have from C++.
		Char(char ch);
		Char(wchar ch);

		// Create a character from a codepoint number.
		STORM_CTOR Char(Nat codepoint);

		// Compare.
		Bool STORM_FN operator ==(Char o) const;
		Bool STORM_FN operator !=(Char o) const;

		// Hash.
		Nat STORM_FN hash() const;

		// Deep copy.
		void STORM_FN deepCopy(CloneEnv *env);

		/**
		 * C++ interface. May change if we change encoding.
		 */

		// Leading/trailing surrogate pair. Leading may be 0, which means we fit into one wchar.
		wchar leading() const;
		wchar trailing() const;

		// # of codepoints in use.
		nat size() const;

	private:
		Nat value;
	};

	// Output.
	Str *STORM_FN toS(EnginePtr e, Char ch);
	wostream &operator <<(wostream &to, const Char &ch);

}