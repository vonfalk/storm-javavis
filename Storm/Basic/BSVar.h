#pragma once
#include "BSExpr.h"
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
			STORM_CTOR LocalVar(Par<Str> name, const Value &val, const SrcPos &pos, Bool param);

			// Type.
			STORM_VAR Value result;

			// Declared at
			STORM_VAR SrcPos pos;

			// Location (not initialized until code generation).
			STORM_VAR VarInfo var;

			// Is this a parameter?
			STORM_VAR Bool param;

			// Constant variable? This is currently only used for the 'this' pointer.
			STORM_VAR Bool constant;

			// Create the variable.
			void STORM_FN create(Par<CodeGen> state);

			// Create the parameter.
			void STORM_FN createParam(Par<CodeGen> state);

		};

		class Block;

		/**
		 * Local variable declaration.
		 */
		class Var : public Expr {
			STORM_CLASS;
		public:
			// Initialize with initializer-list (like Foo(1))
			STORM_CTOR Var(Par<Block> block, Par<SrcName> type, Par<SStr> name, Par<Actual> actual);
			STORM_CTOR Var(Par<Block> block, Value type, Par<SStr> name, Par<Actual> actual);

			// Initialize to an expression.
			STORM_CTOR Var(Par<Block> block, Par<SrcName> type, Par<SStr> name, Par<Expr> init);
			STORM_CTOR Var(Par<Block> block, Value type, Par<SStr> name, Par<Expr> init);

			// Initialize to an expression (auto type).
			STORM_CTOR Var(Par<Block> block, Par<SStr> name, Par<Expr> init);

			// Declared variable.
			Auto<LocalVar> variable;

			// Get the declared variable.
			LocalVar *STORM_FN var();

			// Result type.
			virtual ExprResult STORM_FN result();

			// Generate code.
			virtual void STORM_FN code(Par<CodeGen> state, Par<CodeResult> to);

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
