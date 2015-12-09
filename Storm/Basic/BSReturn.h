#pragma once
#include "BSBlock.h"

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
			STORM_CTOR Return(SrcPos pos, Par<Block> block);

			// Return a value.
			STORM_CTOR Return(SrcPos pos, Par<Block> block, Par<Expr> expr);

			// Value to return.
			STORM_VAR MAYBE(Auto<Expr>) expr;

			// Result (never returns).
			virtual ExprResult STORM_FN result();

			// Generate code.
			virtual void STORM_FN code(Par<CodeGen> state, Par<CodeResult> r);

		private:
			// Type to return.
			Value returnType;

			// Check type.
			void checkType();

			// Find the return type of the parent function.
			Value findParentType(SrcPos pos, Par<Block> block);

			// Code generation.
			void voidCode(Par<CodeGen> state);
			void valueCode(Par<CodeGen> state);
		};

	}
}
