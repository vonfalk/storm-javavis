#pragma once
#include "BSExpr.h"
#include "BSBlock.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		/**
		 * Operator information. Create by either 'lOperator' or 'rOperator' for
		 * left- and right associative operators.
		 */
		class OpInfo : public ObjectOn<Compiler> {
			STORM_CLASS;
		public:
			// Ctor.
			STORM_CTOR OpInfo(Par<SStr> op, Int prio, Bool leftAssoc);

			// Priority. High priority is done before low priority.
			// Since each priority step may contain both left- and right
			// associative operators, we simply treat each priority step
			// as two steps depending on the associativity. Left associative
			// operators have higher priority in this case.
			STORM_VAR Int priority;

			// Left associative.
			STORM_VAR Bool leftAssoc;

			// Operator name.
			STORM_VAR Auto<Str> name;

			// Position.
			STORM_VAR SrcPos pos;

			// Shall this operator be executed before 'o'?
			// The equality segments defined by this function will only contain either
			// right or left associative operators.
			Bool STORM_FN before(Par<OpInfo> o);

			// Helpers based on 'before'.
			Bool STORM_FN after(Par<OpInfo> o);
			Bool STORM_FN eq(Par<OpInfo> o);

			// Create the meaning of this operator.
			virtual Expr *STORM_FN meaning(Par<Block> block, Par<Expr> lhs, Par<Expr> rhs);

		protected:
			virtual void output(wostream &to) const;
		};

		/**
		 * Info for the assignment operator.
		 */
		class AssignOpInfo : public OpInfo {
			STORM_CLASS;
		public:
			// Ctor.
			STORM_CTOR AssignOpInfo(Par<SStr> op, Int prio, Bool leftAssoc);

			// Custom meaning.
			virtual Expr *STORM_FN meaning(Par<Block> block, Par<Expr> lhs, Par<Expr> rhs);
		};

		// Create OpInfo.
		OpInfo *STORM_FN lOperator(Par<SStr> op, Int priority);
		OpInfo *STORM_FN rOperator(Par<SStr> op, Int priority);

		// Assignment operator. (right associative).
		OpInfo *STORM_FN assignOperator(Par<SStr> op, Int priority);

		/**
		 * Combined operator, ie an operator that combines assignment with another operator.  First
		 * looks for a specific overload of the operator, if it is not found, turns the expression
		 * into 'lhs = lhs + rhs'.
		 */
		class CombinedOperator : public OpInfo {
			STORM_CLASS;
		public:
			// Ctor.
			STORM_CTOR CombinedOperator(Par<OpInfo> op, Int prio);

			// Meaning.
			virtual Expr *STORM_FN meaning(Par<Block> block, Par<Expr> lhs, Par<Expr> rhs);

		private:
			// Wrapped operator.
			Auto<OpInfo> op;
		};

		/**
		 * Represents a binary operator. We need a special representation for it
		 * since we are not handling priority in the parser. Unary operators are
		 * easy to get right directly in the parser, so we do them right there.
		 */
		class Operator : public Expr {
			STORM_CLASS;
		public:
			// Ctor.
			STORM_CTOR Operator(Par<Block> block, Par<Expr> lhs, Par<OpInfo> op, Par<Expr> rhs);

			// Block (to get scoping info). (risk of cycles).
			Block *block;

			// Left- and right hand side.
			STORM_VAR Auto<Expr> lhs;
			STORM_VAR Auto<Expr> rhs;

			// Operator info.
			STORM_VAR Auto<OpInfo> op;

			// Alter the tree to match the priority. Returns the new leaf node.
			Operator *prioritize();

			// Invalidate the cached function we are about to call. Call if you alter 'lhs' or 'rhs.
			void invalidate();

			// Get the meaning from the operator (intended for debugging or further transforms).
			virtual Expr *STORM_FN meaning();

			// Result.
			virtual ExprResult STORM_FN result();

			// Generate code.
			virtual void STORM_FN code(Par<CodeGen> s, Par<CodeResult> r);

		protected:
			// Output.
			virtual void output(wostream &to) const;

		private:
			// The function call we are to execute. Lazy-loaded and invalidated by 'prioritize'.
			Auto<Expr> fnCall;
		};

		// Create an operator, swap the operators around to follow correct priority and return the topmost one.
		// This means that this function may not return a new operator always.
		Operator *STORM_FN mkOperator(Par<Block> block, Par<Expr> lhs, Par<OpInfo> op, Par<Expr> rhs);

		/**
		 * Wrap an expression in parens, so that the reordering will stop att paren boundaries.
		 */
		class ParenExpr : public Expr {
			STORM_CLASS;
		public:
			STORM_CTOR ParenExpr(Par<Expr> wrap);

			// Result.
			virtual ExprResult STORM_FN result();

			// Code.
			virtual void STORM_FN code(Par<CodeGen> s, Par<CodeResult> r);

		protected:
			// Output.
			virtual void output(wostream &to) const;

		private:
			// Expression we're containing.
			Auto<Expr> wrap;
		};


		/**
		 * Create various operators. These do not create Operator objects, since their priority
		 * is decided completely at parse-time.
		 */

		// Element access operator.
		Expr *STORM_FN accessExpr(Par<Block> block, Par<Expr> lhs, Par<Expr> par);

		// Prefix operator.
		Expr *STORM_FN prefixOperator(Par<Block> block, Par<SStr> o, Par<Expr> expr);

		// Postfix operator.
		Expr *STORM_FN postfixOperator(Par<Block> block, Par<SStr> o, Par<Expr> expr);

		// Prefix or postfix operator (there is only one variant).
		Expr *STORM_FN prePostOperator(Par<Block> block, Par<SStr> o, Par<Expr> expr);

	}
}