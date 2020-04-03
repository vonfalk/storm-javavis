#pragma once
#include "Compiler/NamedThread.h"
#include "Compiler/Variable.h"
#include "Regex.h"
#include "TokenColor.h"
#include "Delimiters.h"

namespace storm {
	namespace syntax {
		STORM_PKG(lang.bnf);

		class Rule;
		class TokenDecl;

		/**
		 * Description of a token in the syntax. A token is either a regex matching some text or a
		 * reference to another rule. There is also a special case for the delimiting token, so that
		 * we can output the rules more nicely formatted.
		 */
		class Token : public ObjectOn<Compiler> {
			STORM_CLASS;
		public:
			// Create.
			Token();

			// Create.
			STORM_CTOR Token(TokenDecl *decl, MAYBE(MemberVar *) target);

			// If this token is captured, where do we store it?
			MAYBE(MemberVar *) target;

			// If this token is to be sent to a member, which member?
			MAYBE(Str *) invoke;

			// Capture the raw syntax tree?
			Bool raw;

			// Was this token bound to a variable?
			Bool bound;

			// Color of this token.
			TokenColor color;

			// Update the data in here from a TokenDecl.
			void STORM_FN update(TokenDecl *decl, MAYBE(MemberVar *) target);

			// More competent output function.
			virtual void STORM_FN toS(StrBuf *to, Bool bindings) const;

			// Regular output.
			virtual void STORM_FN toS(StrBuf *to) const;
		};


		/**
		 * Token matching a regex.
		 */
		class RegexToken : public Token {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR RegexToken(Str *regex);

			// Regex to match.
			Regex regex;

			// Output.
			virtual void STORM_FN toS(StrBuf *to, Bool bindings) const;
		};


		/**
		 * A token matching a rule.
		 */
		class RuleToken : public Token {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR RuleToken(Rule *rule);

			// Rule we're matching.
			Rule *rule;

			// Output.
			virtual void STORM_FN toS(StrBuf *to, Bool bindings) const;
		};


		/**
		 * A token matching the delimiter rule. Only here to allow for nice output of the syntax.
		 */
		class DelimToken : public RuleToken {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR DelimToken(delim::Delimiter type, Rule *rule);

			// Type of delimiter.
			delim::Delimiter type;
		};

	}
}
