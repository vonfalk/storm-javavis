#pragma once
#include "Block.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		class CatchBlock;

		/**
		 * A try-catch block.
		 *
		 * The TryBlock instance itself contains the expressions protected by the block, and
		 * additional catch blocks are added as separate catch blocks.
		 *
		 * Note that the catch blocks are to reside outside the try-block, and not inside. I.e. when
		 * creating expressions or ExprBlock instances to put inside the try block, don't pass the
		 * TryBlock as the parent, but rather the TryBlock's parent. Otherwise, scoping will be wrong.
		 */
		class TryBlock : public ExprBlock {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR TryBlock(SrcPos pos, Block *parent);

			// Add a catch block. Catch blocks are searched in the order they were added for potential matches.
			void STORM_FN addCatch(CatchBlock *block);

			// Generate code.
			virtual void STORM_FN code(CodeGen *state, CodeResult *to);

			// Create the block.
			virtual void STORM_FN blockCode(CodeGen *state, CodeResult *to, code::Block block);

			// Make sure the return type is correct.
			virtual void STORM_FN blockCode(CodeGen *state, CodeResult *to);

			// Compute the result.
			virtual ExprResult STORM_FN result();

		protected:
			// Output.
			virtual void STORM_FN toS(StrBuf *to) const;

		private:
			// All catch blocks.
			Array<CatchBlock *> *toCatch;
		};

		/**
		 * A catch-block.
		 *
		 * As mentioned for the TryBlock class, this block is considered to reside outside the try
		 * block. Thus, don't pass the TryBlock as parameter to the constructor, but rather the
		 * TryBlock's parent.
		 */
		class CatchBlock : public Block {
			STORM_CLASS;
		public:
			// Create. Specify the type and optionally the name of the exception to catch.
			STORM_CTOR CatchBlock(SrcPos pos, Block *parent, SrcName *type, MAYBE(syntax::SStr *) name);

			// Type we're catching.
			Type *type;

			// Set contained expression (only one).
			void STORM_ASSIGN expr(Expr *expr);

			// Generate block contents.
			void STORM_FN blockCode(CodeGen *state, CodeResult *to);

			// Result.
			ExprResult STORM_FN result();

			// Tell the catch block where the exception is stored.
			code::Var exceptionVar;

		protected:
			// Output.
			virtual void STORM_FN toS(StrBuf *to) const;

		private:
			// Contained expression.
			MAYBE(Expr *) run;

			// Variable to initialize, if any. Initialized by 'TryBlock'.
			MAYBE(LocalVar *) var;
		};

	}
}
