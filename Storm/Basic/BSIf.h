#pragma once
#include "BSBlock.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		class IfTrue;

		/**
		 * If-statement.
		 */
		class If : public Block {
			STORM_CLASS;
		public:
			STORM_CTOR If(Par<Block> parent);

			// Created variable that overrides the one in 'expression'.
			LocalVar *override();

			// Condition expression
			Auto<Expr> condition;

			// True branch.
			Auto<IfTrue> trueCode;

			// False branch, may be null.
			Auto<Expr> falseCode;

			// Set condition.
			virtual void STORM_FN cond(Par<Expr> e);

			// Set true/false code.
			virtual void STORM_FN trueExpr(Par<IfTrue> e);
			virtual void STORM_FN falseExpr(Par<Expr> e);

			// Result.
			virtual Value STORM_FN result();

			// Code.
			virtual void STORM_FN blockCode(Par<CodeGen> state, Par<CodeResult> r);

		private:
			// Overridden variable.
			Auto<LocalVar> created;
		};


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
			Auto<IfTrue> trueCode;

			// False branch, may be null.
			Auto<Expr> falseCode;

			// Use as if (<> as <>).
			void STORM_FN expr(Par<Expr> e);
			void STORM_FN type(Par<TypeName> t);

			// Set true/false code.
			void STORM_FN trueExpr(Par<IfTrue> e);
			void STORM_FN falseExpr(Par<Expr> e);

			// Result.
			virtual Value STORM_FN result();

			// Code.
			virtual void STORM_FN blockCode(Par<CodeGen> state, Par<CodeResult> r);

		private:
			// Overridden variable.
			Auto<LocalVar> created;

			// Validate types.
			void validate();

			// Get the candidate type (peels away Maybe)
			Value eValue();

		};

		/**
		 * Block which knows about our variable!
		 */
		class IfTrue : public Block {
			STORM_CLASS;
		public:
			STORM_CTOR IfTrue(Par<IfAs> parent);
			STORM_CTOR IfTrue(Par<If> parent);

			// Single contained expression.
			Auto<Expr> expr;
			void STORM_FN set(Par<Expr> expr);

			// Generate code.
			virtual Value STORM_FN result();
			virtual void STORM_FN blockCode(Par<CodeGen> state, Par<CodeResult> to);
		};

	}
}
