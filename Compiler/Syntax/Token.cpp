#include "stdafx.h"
#include "Token.h"
#include "Rule.h"
#include "Decl.h"
#include "Core/StrBuf.h"

namespace storm {
	namespace syntax {

		Token::Token() : target(null), invoke(null), raw(false), bound(false), color(tNone) {}

		Token::Token(TokenDecl *decl, MAYBE(MemberVar *) target) {
			update(decl, target);
		}

		void Token::update(TokenDecl *decl, MAYBE(MemberVar *) target) {
			this->target = target;
			this->invoke = decl->invoke;
			this->raw = decl->raw;
			this->bound = decl->store != null;
		}

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

			if (color != tNone)
				*to << L" #" << name(engine(), color);
		}


		RegexToken::RegexToken(Str *regex) : regex(regex) {}

		void RegexToken::toS(StrBuf *to, Bool bindings) const {
			*to << L"\"" << regex << L"\"";
			Token::toS(to, bindings);
		}


		RuleToken::RuleToken(Rule *rule) : rule(rule) {
			color = rule->color;
		}

		void RuleToken::toS(StrBuf *to, Bool bindings) const {
			*to << rule->identifier();
			Token::toS(to, bindings);
		}


		DelimToken::DelimToken(Rule *rule) : RuleToken(rule) {}

	}
}
