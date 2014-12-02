#pragma once
#include "Std.h"
#include "SyntaxObject.h"
#include "CodeGen.h"
#include "Code/Listing.h"
#include "Code/Block.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		/**
		 * Base class for expressions. (no difference between statements and expressions here!)
		 */
		class Expr : public SObject {
			STORM_CLASS;
		public:
			STORM_CTOR Expr();

			// Result of an expression. Default is null. TODO: STORM_FN
			virtual Value result();

			// Generate code, place result in 'var' unless 'var' == Variable::invalid.
			// TODO: To avoid loads of temporary variables, change to code(state, bool result) -> Variable
			virtual void code(const GenState &state, code::Variable var);
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
			virtual void code(const GenState &state, code::Variable var);

		protected:
			virtual void output(wostream &out) const;
		};

		Constant *STORM_FN intConstant(Auto<SStr> str);

		class Block;

		/**
		 * Operator.
		 */
		class Operator : public Expr {
			STORM_CLASS;
		public:
			STORM_CTOR Operator(Auto<Block> block, Auto<Expr> lhs, Auto<SStr> m, Auto<Expr> rhs);

			// Owning block.
			Block *block;

			// Sub-expressions.
			Auto<Expr> lhs;
			Auto<Expr> rhs;

			// The actual operator.
			Auto<Str> op;

			// Result.
			virtual Value result();

			// Generate code.
			virtual void code(const GenState &to, code::Variable var);

		private:
			// Get the function to call.
			Function *findFn();

			// Cache of the function to call.
			Function *fn;
		};

	}
}
