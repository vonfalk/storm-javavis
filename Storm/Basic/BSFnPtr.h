#pragma once
#include "BSExpr.h"
#include "BSType.h"
#include "BSBlock.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		/**
		 * Create a function pointer.
		 */
		class FnPtr : public Expr {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR FnPtr(Par<Block> block, Par<TypeName> name, Par<ArrayP<TypeName>> formal);
			STORM_CTOR FnPtr(Par<Block> block, Par<Expr> dot, Par<SStr> name, Par<ArrayP<TypeName>> formal);

			// Result.
			virtual Value STORM_FN result();

			// Code.
			virtual void STORM_FN code(Par<CodeGen> state, Par<CodeResult> r);

		protected:
			virtual void output(wostream &to) const;

		private:
			// Anything before the dot?
			Auto<Expr> dotExpr;

			// Found function.
			Auto<Function> target;
		};


	}
}
