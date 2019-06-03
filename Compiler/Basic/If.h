#pragma once
#include "Block.h"
#include "WeakCast.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		class IfTrue;

		/**
		 * Basic if-statement.
		 */
		class IfExpr : public Block {
			STORM_CLASS;
		public:
			STORM_CTOR IfExpr(SrcPos pos, Block *parent);

			// True branch.
			MAYBE(IfTrue *) trueCode;

			// False branch, may be null.
			MAYBE(Expr *) falseCode;

			// Set variables from the parser.
			void STORM_FN trueExpr(IfTrue *e);
			void STORM_FN falseExpr(Expr *e);
		};

		/**
		 * If-statement.
		 */
		class If : public IfExpr {
			STORM_CLASS;
		public:
			STORM_CTOR If(Block *parent, Expr *cond);

			// Condition expression
			Expr *condition;

			// Result.
			virtual ExprResult STORM_FN result();

			// Code.
			virtual void STORM_FN blockCode(CodeGen *state, CodeResult *r);

		protected:
			// Output.
			virtual void STORM_FN toS(StrBuf *to) const;

		private:
			// Overridden variable.
			LocalVar *created;
		};


		/**
		 * If-statement using a weak cast as its condition.
		 */
		class IfWeak : public IfExpr {
			STORM_CLASS;
		public:
			STORM_CTOR IfWeak(Block *parent, WeakCast *cast);
			STORM_CTOR IfWeak(Block *parent, WeakCast *cast, syntax::SStr *name);

			// Weak cast to execute.
			WeakCast *weakCast;

			// Result.
			virtual ExprResult STORM_FN result();

			// Code.
			virtual void STORM_FN blockCode(CodeGen *state, CodeResult *r);

			// Get the local variable we're overwriting (if any).
			MAYBE(LocalVar *) overwrite();

		protected:
			// Output.
			virtual void STORM_FN toS(StrBuf *to) const;

		private:
			// Overriden variable (lazily created).
			LocalVar *created;

			// Custom variable name?
			syntax::SStr *varName;
		};


		// Create an if-statement with a single expression.
		IfExpr *STORM_FN createIf(Block *block, Expr *expr);

		/**
		 * Block which knows about IfWeak's variable!
		 */
		class IfTrue : public Block {
			STORM_CLASS;
		public:
			STORM_CTOR IfTrue(SrcPos pos, Block *parent);

			// Single contained expression.
			MAYBE(Expr *) expr;
			void STORM_FN set(Expr *expr);

			// Generate code.
			virtual ExprResult STORM_FN result();
			virtual void STORM_FN blockCode(CodeGen *state, CodeResult *to);

		protected:
			virtual void STORM_FN toS(StrBuf *to) const;
		};

	}
}
