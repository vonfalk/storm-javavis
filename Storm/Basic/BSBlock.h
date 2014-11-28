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
		};

	}
}
