#pragma once
#include "BSBlock.h"

namespace storm {
	namespace bs {

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
			STORM_CTOR Loop(SrcPos pos, Par<Block> parent);

			// Condition (if any).
			Auto<Expr> condExpr;
			void STORM_FN cond(Par<Expr> e);

			// Do content (if any). Will adopt any variables in the block 'e' to make scoping correct in 'while'.
			Auto<Expr> doExpr;
			void STORM_FN doBody(Par<Expr> e);

			// While content (if any).
			Auto<Expr> whileExpr;
			void STORM_FN whileBody(Par<Expr> e);

			// Result (always void).
			virtual ExprResult STORM_FN result();

			// Code.
			virtual void blockCode(Par<CodeGen> state, Par<CodeResult> r, const code::Block &b);

		protected:
			virtual void output(wostream &to) const;
		};

	}
}
