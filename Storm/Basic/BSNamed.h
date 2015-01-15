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
		 * Execute something named (eg a variable, a function).
		 */
		class NamedExpr : public Expr {
			STORM_CLASS;
		public:
			// Try to call 'name' with 'params'.
			STORM_CTOR NamedExpr(Auto<Block> block, Auto<SStr> name, Auto<Actual> params);

			// Try to call 'name' with 'first' as the first parameter of 'params'. This is
			// used when we find an expression like: a.b(...). Then a is 'first'.
			STORM_CTOR NamedExpr(Auto<Block> block, Auto<SStr> name, Auto<Expr> first, Auto<Actual> params);

			// The return type of this 'NamedExpr'.
			virtual Value result();

			// Generate code.
			virtual void code(const GenState &s, GenResult &to);

		private:
			// The function to execute (if any).
			Function *toExecute;

			// The variable to load (if any).
			LocalVar *toLoad;

			// Member variable to load (if any).
			TypeVar *toAccess;

			// The type to create (if any). Constructor saved in 'toExecute'.
			Value toCreate;

			// Parameters
			Auto<Actual> params;

			// Generate code to call a function.
			void callCode(const GenState &s, GenResult &to);

			// Generate code to load a variable.
			void loadCode(const GenState &s, GenResult &to);

			// Generate code to create a type.
			void createCode(const GenState &s, GenResult &to);

			// Generate code to access a member variable.
			void accessCode(const GenState &s, GenResult &to);

			// Find what to call.
			void findTarget(Auto<Block> block, const Name &name, const SrcPos &pos, bool useThis);

			// Given a 'named', find out what to call.
			bool findTarget(Named *n, const SrcPos &pos);

			// Find what to call. Assumes we're trying to use the 'this' pointer.
			bool findTargetThis(Auto<Block> block, const Name &name, const SrcPos &pos, Named *&candidate);

			// Find a constructor.
			void findCtor(Type *t, const SrcPos &pos);
		};


		NamedExpr * STORM_FN Operator(Auto<Block> block, Auto<Expr> lhs, Auto<SStr> m, Auto<Expr> rhs);

	}
}
