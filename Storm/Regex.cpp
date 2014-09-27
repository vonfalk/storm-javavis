#include "stdafx.h"
#include "Regex.h"

namespace storm {

	nat matchRegex(const String &pattern, const String &str, nat start) {
		PLN(Regex(pattern));
		return Regex(pattern).match(str, start);
	}

	/**
	 * Character set class.
	 */

	Regex::Set::Set() : chars(), inverted(false) {}

	Regex::Set::Set(wchar ch) : chars(1, ch), inverted(false) {}

	Regex::Set Regex::Set::all() {
		Set s;
		s.inverted = true;
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

	String Regex::Set::toS() const {
		if (chars.size() == 0)
			if (inverted)
				return L".";
			else
				return L"[^.]";

		std::wostringstream t;
		t << '[';
		if (inverted)
			t << '^';
		for (nat i = 0; i < chars.size(); i++)
			t << chars[i];
		t << ']';

		return t.str();
	}

	/**
	 * Regex class.
	 */

	Regex::Regex(const String &pattern) {
		parse(pattern);
	}

	void Regex::output(std::wostream &to) const {
		to << "Regex: ";
		for (nat i = 0; i < chars.size(); i++) {
			to << chars[i].toS();

			switch (repeat[i]) {
			case rZeroPlus:
				to << '*';
				break;
			case rOnePlus:
				to << '+';
				break;
			case rZeroOne:
				to << '?';
				break;
			}
		}
	}

	void Regex::parse(const String &pattern) {
		for (nat pos = 0; pos < pattern.size(); pos++) {
			switch (pattern[pos]) {
			case '\\':
				if (++pos < pattern.size()) {
					chars.push_back(Set(pattern[pos]));
					repeat.push_back(rOnce);
				}
				break;
			case '[':
				pos += parseGroup(pattern, pos + 1);
				break;
			case '.':
				chars.push_back(Set::all());
				repeat.push_back(rOnce);
				break;
			case '*':
				if (repeat.size() > 0)
					repeat.back() = rZeroPlus;
				break;
			case '+':
				if (repeat.size() > 0)
					repeat.back() = rOnePlus;
				break;
			case '?':
				if (repeat.size() > 0)
					repeat.back() = rZeroOne;
				break;
			default:
				chars.push_back(Set(pattern[pos]));
				repeat.push_back(rOnce);
				break;
			}
		}
	}

	nat Regex::parseGroup(const String &pattern, nat start) {
		Set s;
		nat pos = start;

		if (start < pattern.size() && pattern[start] == '^') {
			s.inverted = true;
			pos++;
		}

		for (; pos < pattern.size(); pos++) {
			switch (pattern[pos]) {
			case '\\':
				if (++pos < pattern.size())
					s.chars.push_back(pattern[pos]);
				break;
			case ']':
				pos++;
				goto done;
			default:
				s.chars.push_back(pattern[pos]);
			}
		}

	done:
		chars.push_back(s);
		repeat.push_back(rOnce);
		return pos - start;
	}

	nat Regex::match(const String &str, nat start) const {
		nat r = match(0, str, start, true);
		return max(r, start);
	}

	nat Regex::match(nat patternPos, const String &str, nat pos, bool firstTry) const {
		if (patternPos == chars.size())
			return pos;

		const Repeat &r = repeat[patternPos];
		const Set &c = chars[patternPos];
		nat res;

		switch (r) {
		case rOnce:
			if (pos >= str.size())
				return 0;

			if (!c.contains(str[pos]))
				return 0;
			return match(patternPos + 1, str, pos + 1, true);
		case rZeroPlus:
			if (pos >= str.size() || !c.contains(str[pos])) {
				return match(patternPos + 1, str, pos, true);
			} else {
				res = match(patternPos, str, pos + 1, false);
				return max(match(patternPos + 1, str, pos + 1, true), res);
			}
		case rOnePlus:
			if (pos >= str.size() || !c.contains(str[pos])) {
				if (firstTry)
					return 0;
				else
					return match(patternPos + 1, str, pos, true);
			} else {
				res = match(patternPos, str, pos + 1, false);
				return max(match(patternPos + 1, str, pos + 1, true), res);
			}
		case rZeroOne:
			if (pos >= str.size() || !c.contains(str[pos])) {
				return match(patternPos + 1, str, pos, true);
			} else {
				res = match(patternPos, str, pos + 1, false);
				return max(match(patternPos + 1, str, pos + 1, true), res);
			}
		}

		return 0;
	}
}
