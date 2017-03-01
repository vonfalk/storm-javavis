#include "stdafx.h"
#include "Regex.h"
#include "Core/CloneEnv.h"
#include "Core/PODArray.h"
#include "Compiler/Exception.h"

namespace storm {
	namespace syntax {

		static void check(Char ch) {
			if (ch.leading() != 0)
				throw InternalError(L"Surrogate characters not supported in regular expressions.");
		}

		/**
		 * Regex.
		 */

		const nat Regex::NO_MATCH = -1;

		Regex::Regex(Str *pattern) {
			states = new (pattern) Array<State>();
			parse(pattern);
		}

		Regex::Regex(const Regex &o) : states(o.states), lastMatch(o.lastMatch) {}

		void Regex::deepCopy(CloneEnv *env) {
			// Note: there is no need to deeply copy things in here, as everything is read only from
			// now on.
			cloned(states, env);
		}

		Bool Regex::matchesEmpty() const {
			for (Nat i = 0; i < states->count(); i++)
				if (!states->at(i).skippable)
					return false;
			return true;
		}

		Bool Regex::operator ==(Regex o) const {
			Nat c = states->count();
			if (c != o.states->count())
				return false;

			for (Nat i = 0; i < c; i++) {
				if (states->at(i) != o.states->at(i))
					return false;
			}

			return true;
		}

		Bool Regex::operator !=(Regex o) const {
			return !(*this == o);
		}

		Nat Regex::hash() const {
			Nat r = 5381;
			for (Nat i = 0; i < states->count(); i++)
				r = ((r << 5) + r) + states->at(i).hash();
			return r;
		}

		void Regex::parse(Str *str) {
			nat pos = 0;
			const wchar *s = str->c_str();
			while (s[pos])
				states->push(State::parse(str->engine(), s, pos));
		}

		Bool Regex::match(Str *str) {
			return match(str, str->begin());
		}

		Bool Regex::match(Str *str, Str::Iter start) {
			Nat r = matchRaw(str, start.offset());
			if (r == NO_MATCH)
				return false;
			lastMatch = str->posIter(r);
			return true;
		}

		Bool Regex::matchAll(Str *str) {
			return matchAll(str, str->begin());
		}

		Bool Regex::matchAll(Str *str, Str::Iter start) {
			Nat r = matchRaw(str, start.offset());
			if (r == NO_MATCH)
				return false;
			return str->posIter(r) == str->end();
		}

		Str::Iter Regex::matchEnd() const {
			return lastMatch;
		}

		Nat Regex::matchRaw(Str *str) const {
			return matchRaw(str, 0);
		}

		Nat Regex::matchRaw(Str *s, Nat start) const {
			// Pre-allocate this many entries for 'current' and 'next'.
			const nat prealloc = 40;
			const wchar *str = s->c_str();
			typedef PODArray<nat, prealloc> States;

			nat best = NO_MATCH;

			// Our two needed state arrays.
			States a(s->engine());
			States b(s->engine());

			// Current and next states.
			States *current = &a;
			States *next = &b;

			current->push(0);

			nat stateCount = states->count();

			// We can simply move through the source string character by character.
			// Note: we exit when there are no more states to process. Otherwise the outer
			// loop would process the entire string even if we are done matching.
			// Note: we iterate one iteration too far in this loop, so we can properly find
			// the accepting state in all cases.
			for (nat pos = start, size = start; pos <= size && current->count() > 0; pos++) {
				wchar ch = str[pos];
				if (ch)
					size = pos + 1;

				// Simulate each state...
				for (nat i = 0; i < current->count(); i++) {
					nat stateId = (*current)[i];

					// Done?
					if (stateId == stateCount) {
						best = pos;
						continue;
					}

					const State &state = states->at(stateId);

					// Skip ahead?
					if (state.skippable)
						current->push(stateId + 1);

					// Match?
					if (!state.match.contains(ch))
						continue;

					// Advance.
					next->push(stateId + 1);

					// Repeat?
					if (state.repeatable)
						next->push(stateId);
				}

#ifdef DEBUG
				static nat warned = prealloc;
				if (current->count() > warned) {
					warned = current->count();
					WARNING(L"Large state array found when matching " << *this);
					WARNING(L"Consider increasing 'prealloc' above " << current->count());
					WARNING(L"Current prealloc: " << prealloc);
				}
#endif

				// Swap next and current.
				std::swap(current, next);
				next->clear();
			}

			return best;
		}

		wostream &operator <<(wostream &to, Regex r) {
			StrBuf *s = new (r.states) StrBuf();
			*s << r;
			return to << s->toS()->c_str();
		}

		StrBuf &operator <<(StrBuf &to, Regex r) {
			for (nat i = 0; i < r.states->count(); i++) {
				r.states->at(i).output(&to);
			}
			return to;
		}


		/**
		 * Set.
		 */

		Regex::Set Regex::Set::empty() {
			Set s = {
				null,
				0,
				false,
			};
			return s;
		}

		Regex::Set Regex::Set::single(wchar ch) {
			Set s = {
				null,
				ch,
				false,
			};
			return s;
		}

		Regex::Set Regex::Set::all() {
			Set s = {
				null,
				0,
				true,
			};
			return s;
		}

