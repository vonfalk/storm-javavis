#pragma once
#include "Std.h"
#include "SyntaxObject.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		/**
		 * Base class for expressions. (no difference between statements and expressions here!)
		 */
		class Expr : public Object {
			STORM_CLASS;
		public:
			STORM_CTOR Expr();
		};


		/**
		 * Constant (eg, number, string...).
		 */
		class Constant : public Expr {
			STORM_CLASS;
		public:
			STORM_CTOR Constant(Auto<SStr> value);
		};

	}
}
