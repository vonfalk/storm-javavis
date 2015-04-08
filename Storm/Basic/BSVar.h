#pragma once
#include "BSExpr.h"
#include "BSType.h"
#include "BSActual.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		/**
		 * Local variable.
		 */
		class LocalVar : public Named {
			STORM_CLASS;
		public:
			LocalVar(const String &name, const Value &val, const SrcPos &pos, bool param = false);

			// Type.
			Value result;

			// Declared at
			SrcPos pos;

			// Location (not initialized until code generation).
			VarInfo var;

			// Is this a parameter?
			bool param;

			// Constant variable? This is currently only used for the 'this' pointer.
			bool constant;

			// Create the variable.
			void create(Par<CodeGen> state);

			// Create the parameter.
			void createParam(Par<CodeGen> state);

		};

		class Block;

		/**
		 * Local variable delcaration.
		 */
		class Var : public Expr {
			STORM_CLASS;
		public:
			// Initialize with initializer-list (like Foo(1))
			STORM_CTOR Var(Par<Block> block, Par<TypeName> type, Par<SStr> name, Par<Actual> actual);

			// Initialize to an expression.
			STORM_CTOR Var(Par<Block> block, Par<TypeName> type, Par<SStr> name, Par<Expr> init);

			// Initialize to an expression (auto type).
			STORM_CTOR Var(Par<Block> block, Par<SStr> name, Par<Expr> init);

			// Declared variable.
			Auto<LocalVar> variable;

			// Get the declared variable.
			LocalVar *STORM_FN var();

			// Result type.
			virtual Value result();

			// Generate code.
			virtual void code(Par<CodeGen> state, Par<CodeResult> to);

		protected:
			virtual void output(wostream &to) const;

		private:
			// Initialize.
			void init(Par<Block> block, const Value &type, Par<SStr> name);

			// Set return value.
			void initTo(Par<Expr> expr);

			// Initialize ctor call.
			void initTo(Par<Actual> actual);

			// Initialize to.
			Auto<Expr> initExpr;

			// Initialize using a constructor directly. When dealing with value types, this
			// will create the value in-place instead of copying the value to its location.
			// When dealing with reference types, it does not matter.
			Auto<Expr> initCtor;
		};

	}
}

#include "BSBlock.h"
