#pragma once
#include "Block.h"
#include "Condition.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		/**
		 * Implements the unless (<cast>) return X; ... syntax.
		 */
		class Unless : public Block {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR Unless(Block *block, Condition *cond);

			// Condition.
			Condition *cond;

			// Statement to be executed if the cast fails.
			Expr *failStmt;

			// Block to be executed if the cast succeeds.
			ExprBlock *successBlock;

			// Set the fail statement.
			void STORM_FN fail(Expr *expr);

			// Add to the success statement.
			void STORM_FN success(Expr *expr);

			// Result.
			virtual ExprResult STORM_FN result();

			// Generate code.
			virtual void STORM_FN blockCode(CodeGen *state, CodeResult *r);

		protected:
			// Output.
			virtual void STORM_FN toS(StrBuf *to) const;

		private:
			// The CondSuccess that is the root of the success block.
			CondSuccess *successRoot;
		};


	}
}
