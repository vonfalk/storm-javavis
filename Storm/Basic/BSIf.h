#pragma once
#include "BSBlock.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		/**
		 * If-statement.
		 */
		class If : public Block {
			STORM_CLASS;
		public:
			STORM_CTOR If(Par<Block> parent);

			// Condition expression
			Auto<Expr> condition;

			// True branch.
			Auto<Expr> trueCode;

			// False branch, may be null.
			Auto<Expr> falseCode;

			// Set condition.
			void STORM_FN cond(Par<Expr> e);

			// Set true/false code.
			void STORM_FN trueExpr(Par<Expr> e);
			void STORM_FN falseExpr(Par<Expr> e);

			// Result.
			virtual Value result();

			// Code.
			virtual void blockCode(GenState &state, GenResult &r);
		};

	}
}
