#pragma once

namespace storm {

	/**
	 * Regex matching. Matches 'str' from 'start', returns index of the first
	 * character that does not match 'pattern', or the length of the string.
	 */
	nat matchRegex(const String &pattern, const String &str, nat start = 0);


	/**
	 * Regex matcher class, stores 'compiled' patterns.
	 */
	class Regex : public Printable {
	public:
        // Create a matcher for the pattern.
		Regex(const String &pattern);

		// Match the string 'str' starting from 'start'. Returning the last character
		// matched by the pattern, or the length of the string.
		nat match(const String &str, nat start = 0) const;

	protected:
		virtual void output(std::wostream &to) const;

	private:
		// Character set.
		struct Set {
			// Create an empty set.
			Set();

			// Create set containing a single character.
			Set(wchar ch);

			// Create a set matching any character.
			static Set all();

			// Characters in this set.
			vector<wchar> chars;

			// Inverted set?
			bool inverted;

			// Does this set contain a specific character?
			bool contains(wchar ch) const;

			// ToString
			String toS() const;
		};

		// Repeat type.
		enum Repeat {
			rOnce,
			rZeroPlus,
			rOnePlus,
			rZeroOne,
		};

		// Set of character sets to be matched.
		vector<Set> chars;

		// Repeat of each character set.
		vector<Repeat> repeat;

		// Parse a pattern string.
		void parse(const String &pattern);

		// Parse a character group. Returns the size of the group. The group is added to 'chars'.
		nat parseGroup(const String &pattern, nat start);

		// Match a string. 'firstTry' is true if this is the first time 'patternPos' is matched.
		nat match(nat patternPos, const String &str, nat pos, bool firstTry) const;
	};

}
