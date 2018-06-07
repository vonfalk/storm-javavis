#pragma once
#include "Expr.h"
#include "Block.h"
#include "Compiler/Function.h"

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
			STORM_CTOR FnPtr(Block *block, SrcName *name, Array<SrcName *> *formal);
			STORM_CTOR FnPtr(Block *block, Expr *dot, syntax::SStr *name, Array<SrcName *> *formal);

			// Create, assuming we already know the exact function we want to get a pointer for.
			STORM_CTOR FnPtr(Function *target, SrcPos pos);
			STORM_CTOR FnPtr(Expr *dot, Function *target, SrcPos pos);

			// Result.
			virtual ExprResult STORM_FN result();

			// Code.
			virtual void STORM_FN code(CodeGen *state, CodeResult *r);

		protected:
			virtual void STORM_FN toS(StrBuf *to) const;

		private:
			// Anything before the dot?
			Expr *dotExpr;

			// Found function.
			Function *target;
		};

	}
}
