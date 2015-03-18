#pragma once
#include "BSBlock.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		/**
		 * While block.
		 */
		class While : public Block {
			STORM_CLASS;
		public:
			STORM_CTOR While(Par<Block> parent);

			// Condition expression.
			Auto<Expr> condExpr;
			void STORM_FN cond(Par<Expr> e);

			// Content.
			Auto<Expr> bodyExpr;
			void STORM_FN body(Par<Expr> e);

			// Result (always void).
			virtual Value result();

			// Code.
			virtual void blockCode(GenState &state, GenResult &r, const code::Block &b);
		};

	}
}
