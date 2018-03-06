#pragma once
#include "Operator.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		/**
		 * Implementation of operators used in Basic Storm.
		 */

		/**
		 * Info for the assignment operator.
		 */
		class AssignOpInfo : public OpInfo {
			STORM_CLASS;
		public:
			// Ctor.
			STORM_CTOR AssignOpInfo(syntax::SStr *op, Int prio, Bool leftAssoc);

			// Custom meaning.
			virtual Expr *STORM_FN meaning(Block *block, Expr *lhs, Expr *rhs);

		private:
			// Try to find an applicable setter function.
			Expr *findSetter(Block *block, Expr *lhs, Expr *rhs);
		};

		// Assignment operator. (right associative).
		OpInfo *STORM_FN assignOperator(syntax::SStr *op, Int priority);


		/**
		 * Info for the 'is' operator.
		 */
		class IsOperator : public OpInfo {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR IsOperator(syntax::SStr *op, Int prio, Bool negate);

			// Custom meaning.
			virtual Expr *STORM_FN meaning(Block *block, Expr *lhs, Expr *rhs);

		private:
			// Negated.
			Bool negate;
		};


		/**
		 * Combined operator, ie an operator that combines assignment with another operator.  First
		 * looks for a specific overload of the operator, if it is not found, turns the expression
		 * into 'lhs = lhs + rhs'.
		 */
		class CombinedOperator : public OpInfo {
			STORM_CLASS;
		public:
			// Ctor.
			STORM_CTOR CombinedOperator(OpInfo *op, Int prio);

			// Meaning.
			virtual Expr *STORM_FN meaning(Block *block, Expr *lhs, Expr *rhs);

		private:
			// Wrapped operator.
			OpInfo *op;
		};


		// Description of a fallback.
		struct OpFallback;

		/**
		 * Operator with a number of 'fallbacks'. Used to implement automatic generation of
		 * operators such as '<', '!=', '>', etc.
		 */
		class FallbackOperator : public OpInfo {
			STORM_CLASS;
		public:
			// Ctor. 'fallback' needs to be statically allocated.
			FallbackOperator(syntax::SStr *op, Int prio, Bool leftAssoc, const OpFallback *fallback);

			// Meaning.
			virtual Expr *STORM_FN meaning(Block *block, Expr *lhs, Expr *rhs);

		private:
			// Fallback description.
			const OpFallback *fallback;

			// Try the fallback at 'id'. May advance 'id' by one if required.
			Expr *tryFallback(Nat &id, Block *block, Expr *lhs, Expr *rhs);
			Expr *tryFallback(const OpFallback &f, Block *block, Expr *lhs, Expr *rhs);
		};


		// Create fallback operators for the comparison operators.
		OpInfo *STORM_FN compareLt(syntax::SStr *op, Int priority);
		OpInfo *STORM_FN compareLte(syntax::SStr *op, Int priority);
		OpInfo *STORM_FN compareGt(syntax::SStr *op, Int priority);
		OpInfo *STORM_FN compareGte(syntax::SStr *op, Int priority);
		OpInfo *STORM_FN compareEq(syntax::SStr *op, Int priority);
		OpInfo *STORM_FN compareNeq(syntax::SStr *op, Int priority);

	}
}
