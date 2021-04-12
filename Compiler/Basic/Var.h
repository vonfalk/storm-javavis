#pragma once
#include "Compiler/Name.h"
#include "Compiler/Named.h"
#include "Compiler/CodeGen.h"
#include "Compiler/Syntax/SStr.h"
#include "Actuals.h"
#include "Param.h"

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

			// Is this a "true" this-variable?
			virtual Bool STORM_FN thisVariable();

		private:
			// Create debug information.
			void addInfo(code::Listing *l, code::Var var);
		};

		/**
		 * Specialization for the "this"-variable that is automatically declared inside member
		 * functions. This is so that other parts may find it and make assumptions wrt. threading etc.
		 */
		class ThisVar : public LocalVar {
			STORM_CLASS;
		public:
			// Create with default name (i.e. this).
			STORM_CTOR ThisVar(Value val, SrcPos pos, Bool param);

			// Custom name. We won't find it automatically if it is not named "this", but we can
			// still identify it as it is an instance of this class.
			STORM_CTOR ThisVar(Str *name, Value val, SrcPos pos, Bool param);

			// Is this a "true" this-variable? Yes!
			virtual Bool STORM_FN thisVariable();
		};

		// Helper to create a parameter from a ValParam class.
		LocalVar *STORM_FN createParam(EnginePtr e, ValParam param, SrcPos pos);

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

			// Better location.
			virtual SrcPos STORM_FN largePos();

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
