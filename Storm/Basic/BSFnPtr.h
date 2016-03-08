#pragma once
#include "BSExpr.h"
#include "BSBlock.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		/**
		 * Create a function pointer.
		 */
		class FnPtr : public Expr {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR FnPtr(Par<Block> block, Par<SrcName> name, Par<ArrayP<SrcName>> formal);
			STORM_CTOR FnPtr(Par<Block> block, Par<Expr> dot, Par<SStr> name, Par<ArrayP<SrcName>> formal, Bool strong);

			// Result.
			virtual ExprResult STORM_FN result();

			// Code.
			virtual void STORM_FN code(Par<CodeGen> state, Par<CodeResult> r);

		protected:
			virtual void output(wostream &to) const;

		private:
			// Anything before the dot?
			Auto<Expr> dotExpr;

			// Found function.
			Auto<Function> target;

			// Strong this ptr?
			bool strongThis;
		};

		// Helpers for creation.
		FnPtr *STORM_FN strongFnPtr(Par<Block> block, Par<Expr> dot, Par<SStr> name, Par<ArrayP<SrcName>> formal);
		FnPtr *STORM_FN weakFnPtr(Par<Block> block, Par<Expr> dot, Par<SStr> name, Par<ArrayP<SrcName>> formal);

	}
}
