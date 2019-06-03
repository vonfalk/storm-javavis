#pragma once
#include "Block.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		/**
		 * The loop in BS. Either:
		 * do { }
		 * do { } while (x);
		 * do { } while (x) { }
		 * while (x) { }
		 */
		class Loop : public Block {
			STORM_CLASS;
		public:
			STORM_CTOR Loop(SrcPos pos, Block *parent);

			// Condition (if any).
			MAYBE(Expr *) condExpr;
			void STORM_FN cond(Expr *e);

			// Do content (if any). Will adopt any variables in the block 'e' to make scoping correct in 'while'.
			MAYBE(Expr *) doExpr;
			void STORM_FN doBody(Expr *e);

			// While content (if any).
			MAYBE(Expr *) whileExpr;
			void STORM_FN whileBody(Expr *e);

			// Result (always void).
			virtual ExprResult STORM_FN result();

			// Code.
			virtual void blockCode(CodeGen *state, CodeResult *r, const code::Block &b);

		protected:
			virtual void STORM_FN toS(StrBuf *to) const;
		};

	}
}
