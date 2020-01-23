#pragma once
#include "Block.h"

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
			STORM_CTOR For(SrcPos pos, Block *parent);

			// Test expression.
			Expr *testExpr;
			void STORM_FN test(Expr *e);

			// Update expression.
			Expr *updateExpr;
			void STORM_FN update(Expr *e);

			// Body.
			Expr *bodyExpr;
			void STORM_FN body(Expr *e);

			// Result (always void).
			virtual ExprResult STORM_FN result();

			// Code.
			virtual void STORM_FN blockCode(CodeGen *s, CodeResult *to, code::Block block);

		protected:
			// ToS.
			virtual void STORM_FN toS(StrBuf *to) const;
		};

	}
}
