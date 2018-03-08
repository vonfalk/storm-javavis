#pragma once
#include "Expr.h"
#include "Block.h"
#include "Actuals.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		/**
		 * Unresolved named expression returned from 'namedExpr' in case a name is not found. This
		 * is useful since parts of the system (such as the assignment operator when using setters)
		 * need to inspect and modify a possibly incorrect name.
		 *
		 * Accessing any of the member functions that would require a valid name will throw an
		 * appropriate exception. This basically means that the error 'namedExpr' would throw is
		 * delayed until another class tries to access the result rather than being thrown immediately.
		 */
		class UnresolvedName : public Expr {
			STORM_CLASS;
		public:
			// Create. Takes the same parameters as the internal 'findTarget' function so that the
			// same query can be repeated later on.
			UnresolvedName(Block *block, SimpleName *name, SrcPos pos, Actuals *params, Bool useThis);

			// Block.
			Block *block;

			// Name.
			SimpleName *name;

			// Actual parameters.
			Actuals *params;

			// Use the 'this' parameter?
			Bool useThis;

			// Retry with different parameters.
			Expr *retry(Actuals *params) const;

			// Result type.
			virtual ExprResult STORM_FN result();

			// Generate code.
			virtual void STORM_FN code(CodeGen *s, CodeResult *to);

		protected:
			// Output.
			virtual void STORM_FN toS(StrBuf *to) const;

		private:
			// Throw the error.
			void error() const;
		};


		// Find out what the named expression means, and create proper object.
		Expr *STORM_FN namedExpr(Block *block, syntax::SStr *name, Actuals *params);
		Expr *STORM_FN namedExpr(Block *block, SrcName *name, Actuals *params);
		Expr *STORM_FN namedExpr(Block *block, SrcPos pos, Name *name, Actuals *params);

		// Special case of above, used when we find an expression like a.b(...). 'first' is inserted
		// into the beginning of 'params' and used. This method inhibits automatic insertion of 'this'.
		Expr *STORM_FN namedExpr(Block *block, syntax::SStr *name, Expr *first, Actuals *params);
		Expr *STORM_FN namedExpr(Block *block, syntax::SStr *name, Expr *first);

	}
}
