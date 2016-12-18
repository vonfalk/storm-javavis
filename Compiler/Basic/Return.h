#pragma once
#include "Block.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		/**
		 * Return statement.
		 */
		class Return : public Expr {
			STORM_CLASS;
		public:
			// Return nothing.
			STORM_CTOR Return(SrcPos pos, Block *block);

			// Return a value.
			STORM_CTOR Return(SrcPos pos, Block *block, Expr *expr);

			// Value to return.
			MAYBE(Expr *) expr;

			// Result (never returns).
			virtual ExprResult STORM_FN result();

			// Generate code.
			virtual void STORM_FN code(CodeGen *state, CodeResult *r);

		protected:
			// Output.
			virtual void STORM_FN toS(StrBuf *to) const;

		private:
			// Type to return.
			Value returnType;

			// Check type.
			void checkType();

			// Find the return type of the parent function.
			Value findParentType(SrcPos pos, Block *block);

			// Code generation.
			void voidCode(CodeGen *state);
			void valueCode(CodeGen *state);
		};

	}
}
