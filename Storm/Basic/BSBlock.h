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
			STORM_CTOR Block(Auto<Block> parent);

			// Lookup node.
			Auto<BlockLookup> lookup;

			// Scope.
			Scope scope;

			// Generate code. Override 'blockCode' to generate only block contents.
			virtual void code(const GenState &state, GenResult &to);

			// Override to generate contents of the block.
			virtual void blockCode(const GenState &state, GenResult &to);

			// Find a variable. Same semantics as 'find'.
			LocalVar *variable(const String &name);

			// Add a variable
			void add(Auto<LocalVar> v);

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
			STORM_CTOR ExprBlock(Auto<Block> parent);

			// Expressions here.
			vector<Auto<Expr> > exprs;

			// Add an expression.
			void STORM_FN expr(Auto<Expr> s);

			// Result.
			virtual Value result();

			// Optimization.
			virtual void code(const GenState &state, GenResult &to);

			// Code generation.
			virtual void blockCode(const GenState &state, GenResult &to);
		};


		/**
		 * Node in the name lookup tree for our blocks.
		 */
		class BlockLookup : public NameLookup {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR BlockLookup(Auto<Block> block, NameLookup *prev);

			// Risk of cycles, no ref.
			Block *block;

			// Previous lookup (risk of cycles).
			NameLookup *prev;

			// Find a variable here.
			virtual Named *find(const Name &name);

			// Parent.
			virtual NameLookup *parent() const;
		};

	}
}
