#include "stdafx.h"
#include "Token.h"
#include "Rule.h"

namespace storm {
	namespace syntax {

		Token::Token() : target(null) {}

		void Token::output(wostream &to) const {
			// Output where to store the data.
			if (target) {
				// TODO: different syntax depending on how we use the data later on?
				to << ' ' << target->name;
			}
		}

		/**
		 * Regex.
		 */

		RegexToken::RegexToken(Par<Str> regex) : regex(regex->v.unescape(true)) {}

		void RegexToken::output(wostream &to) const {
			to << '"' << regex << '"';
			Token::output(to);
		}

		/**
		 * Rule.
		 */

		RuleToken::RuleToken(Par<Rule> rule) : rule(rule) {}

		void RuleToken::output(wostream &to) const {
			to << rule->identifier();
			Token::output(to);
		}


		/**
		 * Delimiter.
		 */

		DelimToken::DelimToken(Par<Rule> rule) : RuleToken(rule) {}

	}
}
