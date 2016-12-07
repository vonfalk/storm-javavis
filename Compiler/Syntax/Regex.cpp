#include "stdafx.h"
#include "Regex.h"
#include "Core/CloneEnv.h"
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

		Regex::Regex(Str *pattern) {
			states = new (pattern) Array<State>();
			parse(pattern);
		}

		Regex::Regex(const Regex &o) : states(o.states) {}

		void Regex::deepCopy(CloneEnv *env) {
			cloned(states, env);
		}

		void Regex::parse(Str *str) {
			nat pos = 0;
			const wchar *s = str->c_str();
			while (s[pos])
				states->push(State::parse(str->engine(), s, pos));
		}

		Str::Iter Regex::match(Str *str) const {
			return match(str, str->begin());
		}

		Str::Iter Regex::match(Str *str, Str::Iter start) const {
			return str->posIter(matchRaw(str, start.offset()));
		}

		Nat Regex::matchRaw(Str *str) const {
			return matchRaw(str, 0);
		}

		Nat Regex::matchRaw(Str *str, Nat start) const {
			return 0;
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
					return single(str[++pos]);
				else
					return single('\\');
			case '[':
				pos++;
				return parseGroup(e, str, pos);
			case '.':
				++pos;
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
					if (str[++pos]) {
						lastCh = str[pos];
						put(out, c, str[pos++]);
					} else {
						lastCh = '\\';
						put(out, c, '\\');
					}
					break;
				case '-':
					if (str[++pos]) {
						for (wchar i = lastCh; i <= str[pos]; i++) {
							put(out, c, i);
						}
						lastCh = str[pos];
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

	}
}
