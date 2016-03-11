#include "stdafx.h"
#include "Token.h"
#include "Rule.h"

namespace storm {
	namespace syntax {

		Token::Token() : target(null) {}

		void Token::output(wostream &to) const {
			output(to, true);
		}

		void Token::output(wostream &to, bool bindings) const {
			if (!bindings)
				return;

			// Output where to store the data.
			if (target) {
				if (invoke) {
					to << L" -> " << invoke;
				} else {
					to << ' ' << target->name;
				}
			}
		}

		/**
		 * Regex.
		 */

		RegexToken::RegexToken(Par<Str> regex) : regex(regex->v.unescape(true)) {}

		void RegexToken::output(wostream &to, bool bindings) const {
			to << '"' << regex << '"';
			Token::output(to, bindings);
		}

		/**
		 * Rule.
		 */

		RuleToken::RuleToken(Par<Rule> rule) : rule(rule) {}

		void RuleToken::output(wostream &to, bool bindings) const {
			to << rule->identifier();
			Token::output(to, bindings);
		}


		/**
		 * Delimiter.
		 */

		DelimToken::DelimToken(Par<Rule> rule) : RuleToken(rule) {}

	}
}
