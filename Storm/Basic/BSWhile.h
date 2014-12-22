#pragma once
#include "BSBlock.h"

namespace storm {
	namespace bs {

		/**
		 * While block.
		 */
		class While : public Block {
			STORM_CLASS;
		public:
			STORM_CTOR While(Auto<Block> parent);

			// Condition expression.
			Auto<Expr> condExpr;
			void STORM_FN cond(Auto<Expr> e);

			// Content.
			Auto<Expr> bodyExpr;
			void STORM_FN body(Auto<Expr> e);

			// Result (always void).
			virtual Value result();

			// Code.
			virtual void blockCode(const GenState &state, GenResult &r, const code::Block &b);
		};

	}
}
