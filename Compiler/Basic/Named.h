#pragma once
#include "Expr.h"
#include "Var.h"
#include "Actuals.h"
#include "Compiler/Function.h"
#include "Compiler/NamedThread.h"
#include "Variable.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		/**
		 * Execute a function.
		 */
		class FnCall : public Expr {
			STORM_CLASS;
		public:
			// if '!lookup', then the lookup functionality (such as vtables) will not be used.
			// if 'sameObject', then we will not spawn other threads for this function call, as it
			// is assumed that we are always calling the same object.
			STORM_CTOR FnCall(SrcPos pos, Scope scope, Function *toExecute, Actuals *params);
			STORM_CTOR FnCall(SrcPos pos, Scope scope, Function *toExecute, Actuals *params, Bool lookup, Bool sameObject);

			// Tell us to return a future instead.
			void makeAsync();

			// Result type.
			virtual ExprResult STORM_FN result();

			// Generate code.
			virtual void STORM_FN code(CodeGen *s, CodeResult *to);

			// To string.
			virtual void STORM_FN toS(StrBuf *to) const;

		private:
			// Function to run.
			Function *toExecute;

			// Parameters.
			Actuals *params;

			// Scope this function is called within.
			Scope scope;

			// Use lookup?
			bool lookup;

			// Same object?
			bool sameObject;

			// Async function call?
			bool async;
		};


		/**
		 * Execute a constructor.
		 */
		class CtorCall : public Expr {
			STORM_CLASS;
		public:
			// Call a constructor.
			STORM_CTOR CtorCall(SrcPos pos, Scope scope, Function *ctor, Actuals *params);

			// Result type.
			virtual ExprResult STORM_FN result();

			// Generate code.
			virtual void STORM_FN code(CodeGen *s, CodeResult *to);

			// To string.
			virtual void STORM_FN toS(StrBuf *to) const;

		private:
			// Type to create.
			Value toCreate;

			// Constructor to run.
			Function *ctor;

			// Parameters.
			Actuals *params;

			// Scope we're being executed in.
			Scope scope;

			// Create a value.
			void createValue(CodeGen *s, CodeResult *to);

			// Create a class.
			void createClass(CodeGen *s, CodeResult *to);
		};

		// Call the default constructor for T.
		CtorCall *STORM_FN defaultCtor(const SrcPos &pos, Scope scope, Type *t);

		// Call the copy-constructor for T.
		CtorCall *STORM_FN copyCtor(const SrcPos &pos, Scope scope, Type *t, Expr *src);

		/**
		 * Get a local variable.
		 */
		class LocalVarAccess : public Expr {
			STORM_CLASS;
		public:
			STORM_CTOR LocalVarAccess(SrcPos pos, LocalVar *var);

			// Result type.
			virtual ExprResult STORM_FN result();

			// Generate code.
			virtual void STORM_FN code(CodeGen *s, CodeResult *to);

			// Variable to access.
			LocalVar *var;

			// To string.
			virtual void STORM_FN toS(StrBuf *to) const;
		};

		/**
		 * Get a local variable (bare-bone wrapper for interfacing with other languages).
		 */
		class BareVarAccess : public Expr {
			STORM_CLASS;
		public:
			STORM_CTOR BareVarAccess(SrcPos pos, Value type, code::Var var);

			// Result.
			virtual ExprResult STORM_FN result();

			// Generate code.
			virtual void STORM_FN code(CodeGen *s, CodeResult *to);

			// Type and variable.
			Value type;
			code::Var var;

			// To string.
			virtual void STORM_FN toS(StrBuf *to) const;
		};


		/**
		 * Read a type variable.
		 */
		class MemberVarAccess : public Expr {
			STORM_CLASS;
		public:
			STORM_CTOR MemberVarAccess(SrcPos pos, Expr *member, MemberVar *var);

			// Result type.
			virtual ExprResult STORM_FN result();

			// Generate code.
			virtual void STORM_FN code(CodeGen *s, CodeResult *to);

			// Variable to access.
			MemberVar *var;

			// Output.
			virtual void STORM_FN toS(StrBuf *to) const;

		private:
			// Member in which the variable is.
			Expr *member;

			// Generate code for a value access.
			void valueCode(CodeGen *s, CodeResult *to);

			// Generate code for a class access.
			void classCode(CodeGen *s, CodeResult *to);

			// Extract the value itself from ptrA
			void extractCode(CodeGen *s, CodeResult *to);
		};

		/**
		 * Read a named thread variable.
		 */
		class NamedThreadAccess : public Expr {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR NamedThreadAccess(SrcPos pos, NamedThread *thread);

			// Result type.
			virtual ExprResult STORM_FN result();

			// Generate code.
			virtual void STORM_FN code(CodeGen *s, CodeResult *to);

			// Output.
			virtual void STORM_FN toS(StrBuf *to) const;

		private:
			// Which thread?
			NamedThread *thread;
		};


		/**
		 * Default assignment operator.
		 */
		class ClassAssign : public Expr {
			STORM_CLASS;
		public:
			STORM_CTOR ClassAssign(Expr *to, Expr *value, Scope scope);

			// Result type.
			virtual ExprResult STORM_FN result();

			// Generate code.
			virtual void STORM_FN code(CodeGen *s, CodeResult *to);

			// Output.
			virtual void STORM_FN toS(StrBuf *to) const;

		private:
			Expr *to;
			Expr *value;
		};


		// Find out what the named expression means, and create proper object.
		Expr *STORM_FN namedExpr(Block *block, syntax::SStr *name, Actuals *params);
		Expr *STORM_FN namedExpr(Block *block, SrcName *name, Actuals *params);
		Expr *STORM_FN namedExpr(Block *block, SrcPos pos, Name *name, Actuals *params);

		// Special case of above, used when we find an expression like a.b(...). 'first' is inserted
		// into the beginning of 'params' and used. This method inhibits automatic insertion of 'this'.
		Expr *STORM_FN namedExpr(Block *block, syntax::SStr *name, Expr *first, Actuals *params);
		Expr *STORM_FN namedExpr(Block *block, syntax::SStr *name, Expr *first);

		// Make a regular function call return a future to the result instead of waiting for the result
		// directly. Syntactically, this is made by adding 'spawn' to it.
		Expr *STORM_FN spawnExpr(Expr *expr);

	}
}
