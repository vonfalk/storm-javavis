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
		};

		/**
		 * Execute something named (eg a variable, a function).
		 */
		class NamedExpr : public Expr {
			STORM_CLASS;
		public:
			STORM_CTOR NamedExpr(Auto<Block> block, Auto<SStr> name, Auto<Actual> params);

			// The return type of this 'NamedExpr'.
			virtual Value result();

			// Generate code.
			virtual void code(const GenState &s, code::Variable to);

		private:
			// The function to execute (if any).
			Function *toExecute;

			// The variable to load (if any).
			LocalVar *toLoad;

			// Parameters
			Auto<Actual> params;

			// Generate code to call a function.
			void callCode(const GenState &s, code::Variable to);

			// Generate code to load a variable.
			void loadCode(const GenState &s, code::Variable to);

			// Find what to call.
			void findTarget(Auto<Scope> scope, const Name &name, const SrcPos &pos);
		};

	}
}
