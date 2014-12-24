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
			STORM_CTOR For(Auto<Block> parent);

			// Test expression.
			Auto<Expr> testExpr;
			void STORM_FN test(Auto<Expr> e);

			// Update expression.
			Auto<Expr> updateExpr;
			void STORM_FN update(Auto<Expr> e);

			// Body.
			Auto<Expr> bodyExpr;
			void STORM_FN body(Auto<Expr> e);

			// Result (always void).
			virtual Value result();

			// Code.
			virtual void blockCode(const GenState &s, GenResult &to, const code::Block &block);
		};

	}
}
