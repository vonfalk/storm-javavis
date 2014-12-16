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
			STORM_CTOR If(Auto<Block> parent);

			// Condition expression
			Auto<Expr> condition;

			// True branch.
			Auto<Expr> trueCode;

			// False branch, may be null.
			Auto<Expr> falseCode;

			// Set condition.
			void STORM_FN cond(Auto<Expr> e);

			// Set true/false code.
			void STORM_FN trueExpr(Auto<Expr> e);
			void STORM_FN falseExpr(Auto<Expr> e);

			// Result.
			virtual Value result();

			// Code.
			virtual void blockCode(const GenState &state, GenResult &r);
		};

	}
}
