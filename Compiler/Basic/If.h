#pragma once
#include "Block.h"
#include "WeakCast.h"
#include "Condition.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		/**
		 * If-statement.
		 */
		class If : public Block {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR If(Block *parent, Condition *cond);

			// Some shorthands for creating an if-statement.
			STORM_CTOR If(Block *parent, WeakCast *weak);
			STORM_CTOR If(Block *parent, Expr *expr);

			// The condition.
			Condition *condition;

			// True branch.
			MAYBE(CondSuccess *) trueCode;

			// False branch.
			MAYBE(Expr *) falseCode;

			// Set true and false branches from the parser.
			void STORM_FN trueBranch(CondSuccess *e);
			void STORM_FN falseBranch(Expr *e);

			// Result.
			virtual ExprResult STORM_FN result();

			// Code.
			virtual void STORM_FN blockCode(CodeGen *state, CodeResult *r);

		protected:
			// Output.
			virtual void STORM_FN toS(StrBuf *to) const;
		};

	}
}
