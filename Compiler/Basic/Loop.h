#pragma once
#include "Block.h"
#include "Condition.h"

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

			// Set the condition.
			void STORM_FN cond(Condition *cond);

			// Helpers for common tasks.
			void STORM_FN condExpr(Expr *expr);
			void STORM_FN condWeak(WeakCast *weak);

			// Get the condition. Throw on failure.
			Condition *STORM_FN cond();

			// Do content (if any). Will adopt any variables in the block 'e' to make scoping
			// correct in 'while', and allow local variables inside the 'do' part of the loop to be
			// used in the condition.
			void STORM_FN doBody(Expr *e);

			// Set the body for the while expression.
			void STORM_FN whileBody(CondSuccess *s);

			// Create a CondSuccess for use with 'whileBody'. This can be done manually as well.
			CondSuccess *STORM_FN createWhileBody();

			// Result (always void).
			virtual ExprResult STORM_FN result();

			// Code.
			virtual void STORM_FN code(CodeGen *state, CodeResult *r);

		protected:
			virtual void STORM_FN toS(StrBuf *to) const;

		private:
			// Condition (if any).
			MAYBE(Condition *) condition;

			// Do-content.
			MAYBE(Expr *) doExpr;

			// While content.
			MAYBE(CondSuccess *) whileExpr;

			// Code generation.
			void code(CodeGen *outer, CodeGen *inner, CodeResult *r);
		};


	}
}
