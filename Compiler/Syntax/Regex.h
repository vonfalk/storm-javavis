#pragma once
#include "Core/Array.h"
#include "Core/StrBuf.h"
#include "Core/Str.h"
#include "Core/Exception.h"

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

			// Deep copy.
			void STORM_FN deepCopy(CloneEnv *env);

			// Does this regex match the empty string?
			Bool STORM_FN matchesEmpty() const;

			// Match the string 'str' starting from 'start' (if present). Returns true if a match
			// was possible at all. The matched part of the string can be retrieved using
			// 'matchEnd'.
			// TODO: Return MAYBE(Str::Iter) if that is ever supported!
			Bool STORM_FN match(Str *str);
			Bool STORM_FN match(Str *str, Str::Iter start);

			// Match the entire string 'str' starting from 'start'.
			Bool STORM_FN matchAll(Str *str);
			Bool STORM_FN matchAll(Str *str, Str::Iter start);

			// Get the result of the last match.
			Str::Iter STORM_FN matchEnd() const;

			// Is this a 'simple' regex? Ie. a regex that is basically a string literal?
			Bool STORM_FN simple() const;

			/**
			 * C++ api used by the parser.
			 */

			static const nat NO_MATCH;

			// Corresponding to above, but with indices. Returns NO_MATCH if no match is found.
			Nat matchRaw(Str *str) const;
			Nat matchRaw(Str *str, Nat start) const;

			// Same regex as another one?
			Bool STORM_FN operator ==(Regex o) const;
			Bool STORM_FN operator !=(Regex o) const;

			// Hash this regex.
			Nat STORM_FN hash() const;

		private:
			// Character set.
			class Set {
				STORM_VALUE;
			public:
				// Create.
				Set(GcArray<wchar> *chars, Int first, Bool inverted);

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

				// Compare.
				Bool operator ==(const Set &o) const;
				Bool operator !=(const Set &o) const;

				// Hash.
				Nat hash() const;
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

				// Simple?
				Bool simple() const;

				// Compare.
				Bool operator ==(const State &o) const;
				Bool operator !=(const State &o) const;

				// Hash.
				Nat hash() const;
			};

			// All states.
			Array<State> *states;

			// Last match.
			Str::Iter lastMatch;

			// Parse a string.
			void parse(Str *parse);

			// Output friends.
			friend StrBuf &operator <<(StrBuf &to, Regex r);
			friend wostream &operator <<(wostream &to, Regex r);
		};

		// Output.
		StrBuf &STORM_FN operator <<(StrBuf &to, Regex r);
		wostream &operator <<(wostream &to, Regex r);

		/**
		 * Regex exception.
		 */
		class EXCEPTION_EXPORT RegexError : public Exception {
			STORM_EXCEPTION;
		public:
			RegexError(const wchar *regex, const wchar *message);
			STORM_CTOR RegexError(Str *regex, Str *message);
			virtual void STORM_FN message(StrBuf *to) const;
		private:
			Str *regex;
			Str *msg;
		};

	}
}
