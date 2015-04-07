#pragma once
#include "BSBlock.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		/**
		 * If-statement.
		 */
		class If : public Block {
			STORM_CLASS;
		public:
			STORM_CTOR If(Par<Block> parent);

			// Condition expression
			Auto<Expr> condition;

			// True branch.
			Auto<Expr> trueCode;

			// False branch, may be null.
			Auto<Expr> falseCode;

			// Set condition.
			void STORM_FN cond(Par<Expr> e);

			// Set true/false code.
			void STORM_FN trueExpr(Par<Expr> e);
			void STORM_FN falseExpr(Par<Expr> e);

			// Result.
			virtual Value result();

			// Code.
			virtual void blockCode(const GenState &state, GenResult &r);
		};


		class IfAsTrue;

		/**
		 * If-as statement.
		 */
		class IfAs : public Block {
			STORM_CLASS;
		public:
			STORM_CTOR IfAs(Par<Block> parent);

			// Expression and type.
			Auto<Expr> expression;
			Auto<Type> target;

			// Created variable that overrides the one in 'expression'.
			LocalVar *override();

			// True branch.
			Auto<IfAsTrue> trueCode;

			// False branch, may be null.
			Auto<Expr> falseCode;

			// Use as if (<> as <>).
			void STORM_FN expr(Par<Expr> e);
			void STORM_FN type(Par<TypeName> t);

			// Set true/false code.
			void STORM_FN trueExpr(Par<IfAsTrue> e);
			void STORM_FN falseExpr(Par<Expr> e);

			// Result.
			virtual Value result();

			// Code.
			virtual void blockCode(const GenState &state, GenResult &r);

		private:
			// Overridden variable.
			Auto<LocalVar> created;

			// Validate types.
			void validate();

		};

		/**
		 * Block which knows about our variable!
		 */
		class IfAsTrue : public Block {
			STORM_CLASS;
		public:
			STORM_CTOR IfAsTrue(Par<IfAs> parent);

			// Single contained expression.
			Auto<Expr> expr;
			void STORM_FN set(Par<Expr> expr);

			// Generate code.
			virtual Value result();
			virtual void blockCode(const GenState &state, GenResult &to);
		};

	}
}
