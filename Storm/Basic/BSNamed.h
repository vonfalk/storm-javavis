#pragma once
#include "BSExpr.h"
#include "BSVar.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		/**
		 * Actual parameters to a function.
		 */
		class Actual : public Object {
			STORM_CLASS;
		public:
			STORM_CTOR Actual();

			// Parameters.
			vector<Auto<Expr> > expressions;

			// Compute all types.
			vector<Value> values();

			// Add a parameter.
			void STORM_FN add(Par<Expr> expr);

			// Add a parameter to the beginning.
			void STORM_FN addFirst(Par<Expr> expr);
		};


		/**
		 * Execute a function.
		 */
		class FnCall : public Expr {
			STORM_CLASS;
		public:
			STORM_CTOR FnCall(Par<Function> toExecute, Par<Actual> params);

			// Result type.
			virtual Value result();

			// Generate code.
			virtual void code(const GenState &s, GenResult &to);

		private:
			// Function to run.
			Auto<Function> toExecute;

			// Parameters.
			Auto<Actual> params;
		};


		/**
		 * Execute a constructor.
		 */
		class CtorCall : public Expr {
			STORM_CLASS;
		public:
			STORM_CTOR CtorCall(Par<Function> ctor, Par<Actual> params);

			// Result type.
			virtual Value result();

			// Generate code.
			virtual void code(const GenState &s, GenResult &to);

		private:
			// Type to create.
			Value toCreate;

			// Constructor to run.
			Auto<Function> ctor;

			// Parameters.
			Auto<Actual> params;

			// Create a value.
			void createValue(const GenState &s, GenResult &to);

			// Create a class.
			void createClass(const GenState &s, GenResult &to);
		};


		/**
		 * Get a local variable.
		 */
		class LocalVarAccess : public Expr {
			STORM_CLASS;
		public:
			STORM_CTOR LocalVarAccess(Par<LocalVar> var);

			// Result type.
			virtual Value result();

			// Generate code.
			virtual void code(const GenState &s, GenResult &to);

		private:
			// Variable to access.
			Auto<LocalVar> var;
		};


		/**
		 * Read a type variable.
		 */
		class MemberVarAccess : public Expr {
			STORM_CLASS;
		public:
			STORM_CTOR MemberVarAccess(Par<Expr> member, Par<TypeVar> var);

			// Result type.
			virtual Value result();

			// Generate code.
			virtual void code(const GenState &s, GenResult &to);

		private:
			// Member in which the variable is.
			Auto<Expr> member;

			// Variable to access.
			Auto<TypeVar> var;
		};


		/**
		 * Default assignment operator.
		 */
		class ClassAssign : public Expr {
			STORM_CLASS;
		public:
			STORM_CTOR ClassAssign(Par<Expr> to, Par<Expr> value);

			// Result type.
			virtual Value result();

			// Generate code.
			virtual void code(const GenState &s, GenResult &to);

		private:
			Auto<Expr> to, value;
		};


		// Find out what the named expression means, and create proper object.
		Expr *STORM_FN namedExpr(Par<Block> block, Par<SStr> name, Par<Actual> params);

		// Special case of above, used when we find an expression like a.b(...). 'first' is inserted
		// into the beginning of 'params' and used. This method inhibits automatic insertion of 'this'.
		Expr *STORM_FN namedExpr(Par<Block> block, Par<SStr> name, Par<Expr> first, Par<Actual> params);

		// Create an operator.
		Expr *STORM_FN operatorExpr(Par<Block> block, Par<Expr> lhs, Par<SStr> m, Par<Expr> rhs);

		// Assignment operator.
		Expr *STORM_FN assignExpr(Par<Block> block, Par<Expr> lhs, Par<SStr> m, Par<Expr> rhs);

	}
}
