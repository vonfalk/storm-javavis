#pragma once
#include "Core/StrBuf.h"
#include "Compiler/Name.h"
#include "RepType.h"

namespace storm {
	namespace syntax {
		STORM_PKG(lang.bnf);

		/**
		 * Logic for parsing syntax files.
		 */

		/**
		 * Parameter declaration.
		 */
		class ParamDecl {
			STORM_VALUE;
		public:
			STORM_CTOR ParamDecl(Name *type, Str *name);

			Name *type;
			Str *name;
		};

		StrBuf &STORM_FN operator <<(StrBuf &to, ParamDecl decl);

		/**
		 * Token in a production declaration.
		 */
		class TokenDecl : public Object {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR TokenDecl();

			// Store this token as 'store'.
			MAYBE(Str *) store;

			// Use this token to invoke 'invoke'.
			MAYBE(Str *) invoke;

			// Capture the raw syntax tree?
			Bool raw;

			// Deep copy.
			virtual void STORM_FN deepCopy(CloneEnv *env);

			// Output.
			virtual void STORM_FN toS(StrBuf *to) const;
		};


		/**
		 * Regex token declaration.
		 */
		class RegexTokenDecl : public TokenDecl {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR RegexTokenDecl(Str *regex);

			// Regex.
			Str *regex;

			// Deep copy.
			virtual void STORM_FN deepCopy(CloneEnv *env);

			// Output.
			virtual void STORM_FN toS(StrBuf *to) const;
		};


		/**
		 * Token matching a rule.
		 */
		class RuleTokenDecl : public TokenDecl {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR RuleTokenDecl(SrcPos pos, Name *rule);

			// Where?
			SrcPos pos;

			// Name to match.
			Name *rule;

			// Parameters to this token. null means no parens were given.
			MAYBE(Array<Str *> *) params;

			// Deep copy.
			virtual void STORM_FN deepCopy(CloneEnv *env);

			// Output.
			virtual void STORM_FN toS(StrBuf *to) const;
		};


		/**
		 * Token matching a delimiter.
		 */
		class DelimTokenDecl : public TokenDecl {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR DelimTokenDecl();

			// Output.
			virtual void STORM_FN toS(StrBuf *to) const;
		};


		/**
		 * Representation of a declared production.
		 */
		class ProductionDecl : public Object {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR ProductionDecl(SrcPos pos, Name *memberOf);

			// Where was this production declared?
			SrcPos pos;

			// Which rule are we a member of?
			Name *rule;

			// Priority.
			Int priority;

			// Tokens.
			Array<TokenDecl *> *tokens;

			// Any specific name of this rule?
			MAYBE(Str *) name;

			// Result (if given).
			MAYBE(Name *) result;

			// Parameters to the result. null = no parens present.
			MAYBE(Array<Str *> *) resultParams;

			// Repetition.
			Nat repStart;
			Nat repEnd;
			RepType repType;

			// Capture the repeat? Only supported if 'repType' is 'repNone'.
			MAYBE(TokenDecl *) repCapture;

			// Deep copy.
			virtual void STORM_FN deepCopy(CloneEnv *env);

			// Output.
			virtual void STORM_FN toS(StrBuf *to) const;

		private:
			// Output the end of a repetition.
			void outputRepEnd(StrBuf *to) const;
		};


		/**
		 * Rule declaration.
		 */
		class RuleDecl : public Object {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR RuleDecl(SrcPos pos, Str *name, Name *result);

			// Declared at?
			SrcPos pos;

			// Name.
			Str *name;

			// Result type.
			Name *result;

			// Parameters.
			Array<ParamDecl> *params;

			// Deep copy.
			virtual void STORM_FN deepCopy(CloneEnv *env);

			// Output.
			virtual void STORM_FN toS(StrBuf *to) const;
		};

		/**
		 * Contents of an entire file.
		 */
		class FileContents : public Object {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR FileContents();

			// Used packages.
			Array<SrcName *> *use;

			// Name of the delimiter rule (if any).
			MAYBE(SrcName *) delimiter;

			// Rule declarations.
			Array<RuleDecl *> *rules;

			// Productions.
			Array<ProductionDecl *> *productions;

			// Deep copy.
			virtual void STORM_FN deepCopy(CloneEnv *env);

			// Output.
			virtual void STORM_FN toS(StrBuf *to) const;
		};

	}
}
