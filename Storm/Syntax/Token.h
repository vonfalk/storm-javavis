#pragma once
#include "Shared/TObject.h"
#include "Regex.h"
#include "Thread.h"
#include "TypeVar.h"

namespace storm {
	namespace syntax {

		class Rule;

		/**
		 * Description of a token in the syntax. A token is either a regex matching som text, or a
		 * reference to another rule. There is also a special case for the delimiting token, so that
		 * we can output the rules more nicely.
		 */
		class Token : public ObjectOn<Compiler> {
			STORM_CLASS;
		public:
			// Create.
			Token();

			// If this token is captured, where to store it? (borrowed ptr).
			TypeVar *target;

		protected:
			virtual void output(wostream &to) const;
		};


		/**
		 * A token matching a regex.
		 */
		class RegexToken : public Token {
			STORM_CLASS;
		public:
			// Create. Handles escape sequences inside the 'regex' string.
			STORM_CTOR RegexToken(Par<Str> regex);

			// Regex to match.
			Regex regex;

		protected:
			virtual void output(wostream &to) const;
		};

		/**
		 * A token matching a rule.
		 */
		class RuleToken : public Token {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR RuleToken(Par<Rule> rule);

			// The rule we should match.
			Auto<Rule> rule;

		protected:
			virtual void output(wostream &to) const;
		};

		/**
		 * A token matching the delimiter rule. Only here to allow for nicer output of the syntax.
		 */
		class DelimToken : public RuleToken {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR DelimToken(Par<Rule> rule);
		};

	}
}