		Regex::Set Regex::Set::parse(Engine &e, const wchar *str, nat &pos) {
			switch (str[pos]) {
			case '\\':
				pos++;
				if (str[pos])
					return single(str[pos++]);
				else
					return single('\\');
			case '[':
				pos++;
				return parseGroup(e, str, pos);
			case '.':
				pos++;
				return all();
			default:
				return single(str[pos++]);
			}
		}

		static void put(GcArray<wchar> *out, nat &pos, wchar ch) {
			if (out)
				out->v[pos] = ch;
			pos++;
		}

		static nat parseGroup(const wchar *str, nat &pos, GcArray<wchar> *out) {
			nat c = 0;
			wchar lastCh = 0;
			while (str[pos] != 0 && str[pos] != ']') {
				switch (str[pos]) {
				case '\\':
					pos++;
					if (str[pos]) {
						lastCh = str[pos];
						put(out, c, str[pos++]);
					} else {
						lastCh = '\\';
						put(out, c, '\\');
					}
					break;
				case '-':
					if (str[++pos]) {
						for (wchar i = lastCh + 1; i <= str[pos]; i++) {
							put(out, c, i);
						}
						lastCh = str[pos++];
					}
					break;
				default:
					lastCh = str[pos];
					put(out, c, str[pos++]);
					break;
				}
			}

			pos++;
			return c;
		}

		static const GcType gcType = {
			GcType::tArray,
			null,
			null,
			sizeof(wchar), // element size
			0,
			{},
		};

		Regex::Set Regex::Set::parseGroup(Engine &e, const wchar *str, nat &pos) {
			Set r = empty();
			if (str[pos] == '^') {
				r.inverted = true;
				pos++;
			}

			nat tmp = pos;
			nat count = storm::syntax::parseGroup(str, tmp, null);

			r.chars = runtime::allocArray<wchar>(e, &gcType, count);
			storm::syntax::parseGroup(str, pos, r.chars);

			return r;
		}

		nat Regex::Set::count() const {
			if (chars)
				return chars->count;
			else if (first != 0)
				return 1;
			else
				return 0;
		}

		Bool Regex::Set::contains(wchar ch) const {
			if (ch == wchar(first)) {
				return !inverted;
			} else if (chars) {
				for (nat i = 0; i < chars->count; i++) {
					if (ch == chars->v[i])
						return !inverted;
				}
			}
			return inverted;
		}

		static void escape(StrBuf *to, wchar ch) {
			switch (ch) {
			case '\n':
				*to << L"\\n";
				break;
			case '\t':
				*to << L"\\t";
				break;
			case '\r':
				*to << L"\\r";
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
				*to << L"\\";
				// fall thru
			default:
				to->addRaw(ch);
				break;
			}
		}

		void Regex::Set::output(StrBuf *to) const {
			if (count() == 0) {
				if (inverted)
					*to << L".";
				else
					*to << L"[^.]";
				return;
			}

			if (chars == null) {
				if (inverted) {
					*to << L"[^";
					escape(to, first);
					*to << L"]";
				} else {
					escape(to, first);
				}
				return;
			}

			*to << L"[";
			if (inverted)
				*to << L"^";
			bool inRep = false;
			wchar last = 0;
			for (nat i = 0; i < chars->count; i++) {
				if (!inRep) {
					if (chars->v[i] == last + 1) {
						inRep = true;
						*to << L"-";
					} else {
						escape(to, chars->v[i]);
					}
				} else if (chars->v[i] != last + 1) {
					escape(to, last);
					escape(to, chars->v[i]);
					inRep = false;
				}
				last = chars->v[i];
			}

			if (inRep)
				escape(to, last);
			*to << L"]";
		}

		Bool Regex::Set::operator ==(const Set &o) const {
			Nat c = count();
			if (c != o.count())
				return false;

			if (inverted != o.inverted)
				return false;
			if (first != o.first)
				return false;

			for (Nat i = 0; i < c - 1; i++) {
				if (chars->v[i] != o.chars->v[i])
					return false;
			}

			return true;
		}

		Bool Regex::Set::operator !=(const Set &o) const {
			return !(*this == o);
		}

		Nat Regex::Set::hash() const {
			Nat r = 5381;
			r = ((r << 5) + r) + first;
			if (chars) {
				for (Nat i = 0; i < chars->count; i++)
					r = ((r << 5) + r) + chars->v[i];
			}
			return r;
		}


		/**
		 * State.
		 */

		Regex::State Regex::State::parse(Engine &e, const wchar *str, nat &pos) {
			State s = {
				Set::parse(e, str, pos),
				false,
				false
			};
			switch (str[pos]) {
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
			return s;
		}

		void Regex::State::output(StrBuf *to) const {
			match.output(to);
			if (skippable && repeatable)
				*to << L"*";
			else if (skippable)
				*to << L"?";
			else if (repeatable)
				*to << L"+";
		}

		Bool Regex::State::operator ==(const State &o) const {
			return skippable == o.skippable
				&& repeatable == o.repeatable
				&& match == o.match;
		}

		Bool Regex::State::operator !=(const State &o) const {
			return !(*this == o);
		}

		Nat Regex::State::hash() const {
			Nat z = skippable;
			z |= Nat(repeatable) << 1;
			return match.hash() ^ z;
		}

	}
}
