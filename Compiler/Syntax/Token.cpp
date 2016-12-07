#include "stdafx.h"
#include "Token.h"
#include "Rule.h"
#include "Core/StrBuf.h"

namespace storm {
	namespace syntax {

		Token::Token() : target(null), invoke(null), raw(false) {}

		void Token::toS(StrBuf *to) const {
			toS(to, true);
		}

		void Token::toS(StrBuf *to, Bool bindings) const {
			if (!bindings)
				return;

			if (target) {
				if (raw)
					*to << L"@";

				if (invoke) {
					*to << L" -> " << invoke;
				} else {
					*to << L" " << target->name;
				}
			}
		}


		RegexToken::RegexToken(Str *regex) {
			TODO(L"Set the regex!");
		}

		void RegexToken::toS(StrBuf *to, Bool bindings) const {
			*to << L"\"" << L"regex" << L"\"";
			Token::toS(to, bindings);
		}


		RuleToken::RuleToken(Rule *rule) : rule(rule) {}

		void RuleToken::toS(StrBuf *to, Bool bindings) const {
			*to << rule->identifier();
			Token::toS(to, bindings);
		}


		DelimToken::DelimToken(Rule *rule) : RuleToken(rule) {}

	}
}
