#pragma once
#include "BSBlock.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		/**
		 * For loop (basic). The start expression should
		 * be placed in a block outside the for loop.
		 */
		class For : public Block {
			STORM_CLASS;
		public:
			STORM_CTOR For(Par<Block> parent);

			// Test expression.
			Auto<Expr> testExpr;
			void STORM_FN test(Par<Expr> e);

			// Update expression.
			Auto<Expr> updateExpr;
			void STORM_FN update(Par<Expr> e);

			// Body.
			Auto<Expr> bodyExpr;
			void STORM_FN body(Par<Expr> e);

			// Result (always void).
			virtual Value result();

			// Code.
			virtual void blockCode(GenState &s, GenResult &to, const code::Block &block);
		};

	}
}
