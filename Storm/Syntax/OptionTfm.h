#pragma once
#include "Parse.h"
#include "Token.h"
#include "Basic/BSFunction.h"

namespace storm {
	namespace syntax {

		class Rule;
		class Option;
		class OptionType;

		/**
		 * Logic for generating code that transforms a syntax tree into the user's preferred
		 * representation.
		 */

		/**
		 * Implementation of the transform function.
		 */
		class TransformFn : public bs::BSRawFn {
			STORM_CLASS;
		public:
			// Create the transform function.
			STORM_CTOR TransformFn(Par<OptionDecl> decl, Par<OptionType> owner, Par<Rule> rule, Scope scope);

			// Create body.
			virtual bs::FnBody *STORM_FN createBody();

		private:
			// Scope.
			Scope scope;

			// Result and parameters to the result (may be null).
			Auto<Name> result;
			Auto<ArrayP<Str>> resultParams;

			// The Option we're transforming.
			Option *source;

			// Parameters to rules in 'source'. These are not stored in 'source', so we need to
			// remember them ourself. May contain null, so do not return directly to storm!
			Auto<ArrayP<ArrayP<Str>>> tokenParams;

			// Keep track of all parameters we're looking for, so we can report an error if we find
			// any cycles.
			set<String> lookingFor;

			// Create the variable 'me' from the expression (if needed).
			bs::Expr *createMe(Par<bs::ExprBlock> in);

			// Find (and create if neccessary) a variable.
			bs::Expr *findVar(Par<bs::ExprBlock> in, const String &name);

			// Create a variable.
			bs::LocalVar *createVar(Par<bs::ExprBlock> in, const String &name, nat pos);

			// Create a variable by copying what is in the syntax tree (the case with RegexTokens).
			bs::LocalVar *createPlainVar(Par<bs::ExprBlock> in, const String &name, Par<Token> token);

			// Create a variable from a RuleToken.
			bs::LocalVar *createRuleVar(Par<bs::ExprBlock> in, const String &name, Par<RuleToken> token, nat pos);

			// Get 'this.
			bs::Expr *thisVar(Par<bs::ExprBlock> in);

			// Find the token storing to 'name'. Returns something larger than # of tokens on failure.
			nat findToken(const String &name);
		};

		// Create a function that transforms an option.
		Function *createTransformFn(Par<OptionDecl> option, Par<OptionType> owner, Scope scope);

	}
}
