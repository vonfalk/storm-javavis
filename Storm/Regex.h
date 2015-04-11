#pragma once

namespace storm {

	/**
	 * Regex matching. Matches 'str' from 'start', returns index of the first
	 * character that does not match 'pattern', or the length of the string.
	 * Returns NO_MATCH if the pattern does not match.
	 */
	nat matchRegex(const String &pattern, const String &str, nat start = 0);

	// Returned when there is no match at all.
	extern const nat NO_MATCH;

	/**
	 * Regex matcher class, stores 'compiled' patterns.
	 * KNOWN BUGS:
	 * * We do not handle escaped characters in ranges (like this: [\--\+])
	 *
	 * Currently implemented as a NFA (nondeterministic finite automata) state machine.
	 */
	class Regex : public Printable {
	public:
        // Create a matcher for the pattern.
		Regex(const String &pattern);

		// Match the string 'str' starting from 'start'. Returning the last character
		// matched by the pattern, or the length of the string. Returns NO_MATCH if the
		// pattern does not match.
		nat match(const String &str, nat start = 0) const;

	protected:
		virtual void output(std::wostream &to) const;

	private:
		// Character set.
		struct Set {
			// Create an empty set.
			static Set empty();

			// Create set containing a single character.
			static Set single(wchar ch);

			// Create a set matching any character.
			static Set all();

			// Create a set from a regex string at 'pos'.
			static Set parse(const String &in, nat &pos);

			// Create a set from a regex string, assuming we're in a group syntax.
			static Set parseGroup(const String &in, nat &pos);

			// Characters in this set.
			vector<wchar> chars;

			// Inverted set?
			bool inverted;

			// Does this set contain a specific character?
			bool contains(wchar ch) const;

			// Output.
			void output(std::wostream &to) const;
		};

		// State in the NFA.
		struct State {
			// Matching characters.
			Set match;

			// Allow skipping this state?
			// Used for the * and ? repetitions.
			bool skippable;

			// Allow repeating this state?
			// Used for the * and + repetitions.
			bool repeatable;

			// Parse from a regex.
			static State parse(const String &in, nat &pos);

			// Output.
			void output(std::wostream &to) const;
		};

		// States.
		vector<State> states;

		// Parse a pattern string.
		void parse(const String &pattern);

	};

}
