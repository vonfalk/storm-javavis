#pragma once
#include "BSExpr.h"
#include "BSVar.h"
#include "BSActual.h"
#include "Function.h"
#include "NamedThread.h"
#include "TypeVar.h"

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
			STORM_CTOR FnCall(SrcPos pos, Par<Function> toExecute, Par<Actual> params);
			STORM_CTOR FnCall(SrcPos pos, Par<Function> toExecute, Par<Actual> params, Bool lookup, Bool sameObject);

			// Tell us to return a future instead.
			void makeAsync();

			// Result type.
			virtual ExprResult STORM_FN result();

			// Generate code.
			virtual void STORM_FN code(Par<CodeGen> s, Par<CodeResult> to);

		protected:
			virtual void output(wostream &to) const;

		private:
			// Function to run.
			Auto<Function> toExecute;

			// Parameters.
			Auto<Actual> params;

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
			STORM_CTOR CtorCall(SrcPos pos, Par<Function> ctor, Par<Actual> params);

			// Result type.
			virtual ExprResult STORM_FN result();

			// Generate code.
			virtual void STORM_FN code(Par<CodeGen> s, Par<CodeResult> to);

		protected:
			virtual void output(wostream &to) const;

		private:
			// Type to create.
			Value toCreate;

			// Constructor to run.
			Auto<Function> ctor;

			// Parameters.
			Auto<Actual> params;

			// Create a value.
			void createValue(Par<CodeGen> s, Par<CodeResult> to);

			// Create a class.
			void createClass(Par<CodeGen> s, Par<CodeResult> to);
		};

		// Call the default constructor for T.
		CtorCall *STORM_FN defaultCtor(const SrcPos &pos, Par<Type> t);

		// Call the copy-constructor for T.
		CtorCall *STORM_FN copyCtor(const SrcPos &pos, Par<Type> t, Par<Expr> src);

		/**
		 * Get a local variable.
		 */
		class LocalVarAccess : public Expr {
			STORM_CLASS;
		public:
			STORM_CTOR LocalVarAccess(SrcPos pos, Par<LocalVar> var);

			// Result type.
			virtual ExprResult STORM_FN result();

			// Generate code.
			virtual void STORM_FN code(Par<CodeGen> s, Par<CodeResult> to);

			// Variable to access.
			Auto<LocalVar> var;

		protected:
			virtual void output(wostream &to) const;
		};

		/**
		 * Get a local variable (bare-bone wrapper for interfacing with other languages).
		 */
		class BareVarAccess : public Expr {
			STORM_CLASS;
		public:
			STORM_CTOR BareVarAccess(SrcPos pos, Value type, wrap::Variable var);

			// Result.
			virtual ExprResult STORM_FN result();

			// Generate code.
			virtual void STORM_FN code(Par<CodeGen> s, Par<CodeResult> to);

			// Type and variable.
			STORM_VAR Value type;
			STORM_VAR wrap::Variable var;

		protected:
			virtual void output(wostream &to) const;
		};


		/**
		 * Read a type variable.
		 */
		class MemberVarAccess : public Expr {
			STORM_CLASS;
		public:
			STORM_CTOR MemberVarAccess(SrcPos pos, Par<Expr> member, Par<TypeVar> var);

			// Result type.
			virtual ExprResult STORM_FN result();

			// Generate code.
			virtual void STORM_FN code(Par<CodeGen> s, Par<CodeResult> to);

			// Variable to access.
			Auto<TypeVar> var;

		protected:
			// Output.
			virtual void output(wostream &to) const;

		private:
			// Member in which the variable is.
			Auto<Expr> member;

			// Generate code for a value access.
			void valueCode(Par<CodeGen> s, Par<CodeResult> to);

			// Generate code for a class access.
			void classCode(Par<CodeGen> s, Par<CodeResult> to);

			// Extract the value itself from ptrA
			void extractCode(Par<CodeGen> s, Par<CodeResult> to);
		};

		/**
		 * Read a named thread variable.
		 */
		class NamedThreadAccess : public Expr {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR NamedThreadAccess(SrcPos pos, Par<NamedThread> thread);

			// Result type.
			virtual ExprResult STORM_FN result();

			// Generate code.
			virtual void STORM_FN code(Par<CodeGen> s, Par<CodeResult> to);

		private:
			// Which thread?
			Auto<NamedThread> thread;
		};


		/**
		 * Default assignment operator.
		 */
		class ClassAssign : public Expr {
			STORM_CLASS;
		public:
			STORM_CTOR ClassAssign(Par<Expr> to, Par<Expr> value);

			// Result type.
			virtual ExprResult STORM_FN result();

			// Generate code.
			virtual void STORM_FN code(Par<CodeGen> s, Par<CodeResult> to);

		private:
			Auto<Expr> to, value;
		};


		// Find out what the named expression means, and create proper object.
		Expr *STORM_FN namedExpr(Par<Block> block, Par<SStr> name, Par<Actual> params);
		Expr *STORM_FN namedExpr(Par<Block> block, Par<SrcName> name, Par<Actual> params);
		Expr *STORM_FN namedExpr(Par<Block> block, SrcPos pos, Par<Name> name, Par<Actual> params);

		// Special case of above, used when we find an expression like a.b(...). 'first' is inserted
		// into the beginning of 'params' and used. This method inhibits automatic insertion of 'this'.
		Expr *STORM_FN namedExpr(Par<Block> block, Par<SStr> name, Par<Expr> first, Par<Actual> params);
		Expr *STORM_FN namedExpr(Par<Block> block, Par<SStr> name, Par<Expr> first);

		// Make a regular function call return a future to the result instead of waiting for the result
		// directly. Syntactically, this is made by adding 'spawn' to it.
		Expr *STORM_FN spawnExpr(Par<Expr> expr);

	}
}