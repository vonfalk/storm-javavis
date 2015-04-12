#include "stdafx.h"
#include "Regex.h"
#include "Utils/PreArray.h"

namespace storm {

	const nat NO_MATCH = -1;

	nat matchRegex(const String &pattern, const String &str, nat start) {
		Regex p(pattern);
		nat r = p.match(str, start);
		// PLN(p << " with " << str << " -> " << r);
		return r;
	}

	/**
	 * Character set class.
	 */

	Regex::Set Regex::Set::empty() {
		Set s = { vector<wchar>(), false };
		return s;
	}

	Regex::Set Regex::Set::single(wchar ch) {
		Set s = { vector<wchar>(1, ch), false };
		return s;
	}

	Regex::Set Regex::Set::all() {
		Set s = { vector<wchar>(), true };
		return s;
	}

	bool Regex::Set::contains(wchar ch) const {
		for (nat i = 0; i < chars.size(); i++) {
			if (chars[i] == ch) {
				return !inverted;
			}
		}

		return inverted;
	}

	static void escape(std::wostream &to, wchar ch) {
		switch (ch) {
		case '\n':
			to << "\\n";
			break;
		case '\t':
			to << "\\t";
			break;
		case '\r':
			to << "\\r";
			break;
		case '.':
		case '[':
		case ']':
		case '*':
		case '?':
		case '+':
		case '^':
		case '"':
		case '\\':
			to << "\\";
			// fall thru
		default:
			to << ch;
			break;
		}
	}

	void Regex::Set::output(std::wostream &to) const {
		if (chars.size() == 0) {
			if (inverted)
				to << L".";
			else
				to << L"[^.]";
			return;
		}

		if (!inverted && chars.size() == 1) {
			escape(to, chars[0]);
			return;
		}

		to << '[';
		if (inverted)
			to << '^';

		bool inRep = false;
		wchar last = 0;
		for (nat i = 0; i < chars.size(); i++) {
			if (!inRep) {
				if (chars[i] == last + 1) {
					inRep = true;
					to << L"-";
				} else {
					escape(to, chars[i]);
				}
			} else if (chars[i] != last + 1) {
				escape(to, last);
				escape(to, chars[i]);
				inRep = false;
			}
			last = chars[i];
		}

		if (inRep)
			escape(to, last);

		to << ']';
	}

	Regex::Set Regex::Set::parse(const String &in, nat &pos) {
		switch (in[pos]) {
		case '\\':
			if (++pos < in.size())
				return single(in[pos++]);
			else
				return single('\\');
		case '[':
			pos++;
			return parseGroup(in, pos);
		case '.':
			pos++;
			return all();
		default:
			return single(in[pos++]);
		}
	}

	Regex::Set Regex::Set::parseGroup(const String &in, nat &pos) {
		Set result = Set::empty();
		if (in[pos] == '^') {
			result.inverted = true;
			pos++;
		}

		while (pos < in.size() && in[pos] != ']') {
			switch (in[pos]) {
			case '\\':
				if (++pos < in.size())
					result.chars.push_back(in[pos++]);
				break;
			case '-':
				if (++pos < in.size() && result.chars.size() > 0) {
					for (wchar c = result.chars.back() + 1; c < in[pos]; c++)
						result.chars.push_back(c);
					result.chars.push_back(in[pos++]);
				}
				break;
			default:
				result.chars.push_back(in[pos++]);
				break;
			}
		}

		if (pos < in.size())
			pos++;

		return result;
	}

	/**
	 * State class.
	 */

	void Regex::State::output(std::wostream &to) const {
		match.output(to);
		if (skippable && repeatable)
			to << '*';
		else if (skippable)
			to << '?';
		else if (repeatable)
			to << '+';
	}

	Regex::State Regex::State::parse(const String &in, nat &pos) {
		State s = { Set::parse(in, pos), false, false };
		if (pos < in.size()) {
			switch (in[pos]) {
			case '*':
				s.skippable = true;
				s.repeatable = true;
				pos++;
				break;
			case '?':
				s.skippable = true;
				pos++;
				break;
			case '+':
				s.repeatable = true;
				pos++;
				break;
			}
		}
		return s;
	}

	/**
	 * Regex class.
	 */

	Regex::Regex(const String &pattern) {
		parse(pattern);
	}

	void Regex::output(std::wostream &to) const {
		for (nat i = 0; i < states.size(); i++) {
			states[i].output(to);
		}
	}

	void Regex::parse(const String &pattern) {
		nat pos = 0;
		while (pos < pattern.size()) {
			states.push_back(State::parse(pattern, pos));
		}
	}

	static void update(nat &best, nat candidate) {
		if (best == NO_MATCH)
			best = candidate;
		else if (candidate != NO_MATCH)
			best = max(best, candidate);
	}

	nat Regex::match(const String &str, nat start) const {
		// Pre-allocate this much memory for 'current' and 'next'.
		const nat prealloc = 40;

		nat best = NO_MATCH;

		// All current states.
		PreArray<nat, prealloc> current;
		current.push(0);

		// We can simply move through the source string character by character.
		// Note: we exit when there are no more states to process. Otherwise the outer
		// loop would process the entire string even though we are done with our matching.
		for (nat pos = start; pos <= str.size() && current.count() > 0; pos++) {
			PreArray<nat, prealloc> next;
			wchar ch = 0;
			if (pos < str.size())
				ch = str[pos];

			// Simulate each state...
			for (nat i = 0; i < current.count(); i++) {
				nat stateId = current[i];

				// Done?
				if (stateId == states.size()) {
					update(best, pos);
					continue;
				}

				// Skip ahead?
				const State &state = states[stateId];
				if (state.skippable)
					current.push(stateId + 1);

				// Match?
				if (!state.match.contains(ch))
					continue;

				next.push(stateId + 1);

				// Repeat?
				if (state.repeatable)
					next.push(stateId);
			}

#ifdef DEBUG
			// Warn when we are above the limit in debug mode.
			static nat warned = prealloc;
			if (next.count() > warned) {
				warned = next.count();
				WARNING(L"Large state array found when matching " << *this);
				WARNING(L"Consider increasing 'prealloc' above " << next.count());
				WARNING(L"Current prealloc: " << prealloc);
			}
#endif

			// New states!
			current = next;
		}

		return best;
	}

}
