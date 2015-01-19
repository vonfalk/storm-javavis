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
			void STORM_FN add(Auto<Expr> expr);

			// Add a parameter to the beginning.
			void STORM_FN addFirst(Auto<Expr> expr);
		};


		/**
		 * Execute a function.
		 */
		class FnCall : public Expr {
			STORM_CLASS;
		public:
			STORM_CTOR FnCall(Auto<Function> toExecute, Auto<Actual> params);

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
			STORM_CTOR CtorCall(Auto<Function> ctor, Auto<Actual> params);

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
		};


		/**
		 * Get a local variable.
		 */
		class LocalVarAccess : public Expr {
			STORM_CLASS;
		public:
			STORM_CTOR LocalVarAccess(Auto<LocalVar> var);

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
			STORM_CTOR MemberVarAccess(Auto<Expr> member, Auto<TypeVar> var);

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


		// Find out what the named expression means, and create proper object.
		Expr *STORM_FN namedExpr(Auto<Block> block, Auto<SStr> name, Auto<Actual> params);

		// Special case of above, used when we find an expression like a.b(...). 'first' is inserted
		// into the beginning of 'params' and used. This method inhibits automatic insertion of 'this'.
		Expr *STORM_FN namedExpr(Auto<Block> block, Auto<SStr> name, Auto<Expr> first, Auto<Actual> params);

		// Create an operator.
		Expr *STORM_FN operatorExpr(Auto<Block> block, Auto<Expr> lhs, Auto<SStr> m, Auto<Expr> rhs);

	}
}
