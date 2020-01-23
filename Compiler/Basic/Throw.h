#pragma once
#include "Block.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		/**
		 * Throw statement.
		 */
		class Throw : public Expr {
			STORM_CLASS;
		public:
			// Throw a value.
			STORM_CTOR Throw(SrcPos pos, Expr *expr);

			// Value to throw.
			Expr *expr;

			// Result (never returns).
			virtual ExprResult STORM_FN result();

			// Generate code.
			virtual void STORM_FN code(CodeGen *state, CodeResult *r);

		protected:
			// Output.
			virtual void STORM_FN toS(StrBuf *to) const;
		};

	}
}
