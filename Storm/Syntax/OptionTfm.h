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


			/**
			 * Create variables.
			 */

			// Create the variable 'me' from the expression (if needed).
			bs::Expr *createMe(Par<bs::ExprBlock> in);

			// Find a variable. Throws exception if it is not yet created.
			bs::Expr *readVar(Par<bs::Block> in, const String &name);

			// Find (and create if neccessary) a variable.
			bs::Expr *findVar(Par<bs::ExprBlock> in, const String &name);

			// Create a variable.
			bs::LocalVar *createVar(Par<bs::ExprBlock> in, const String &name, nat pos);

			// Create a variable by copying what is in the syntax tree (the case with RegexTokens).
			bs::LocalVar *createPlainVar(Par<bs::ExprBlock> in, const String &name, Par<Token> token);

			// Create a variable from by transforming the original syntax tree.
			bs::LocalVar *createTfmVar(Par<bs::ExprBlock> in, const String &name, Par<Token> token, nat pos);


			/**
			 * Generate code for invocations.
			 */

			// Generate member function calls.
			void executeMe(Par<bs::ExprBlock> in, Par<bs::Expr> me);

			// Generate member function call for token #pos.
			void executeToken(Par<bs::ExprBlock> in, Par<bs::Expr> me, Par<Token> token, nat pos);

			// Generate a member function call for token #pos, where 'src' is already loaded from 'this'.
			bs::Expr *executeToken(Par<bs::Block> in, Par<bs::Expr> me, Par<bs::Expr> src, Par<Token> token, nat pos);

			// Execute tokens inside an if-statement.
			void executeTokenIf(Par<bs::ExprBlock> in, Par<bs::Expr> me, Par<Token> token, nat pos);

			// Generate a loop executing tokens as many times as needed.
			void executeTokenLoop(nat from, nat to, Par<bs::ExprBlock> in, Par<bs::Expr> me);

			// Load any variables required by Token.
			void executeLoad(Par<bs::ExprBlock> in, Par<Token> token, nat pos);


			/**
			 * Helpers.
			 */

			// Get 'this'.
			bs::Expr *thisVar(Par<bs::Block> in);

			// Get 'pos'.
			bs::Expr *posVar(Par<bs::Block> in);

			// Get the underlying return type for a token (borrowed ptr).
			Type *tokenType(Par<Token> token);

			// Find the token storing to 'name'. Returns #tokens if none exists.
			nat findToken(const String &name);

			// Get a token (adds the capture token at the end if it exists). borrowed ptr.
			Token *getToken(nat pos);
			nat tokenCount();

			// Generate parameters required for invoking token #n, only precomputed variables are used.
			bs::Actual *readActuals(Par<bs::Block> in, nat pos);

			// Generate parameters required for invoking token #n (this parameter not added).
			bs::Actual *createActuals(Par<bs::ExprBlock> in, nat pos);

			// Find the transform function for a given set of parameters (this parameter added automatically).
			Function *findTransformFn(Par<Type> type, Par<bs::Actual> params);


		};

		// Create a function that transforms an option.
		Function *createTransformFn(Par<OptionDecl> option, Par<OptionType> owner, Scope scope);

	}
}
