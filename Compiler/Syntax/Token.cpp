#include "stdafx.h"
#include "Token.h"
#include "Rule.h"
#include "Decl.h"
#include "Core/StrBuf.h"
#include "Core/Join.h"

namespace storm {
	namespace syntax {

		Token::Token() : target(null), invoke(null), color(tNone),
						 raw(false), bound(false), type(0) {}

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


		RegexToken::RegexToken(Str *regex) : regex(regex) {
			type = tRegex;
		}

		void RegexToken::toS(StrBuf *to, Bool bindings) const {
			*to << L"\"" << regex << L"\"";
			Token::toS(to, bindings);
		}


		RuleToken::RuleToken(Rule *rule, Array<Str *> *params) : rule(rule), params(params) {
			color = rule->color;
			type = tRule;
		}

		void RuleToken::toS(StrBuf *to, Bool bindings) const {
			*to << rule->identifier();
			if (params) {
				*to << S("(");
				join(to, params, S(", "));
				*to << S(")");
			}
			Token::toS(to, bindings);
		}


		DelimToken::DelimToken(delim::Delimiter type, Rule *rule) : RuleToken(rule, null), type(type) {
			Token::type = tDelim;
		}

	}
}
