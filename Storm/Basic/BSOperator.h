#pragma once
#include "BSExpr.h"
#include "BSBlock.h"

namespace storm {
	namespace bs {

		/**
		 * Operator information. Create by either 'lOperator' or 'rOperator' for
		 * left- and right associative operators.
		 */
		class OpInfo : public Object {
			STORM_CLASS;
		public:
			// Ctor.
			STORM_CTOR OpInfo(Par<SStr> op, Int prio, Bool leftAssoc);

			// Priority. High priority is done before low priority.
			// Since each priority step may contain both left- and right
			// associative operators, we simply treat each priority step
			// as two steps depending on the associativity. Left associative
			// operators have higher priority in this case.
			Int priority;

			// Left associative.
			Bool leftAssoc;

			// Operator name.
			Auto<Str> name;

			// Position.
			SrcPos pos;

			// Shall this operator be executed before 'o'?
			// The equality segments defined by this function will only contain either
			// right or left associative operators.
			Bool STORM_FN before(Par<OpInfo> o);
		};

		// Create OpInfo.
		OpInfo *STORM_FN lOperator(Par<SStr> op, Int priority);
		OpInfo *STORM_FN rOperator(Par<SStr> op, Int priority);

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
			Auto<Expr> lhs, rhs;

			// Operator info.
			Auto<OpInfo> op;

		protected:
			// Output.
			virtual void output(wostream &to) const;
		};


		/**
		 * Create various operators. These do not create Operator objects, since their priority
		 * is decided completely at parse-time.
		 */

		// Assignment operator. (TODO: Use Operator here as well?)
		Expr *STORM_FN assignExpr(Par<Block> block, Par<Expr> lhs, Par<SStr> m, Par<Expr> rhs);

		// Element access operator.
		Expr *STORM_FN accessExpr(Par<Block> block, Par<Expr> lhs, Par<Expr> par);

		// Prefix operator.
		Expr *STORM_FN prefixOperator(Par<Block> block, Par<SStr> o, Par<Expr> expr);

		// Postfix operator.
		Expr *STORM_FN postfixOperator(Par<Block> block, Par<SStr> o, Par<Expr> expr);

	}
}
