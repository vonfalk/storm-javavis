#pragma once
#include "Compiler/Name.h"
#include "Compiler/Named.h"
#include "Compiler/CodeGen.h"
#include "Compiler/Syntax/SStr.h"
#include "Actuals.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		/**
		 * Local variable.
		 */
		class LocalVar : public Named {
			STORM_CLASS;
		public:
			STORM_CTOR LocalVar(Str *name, Value val, SrcPos pos, Bool param);

			// Type.
			Value result;

			// Location (not initialized until code generation).
			VarInfo var;

			// Is this a parameter?
			Bool param;

			// Constant variable? Currently only used for the 'this' pointer.
			Bool constant;

			// Create the variable.
			void STORM_FN create(CodeGen *state);

			// Create the parameter.
			void STORM_FN createParam(CodeGen *state);

		private:
			// Create debug information.
			void addInfo(code::Listing *l, code::Var var);
		};

		class Block;

		/**
		 * Local variable declaration.
		 */
		class Var : public Expr {
			STORM_CLASS;
		public:
			// Initialize with initializer-list (like Foo(1))
			STORM_CTOR Var(Block *block, SrcName *type, syntax::SStr *name, Actuals *actual);
			STORM_CTOR Var(Block *block, Value type, syntax::SStr *name, Actuals *actual);

			// Initialize to an expression. We don't allow calls to non-cast-ctors in this case!
			STORM_CTOR Var(Block *block, SrcName *type, syntax::SStr *name, Expr *init);
			STORM_CTOR Var(Block *block, Value type, syntax::SStr *name, Expr *init);

			// Initialize to an expression (auto type).
			STORM_CTOR Var(Block *block, syntax::SStr *name, Expr *init);

			// Declared variable.
			LocalVar *var;

			// Result type.
			virtual ExprResult STORM_FN result();

			// Generate code.
			virtual void STORM_FN code(CodeGen *state, CodeResult *to);

		protected:
			virtual void STORM_FN toS(StrBuf *to) const;

		private:
			// Initialize.
			void init(Block *block, const Value &type, syntax::SStr *name);

			// Set return value.
			void initTo(Expr *expr);

			// Initialize ctor call. If 'castOnly' is true, then we only allow copy-ctors and
			// cast-ctors to be called.
			void initTo(Actuals *actual, Bool castOnly);

			// Initialize to.
			Expr *initExpr;

			// Initialize using a constructor directly. When dealing with value types, this
			// will create the value in-place instead of copying the value to its location.
			// When dealing with reference types, it does not matter.
			Expr *initCtor;

			// Scope.
			Scope scope;
		};

	}
}
