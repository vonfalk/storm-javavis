#pragma once
#include "BSBlock.h"
#include "BSWeakCast.h"

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
			STORM_CTOR If(Par<Block> parent, Par<Expr> cond);

			// Condition expression
			Auto<Expr> condition;

			// True branch.
			Auto<Expr> trueCode;

			// False branch, may be null.
			Auto<Expr> falseCode;

			// Set true/false code.
			virtual void STORM_FN trueExpr(Par<Expr> e);
			virtual void STORM_FN falseExpr(Par<Expr> e);

			// Result.
			virtual ExprResult STORM_FN result();

			// Code.
			virtual void STORM_FN blockCode(Par<CodeGen> state, Par<CodeResult> r);

		protected:
			// Output.
			virtual void output(wostream &to) const;

		private:
			// Overridden variable.
			Auto<LocalVar> created;
		};


		/**
		 * If-statement using a weak cast as its condition.
		 */
		class IfWeak : public Block {
			STORM_CLASS;
		public:
			STORM_CTOR IfWeak(Par<Block> parent, Par<WeakCast> cast);
			STORM_CTOR IfWeak(Par<Block> parent, Par<WeakCast> cast, Par<SStr> name);

			// Weak cast to execute.
			Auto<WeakCast> weakCast;

			// True branch.
			Auto<IfTrue> trueCode;

			// False branch, may be null.
			Auto<Expr> falseCode;

			// Set variables from the parser.
			void STORM_FN trueExpr(Par<IfTrue> e);
			void STORM_FN falseExpr(Par<Expr> e);

			// Result.
			virtual ExprResult STORM_FN result();

			// Code.
			virtual void STORM_FN blockCode(Par<CodeGen> state, Par<CodeResult> r);

			// Get the local variable we're overwriting (if any).
			MAYBE(LocalVar) *overwrite();

		protected:
			// Output.
			virtual void output(wostream &to) const;

		private:
			// Overriden variable (lazily created).
			Auto<LocalVar> created;

			// Custom variable name?
			Auto<SStr> varName;
		};


		// Create an if-statement with a single expression.
		Expr *STORM_FN createIf(Par<Block> block, Par<Expr> expr);

		/**
		 * Block which knows about IfWeak's variable!
		 */
		class IfTrue : public Block {
			STORM_CLASS;
		public:
			STORM_CTOR IfTrue(Par<Block> parent);

			// Single contained expression.
			Auto<Expr> expr;
			void STORM_FN set(Par<Expr> expr);

			// Generate code.
			virtual ExprResult STORM_FN result();
			virtual void STORM_FN blockCode(Par<CodeGen> state, Par<CodeResult> to);

		protected:
			virtual void output(wostream &to) const;
		};

	}
}
