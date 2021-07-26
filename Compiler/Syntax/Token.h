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
		class RuleToken;
		class DelimToken;
		class RegexToken;
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

			// Color of this token.
			TokenColor color;

			// Capture the raw syntax tree?
			Bool raw;

			// Was this token bound to a variable? Note that "target" is also set if we need to
			// invoke a function, even if the token was not saved to a variable.
			Bool bound;

			// Update the data in here from a TokenDecl.
			void STORM_FN update(TokenDecl *decl, MAYBE(MemberVar *) target);

			// More competent output function.
			virtual void STORM_FN toS(StrBuf *to, Bool bindings) const;

			// Regular output.
			virtual void STORM_FN toS(StrBuf *to) const;

			// Get this token as a rule.
			inline MAYBE(RuleToken *) STORM_FN asRule() {
				return (type & tRule) ? reinterpret_cast<RuleToken *>(this) : null;
			}
			inline MAYBE(DelimToken *) STORM_FN asDelim() {
				return ((type & tDelim) == tDelim) ? reinterpret_cast<DelimToken *>(this) : null;
			}
			// Get this token as a regex.
			inline MAYBE(RegexToken *) STORM_FN asRegex() {
				return (type & tRegex) ? reinterpret_cast<RegexToken *>(this) : null;
			}

		protected:
			/**
			 * What kind of token is this?
			 *
			 * We have an enum here since it is a lot faster than as<> (even though that is pretty
			 * fast), and since parsing needs to know the type of tokens _very_ often.
			 */
			enum {
				tRule = 0x01,
				tDelim = 0x03,
				tRegex = 0x04,
			};

			// Type?
			Byte type;
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
			STORM_CTOR RuleToken(Rule *rule, MAYBE(Array<Str *> *) params);

			// Rule we're matching.
			Rule *rule;

			// Parameters to this token. null means no parens were given.
			MAYBE(Array<Str *> *) params;

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
