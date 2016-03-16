#pragma once
#include "Shared/TObject.h"
#include "Shared/Array.h"
#include "Storm/Thread.h"
#include "Storm/Tokenizer.h"
#include "Storm/Name.h"

#include "RepType.h"

namespace storm {
	namespace syntax {
		STORM_PKG(lang.bnf);

		/**
		 * Logic for parsing syntax files.
		 * TODO? Should all these objects be forced onto the compiler thread?
		 */

		/**
		 * Representation of a rule declaration.
		 */
		class RuleDecl : public ObjectOn<Compiler> {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR RuleDecl(SrcPos pos, Par<Str> name, Par<Name> result);

			// Declared at?
			STORM_VAR SrcPos pos;

			// Name.
			STORM_VAR Auto<Str> name;

			// Result type.
			STORM_VAR Auto<Name> result;

			// Parameter types.
			STORM_VAR Auto<ArrayP<Name>> paramTypes;

			// Parameter names.
			STORM_VAR Auto<ArrayP<Str>> paramNames;

		protected:
			// Output.
			virtual void output(wostream &to) const;
		};


		/**
		 * Token in an option declaration.
		 */
		class TokenDecl : public ObjectOn<Compiler> {
			STORM_CLASS;
		public:
			STORM_CTOR TokenDecl();

			// Store this token as <store>.
			STORM_VAR MAYBE(Auto<Str>) store;

			// Use this token to invoke <invoke>.
			STORM_VAR MAYBE(Auto<Str>) invoke;

			// Capture the raw syntax tree?
			STORM_VAR Bool raw;

		protected:
			virtual void output(wostream &to) const;
		};

		/**
		 * Token matching a regexp.
		 */
		class RegexTokenDecl : public TokenDecl {
			STORM_CLASS;
		public:
			STORM_CTOR RegexTokenDecl(Par<Str> regex);

			// Regex. Note: need to call .unescape(false) before passing it to the regex parser!
			STORM_VAR Auto<Str> regex;

		protected:
			virtual void output(wostream &to) const;
		};

		/**
		 * Token matching a rule.
		 */
		class RuleTokenDecl : public TokenDecl {
			STORM_CLASS;
		public:
			STORM_CTOR RuleTokenDecl(Par<Name> rule, SrcPos pos);

			// Where?
			STORM_VAR SrcPos pos;

			// Name to match.
			STORM_VAR Auto<Name> rule;

			// Parameters to this token. null means not even parens were given.
			STORM_VAR MAYBE(Auto<ArrayP<Str>>) params;

		protected:
			virtual void output(wostream &to) const;
		};

		/**
		 * Token matching the delimiter.
		 */
		class DelimTokenDecl : public TokenDecl {
			STORM_CLASS;
		public:
			STORM_CTOR DelimTokenDecl();

		protected:
			virtual void output(wostream &to) const;
		};


		/**
		 * Representation of an option declaration.
		 */
		class OptionDecl : public ObjectOn<Compiler> {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR OptionDecl(SrcPos pos, Par<Name> memberOfRule);

			// Where was this rule declared?
			STORM_VAR SrcPos pos;

			// Which rule is this option a member of?
			STORM_VAR Auto<Name> rule;

			// Priority.
			STORM_VAR Int priority;

			// Tokens.
			STORM_VAR Auto<ArrayP<TokenDecl>> tokens;

			// Specific name of this rule?
			STORM_VAR MAYBE(Auto<Str>) name;

			// Result (if given).
			STORM_VAR MAYBE(Auto<Name>) result;

			// Parameters to result.
			STORM_VAR MAYBE(Auto<ArrayP<Str>>) resultParams;

			// Repeat.
			STORM_VAR Nat repStart;
			STORM_VAR Nat repEnd;
			STORM_VAR RepType repType;

			// Capture the repeat? Only supported if 'repType' is 'repNone'.
			STORM_VAR MAYBE(Auto<TokenDecl>) repCapture;

		protected:
			virtual void output(wostream &to) const;

			// Output the end of a repeat sequence.
			void outputRepEnd(wostream &to) const;
		};

		/**
		 * Representation of the parsed contents of a syntax file.
		 */
		class Contents : public ObjectOn<Compiler> {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR Contents();

			// Used packages.
			STORM_VAR Auto<ArrayP<Name>> use;

			// Name of the delimiter rule.
			STORM_VAR MAYBE(Auto<Name>) delimiter;

			// Rule declarations.
			STORM_VAR Auto<ArrayP<RuleDecl>> rules;

			// Options.
			STORM_VAR Auto<ArrayP<OptionDecl>> options;

		protected:
			virtual void output(wostream &to) const;
		};

		// Parse a file. Throws exception on error.
		Contents *STORM_FN parseSyntax(Par<Url> file) ON(Compiler);

	}
}
