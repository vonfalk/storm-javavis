#pragma once
#include "Std.h"
#include "SyntaxObject.h"
#include "Code/Listing.h"
#include "Code/Block.h"

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

			// Result of an expression. Default is null. TODO: STORM_FN
			virtual Value result();

			// Generate code, place result in 'var' unless 'var' == Variable::invalid.
			virtual code::Listing code(code::Variable var);
		};


		/**
		 * Constant (eg, number, string...).
		 */
		class Constant : public Expr {
			STORM_CLASS;
		public:
			STORM_CTOR Constant(Int i);

			// Types
			enum Type {
				tInt,
			};

			// Actual type.
			Type cType;

			// Value (if integer).
			Int value;

			// Return value.
			virtual Value result();

			// Generate code.
			virtual code::Listing code(code::Variable var);

		protected:
			virtual void output(wostream &out) const;
		};

		Constant *STORM_FN intConstant(Auto<SStr> str);
	}
}
