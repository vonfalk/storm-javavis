#pragma once
#include "Decl.h"
#include "Token.h"
#include "Core/Set.h"
#include "Core/Array.h"
#include "Compiler/Basic/Function.h"
#include "Compiler/Basic/Expr.h"
#include "Compiler/Basic/Block.h"

namespace storm {
	namespace syntax {
		STORM_PKG(lang.bnf);

		class Production;
		class ProductionType;

		/**
		 * Logic for generating code that transforms a syntax tree into the user's preferred
		 * representation.
		 *
		 * Built on top of Basic Storm.
		 */

		/**
		 * Implementation of the transform function.
		 */
		class TransformFn : public bs::BSRawFn {
			STORM_CLASS;
		public:
			// Create the transform function.
			STORM_CTOR TransformFn(ProductionDecl *decl, ProductionType *owner, Rule *rule, Scope scope);

			// Create body.
			virtual bs::FnBody *STORM_FN createBody();

		private:
			// Scope.
			Scope scope;

			// Result and parameters to the result (may be null).
			Name *result;
			Array<Str *> *resultParams;

			// The Production we're transforming.
			Production *source;

			// Remembered parameters.
			class Params {
				STORM_VALUE;
			public:
				STORM_CTOR Params() { v = null; }

				// May be null.
				Array<Str *> *v;
			};

			// Parameters to rules in 'source'. These are not stored in 'source', so we need to
			// remember them ourself. May contain null, so do not return directly to storm!
			Array<Params> *tokenParams;

			// Keep track of all parameters we're looking for, so we can report an error if we find
			// any cycles.
			Set<Str *> *lookingFor;


			/**
			 * Create variables.
			 */

			// Create the variable 'me' from the expression (if needed).
			bs::Expr *createMe(bs::ExprBlock *in);

			// Find a variable. Throws exception if it is not yet created.
			bs::Expr *readVar(bs::Block *in, Str *name);

			// Find (and create if neccessary) a variable.
			bs::Expr *findVar(bs::ExprBlock *in, Str *name);

			// Create a variable.
			bs::LocalVar *createVar(bs::ExprBlock *in, Str *name, nat pos);

			// Create a variable by copying what is in the syntax tree (the case with RegexTokens).
			bs::LocalVar *createPlainVar(bs::ExprBlock *in, Str *name, Token *token);

			// Create a variable from by transforming the original syntax tree.
			bs::LocalVar *createTfmVar(bs::ExprBlock *in, Str *name, Token *token, nat pos);


			/**
			 * Generate code for invocations.
			 */

			// Generate member function calls.
			void executeMe(bs::ExprBlock *in, bs::Expr *me);

			// Generate member function call for token #pos.
			void executeToken(bs::ExprBlock *in, bs::Expr *me, Token *token, nat pos);

			// Generate a member function call for token #pos, where 'src' is already loaded from 'this'.
			bs::Expr *executeToken(bs::Block *in, bs::Expr *me, bs::Expr *src, Token *token, nat pos);

			// Execute tokens inside an if-statement.
			void executeTokenIf(bs::ExprBlock *in, bs::Expr *me, Token *token, nat pos);

			// Generate a loop executing tokens as many times as needed.
			void executeTokenLoop(nat from, nat to, bs::ExprBlock *in, bs::Expr *me);

			// Load any variables required by Token.
			void executeLoad(bs::ExprBlock *in, Token *token, nat pos);


			/**
			 * Helpers.
			 */

			// Get 'this'.
			bs::Expr *thisVar(bs::Block *in);

			// Get 'pos'.
			bs::Expr *posVar(bs::Block *in);

			// Get the underlying return type for a token (borrowed ptr).
			Type *tokenType(Token *token);

			// Find the token storing to 'name'. Returns #tokens if none exists.
			nat findToken(Str *name);

			// Get a token (adds the capture token at the end if it exists). borrowed ptr.
			Token *getToken(nat pos);
			nat tokenCount();

			// Generate parameters required for invoking token #n, only precomputed variables are used.
			bs::Actuals *readActuals(bs::Block *in, nat pos);

			// Generate parameters required for invoking token #n (this parameter not added).
			bs::Actuals *createActuals(bs::ExprBlock *in, nat pos);

			// Find the transform function for a given set of parameters (this parameter added automatically).
			Function *findTransformFn(Type *type, bs::Actuals *params);


		};

		// Create a function that transforms an option.
		Function *createTransformFn(ProductionDecl *option, ProductionType *owner, Scope scope);

	}
}
