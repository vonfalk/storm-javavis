#pragma once
#include "Core/Array.h"
#include "Core/StrBuf.h"
#include "Core/Str.h"

namespace storm {
	namespace syntax {
		STORM_PKG(lang.bnf);

		/**
		 * Regex matching. Stores pre-parsed patterns.
		 *
		 * Note: Surrogate pairs are not properly supported.
		 */
		class Regex {
			STORM_VALUE;
		public:
			// Create a matcher for the pattern.
			STORM_CTOR Regex(Str *pattern);

			// Copy.
			STORM_CAST_CTOR Regex(const Regex &o);

			// Deep copy.
			void STORM_FN deepCopy(CloneEnv *env);

			// Match the string 'str' starting from 'start'. Returns an iterator pointing past the
			// last matched character. If the pattern does not match, an iterator to the beginning
			// is returned.
			Str::Iter STORM_FN match(Str *str) const;
			Str::Iter STORM_FN match(Str *str, Str::Iter start) const;

			// Corresponding to above, but with indices. Returns 0 if no match is found.
			Nat matchRaw(Str *str) const;
			Nat matchRaw(Str *str, Nat start) const;

		private:
			// Character set.
			class Set {
				STORM_VALUE;
			public:
				// Create an empty set.
				static Set empty();

				// Create a set containing a single character.
				static Set single(wchar ch);

				// Create a set matching any character.
				static Set all();

				// Parse a regex.
				static Set parse(Engine &e, const wchar *str, nat &pos);

				// Parse a group.
				static Set parseGroup(Engine &e, const wchar *str, nat &pos);

				// Characters in this set.
				GcArray<wchar> *chars;

				// Single character to match (if any).
				Int first;

				// Inverted set?
				Bool inverted;

				// Does this set contain a specific character?
				Bool contains(wchar ch) const;

				// Number of chars in here.
				nat count() const;

				// Output.
				void output(StrBuf *to) const;
			};

			// A state in the NFA.
			class State {
				STORM_VALUE;
			public:
				// Matching characters.
				Set match;

				// Allow skipping this state?
				// Used for * and ?
				Bool skippable;

				// Allow repeating this state?
				// Used for the * and + repetitions.
				Bool repeatable;

				// Parse the next state from a regex.
				static State parse(Engine &e, const wchar *str, nat &pos);

				// Output.
				void output(StrBuf *to) const;
			};

			// All states.
			Array<State> *states;

			// Parse a string.
			void parse(Str *parse);

			// Output friends.
			friend StrBuf &operator <<(StrBuf &to, Regex r);
			friend wostream &operator <<(wostream &to, Regex r);
		};

		// Output.
		StrBuf &STORM_FN operator <<(StrBuf &to, Regex r);
		wostream &operator <<(wostream &to, Regex r);

	}
}
