#pragma once
#include "BSExpr.h"
#include "BSVar.h"

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
			Block(const Scope &scope); // todo: make STORM_CTOR
			STORM_CTOR Block(Par<Block> parent);

			// Lookup node.
			Auto<BlockLookup> lookup;

			// Scope.
			Scope scope;

			// Generate code. Override 'blockCode' to generate only block contents.
			virtual void code(const GenState &state, GenResult &to);

			// Override to initialize the block yourself.
			virtual void blockCode(const GenState &state, GenResult &to, const code::Block &newBlock);

			// Override to generate contents of the block.
			virtual void blockCode(const GenState &state, GenResult &to);

			// Find a variable. Same semantics as 'find'.
			LocalVar *variable(const String &name);

			// Add a variable
			void add(Par<LocalVar> v);

		private:
			// Variables in this block.
			typedef hash_map<String, Auto<LocalVar> > VarMap;
			VarMap variables;

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
			void STORM_FN add(Par<Expr> s);

			// Result.
			virtual Value result();

			// Optimization.
			virtual void code(const GenState &state, GenResult &to);

			// Code generation.
			virtual void blockCode(const GenState &state, GenResult &to);

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

		protected:
			// Find a variable here.
			virtual Named *findHere(const String &name, const vector<Value> &params);
		};

	}
}
