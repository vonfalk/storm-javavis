#pragma once
#include "BSExpr.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		/**
		 * A block. Blocks are expressions and return the last value in themselves.
		 */
		class Block : public Expr {
			STORM_CLASS;
		public:
			STORM_CTOR Block();
			STORM_CTOR Block(Auto<Block> parent);

			Block *parent; // No auto, will destroy refcounting.

			void STORM_FN expr(Auto<Expr> s);

			// Result.
			virtual Value result();

			// Generate code.
			virtual code::Listing code(code::Variable var);

			// Expressions in this block.
			vector<Auto<Expr> > exprs;

		};

	}
}
