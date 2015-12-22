#pragma once
#include "BSBlock.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		/**
		 * Weak cast, ie. a cast that may fail and has to be handled.
		 */
		class WeakCast : public SObject {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR WeakCast();

			// Get the name of a local variable we want to save the result of the weak cast to. May
			// return null if there is no obvious location to store the result in.
			virtual MAYBE(Str) *STORM_FN overwrite();

			// Get the resulting variable-type of this expression.
			virtual Value STORM_FN result();

			// Generate code. 'boolResult' indicates if the cast succeeded or failed. 'var' is the location to pass
			// the result of the cast to be used in the 'true' branch.
			virtual void STORM_FN code(Par<CodeGen> state, Par<CodeResult> boolResult, MAYBE(Par<LocalVar>) var);

			// Default implementation of 'overwrite'. Extracts the name from an expression if possible.
			MAYBE(Str) *STORM_FN defaultOverwrite(Par<Expr> expr);
		};


		/**
		 * Weak downcast using 'as'.
		 */
		class WeakDowncast : public WeakCast {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR WeakDowncast(Par<Block> block, Par<Expr> expr, Par<TypeName> type);

			// Get variable we can write to.
			virtual MAYBE(Str) *STORM_FN overwrite();

			// Get the result type.
			virtual Value STORM_FN result();

			// Generate code.
			virtual void STORM_FN code(Par<CodeGen> state, Par<CodeResult> boolResult, MAYBE(Par<LocalVar>) var);

		protected:
			// Output.
			virtual void output(wostream &to) const;

		private:
			// Expression to cast.
			Auto<Expr> expr;

			// Type to cast to.
			Value to;
		};

		/**
		 * Weak cast from Maybe<T> to T.
		 */
		class WeakMaybeCast : public WeakCast {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR WeakMaybeCast(Par<Expr> expr);

			// Get variable we can write to.
			virtual MAYBE(Str) *STORM_FN overwrite();

			// Get the result type.
			virtual Value STORM_FN result();

			// Generate code.
			virtual void STORM_FN code(Par<CodeGen> state, Par<CodeResult> boolResult, MAYBE(Par<LocalVar>) var);

		protected:
			// Output.
			virtual void output(wostream &to) const;

		private:
			// Expression to cast.
			Auto<Expr> expr;
		};

		// Figure out what kind of weak cast to create based on the type of the lhs of expressions like <lhs> as <type>.
		WeakCast *STORM_FN weakAsCast(Par<Block> block, Par<Expr> expr, Par<TypeName> type);

	}
}
