#pragma once
#include "Expr.h"
#include "Block.h"

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
			STORM_CTOR OpInfo(syntax::SStr *op, Int prio, Bool leftAssoc);

			// Priority. High priority is done before low priority.
			// Since each priority step may contain both left- and right
			// associative operators, we simply treat each priority step
			// as two steps depending on the associativity. Left associative
			// operators have higher priority in this case.
			Int priority;

			// Left associative.
			Bool leftAssoc;

			// Operator name.
			Str *name;

			// Position.
			SrcPos pos;

			// Shall this operator be executed before 'o'?
			// The equality segments defined by this function will only contain either
			// right or left associative operators.
			Bool STORM_FN before(OpInfo *o);

			// Helpers based on 'before'.
			Bool STORM_FN after(OpInfo *o);
			Bool STORM_FN eq(OpInfo *o);

			// Create the meaning of this operator.
			virtual Expr *STORM_FN meaning(Block *block, Expr *lhs, Expr *rhs);

		protected:
			// To string.
			virtual void STORM_FN toS(StrBuf *to) const;

			// Resolve the operator 'name' according to the normal rules. Returns 'null' if nothing usable is found.
			MAYBE(Expr *) STORM_FN find(Block *block, Str *name, Expr *lhs, Expr *rhs);
			MAYBE(Expr *) STORM_FN find(Block *block, Str *name, Expr *lhs, Expr *rhs, Bool strict);
		};


		// Create OpInfo.
		OpInfo *STORM_FN lOperator(syntax::SStr *op, Int priority);
		OpInfo *STORM_FN rOperator(syntax::SStr *op, Int priority);


		/**
		 * Represents a binary operator. We need a special representation for it
		 * since we are not handling priority in the parser. Unary operators are
		 * easy to get right directly in the parser, so we handle them right there.
		 */
		class Operator : public Expr {
			STORM_CLASS;
		public:
			// Ctor.
			STORM_CTOR Operator(Block *block, Expr *lhs, OpInfo *op, Expr *rhs);

			// Block (to get scoping info). (risk of cycles).
			Block *block;

			// Left- and right hand side.
			Expr *lhs;
			Expr *rhs;

			// Operator info.
			OpInfo *op;

			// Alter the tree to match the priority. Returns the new leaf node.
			Operator *prioritize();

			// Invalidate the cached function we are about to call. Call if you alter 'lhs' or 'rhs.
			void invalidate();

			// Get position.
			virtual SrcPos STORM_FN largePos();

			// Get the meaning from the operator (intended for debugging or further transforms).
			virtual Expr *STORM_FN meaning();

			// Result.
			virtual ExprResult STORM_FN result();

			// Generate code.
			virtual void STORM_FN code(CodeGen *s, CodeResult *r);

		protected:
			// Output.
			virtual void STORM_FN toS(StrBuf *to) const;

		private:
			// The function call we are to execute. Lazy-loaded and invalidated by 'prioritize'.
			Expr *fnCall;
		};

		// Create an operator, swap the operators around to follow correct priority and return the topmost one.
		// This means that this function may not return a new operator always.
		Operator *STORM_FN mkOperator(Block *block, Expr *lhs, OpInfo *op, Expr *rhs);

		/**
		 * Wrap an expression in parens, so that the reordering will stop att paren boundaries.
		 */
		class ParenExpr : public Expr {
			STORM_CLASS;
		public:
			STORM_CTOR ParenExpr(Expr *wrap);

			// Result.
			virtual ExprResult STORM_FN result();

			// Code.
			virtual void STORM_FN code(CodeGen *s, CodeResult *r);

		protected:
			// Output.
			virtual void STORM_FN toS(StrBuf *to) const;

		private:
			// Expression we're containing.
			Expr *wrap;
		};


		/**
		 * Create various operators. These do not create Operator objects, since their priority
		 * is decided completely at parse-time.
		 */

		// Element access operator.
		Expr *STORM_FN accessExpr(Block *block, Expr *lhs, Expr *par);

		// Prefix operator.
		Expr *STORM_FN prefixOperator(Block *block, syntax::SStr *o, Expr *expr);

		// Postfix operator.
		Expr *STORM_FN postfixOperator(Block *block, syntax::SStr *o, Expr *expr);

		// Prefix or postfix operator (there is only one variant).
		Expr *STORM_FN prePostOperator(Block *block, syntax::SStr *o, Expr *expr);

	}
}
