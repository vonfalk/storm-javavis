#pragma once
#include "BSExpr.h"
#include "BSVar.h"
#include "Scope.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		class LocalVar;
		class BlockLookup;

		/**
		 * A basic block. This block only acts as a new scope for variables. It
		 * is abstract to let other types of expressions act as some kind of block
		 * for variables.
		 */
		class Block : public Expr {
			STORM_CLASS;
		public:
			STORM_CTOR Block(const Scope &scope);
			STORM_CTOR Block(Par<Block> parent);

			// Lookup node.
			STORM_VAR Auto<BlockLookup> lookup;

			// Scope.
			STORM_VAR Scope scope;

			// Generate code. Override 'blockCode' to generate only block contents.
			virtual void STORM_FN code(Par<CodeGen> state, Par<CodeResult> to);

			// Override to initialize the block yourself.
			virtual void blockCode(Par<CodeGen> state, Par<CodeResult> to, const code::Block &newBlock);

			// Override to generate contents of the block.
			virtual void STORM_FN blockCode(Par<CodeGen> state, Par<CodeResult> to);

			// Find a variable. Same semantics as 'find'.
			MAYBE(LocalVar) *variable(const String &name);

			// Add a variable
			virtual void STORM_FN add(Par<LocalVar> v);

			// Lift all variables present inside 'o' into this block. Can only be used one step at a
			// time to not cause strange scoping issues.
			virtual void STORM_FN liftVars(Par<Block> from);

		private:
			// Variables in this block.
			typedef hash_map<String, Auto<LocalVar> > VarMap;
			VarMap variables;

			// Check if 'x' is a child to us.
			bool isParentTo(Par<Block> x);

		};

		/**
		 * A block that contains statements.
		 */
		class ExprBlock : public Block {
			STORM_CLASS;
		public:
			ExprBlock(const Scope &scope);
			STORM_CTOR ExprBlock(Par<Block> parent);

			// Expressions here.
			vector<Auto<Expr> > exprs;

			// Add an expression.
			using Block::add;
			void STORM_FN add(Par<Expr> s);

			// Result.
			virtual ExprResult STORM_FN result();

			// Optimization.
			virtual void STORM_FN code(Par<CodeGen> state, Par<CodeResult> to);

			// Code generation.
			virtual void STORM_FN blockCode(Par<CodeGen> state, Par<CodeResult> to);

			// Auto-casting should work across the boundaries of an expression.
			virtual Int STORM_FN castPenalty(Value to);

		private:
			virtual void output(wostream &to) const;
		};


		/**
		 * Node in the name lookup tree for our blocks.
		 */
		class BlockLookup : public NameLookup {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR BlockLookup(Par<Block> block, NameLookup *prev);

			// Risk of cycles, no ref.
			Block *block;

			// Find a variable here.
			virtual MAYBE(Named) *STORM_FN find(Par<NamePart> part);
		};

	}
}
