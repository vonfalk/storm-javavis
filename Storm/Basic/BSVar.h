#pragma once
#include "BSExpr.h"
#include "BSType.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		/**
		 * Local variable.
		 * TODO? NameOverload as well? For member variables?
		 */
		class LocalVar : public Named {
			STORM_CLASS;
		public:
			LocalVar(const String &name, const Value &val, const SrcPos &pos);

			// Type.
			Value result;

			// Declared at
			SrcPos pos;

			// Location (not initialized until code generation).
			code::Variable var;
		};


		/**
		 * Local variable delcaration.
		 */
		class Var : public Expr {
			STORM_CLASS;
		public:
			STORM_CTOR Var(Auto<Block> block, Auto<TypeName> type, Auto<SStr> name);
			STORM_CTOR Var(Auto<Block> block, Auto<SStr> name, Auto<Expr> init);

			// Declared variable.
			Auto<LocalVar> variable;

			// Initialize to.
			Auto<Expr> initExpr;

			// Set initializing expression.
			void STORM_FN initTo(Auto<Expr> e);

			// Result type.
			virtual Value result();

			// Generate code.
			virtual void code(const GenState &state, code::Variable to);

		private:
			// Initialize.
			void init(Auto<Block> block, const Value &type, Auto<SStr> name);
		};


	}
}
