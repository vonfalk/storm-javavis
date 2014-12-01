#pragma once
#include "BSExpr.h"
#include "BSScope.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		/**
		 * A block. Blocks are expressions and return the last value in themselves.
		 */
		class Block : public Expr {
			STORM_CLASS;
		public:
			STORM_CTOR Block(Auto<BSScope> scope);
			STORM_CTOR Block(Auto<Block> parent);

			// Scope.
			Auto<BSScope> scope;

			// No auto, will destroy refcounting.
			Block *parent;

			void STORM_FN expr(Auto<Expr> s);

			// Result.
			virtual Value result();

			// Generate code.
			virtual void code(GenState state, code::Variable var);

			// Expressions in this block.
			vector<Auto<Expr> > exprs;

		};

	}
}
