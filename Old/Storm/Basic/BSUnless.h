#pragma once
#include "BSBlock.h"
#include "BSWeakCast.h"

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
			STORM_CTOR Unless(Par<Block> block, Par<WeakCast> cast);
			STORM_CTOR Unless(Par<Block> block, Par<WeakCast> cast, Par<SStr> name);

			// Cast.
			Auto<WeakCast> cast;

			// Statement to be executed if the cast fails.
			Auto<Expr> failStmt;

			// Block to be executed if the cast succeeds.
			Auto<ExprBlock> successBlock;

			// Set the fail statement.
			void STORM_FN fail(Par<Expr> expr);

			// Add to the success statement.
			void STORM_FN success(Par<Expr> expr);

			// Result.
			virtual ExprResult STORM_FN result();

			// Generate code.
			virtual void STORM_FN blockCode(Par<CodeGen> state, Par<CodeResult> r);

		protected:
			// Output.
			virtual void output(wostream &to) const;

		private:
			// Variable to overwrite.
			Auto<LocalVar> overwrite;

			// Set up ourselves.
			void init(Auto<Str> name);
		};

	}
}
