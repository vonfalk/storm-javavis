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
			LocalVar(const String &name, const Value &val, const SrcPos &pos, bool param = false);

			// Type.
			Value result;

			// Declared at
			SrcPos pos;

			// Location (not initialized until code generation).
			code::Variable var;

			// Is this a parameter?
			bool param;
		};

		class Block;

		/**
		 * Local variable delcaration.
		 */
		class Var : public Expr {
			STORM_CLASS;
		public:
			// Initialize to zero or null (remove?).
			STORM_CTOR Var(Auto<Block> block, Auto<TypeName> type, Auto<SStr> name);

			// Initialize to an expression.
			STORM_CTOR Var(Auto<Block> block, Auto<TypeName> type, Auto<SStr> name, Auto<Expr> init);

			// Initialize to an expression (auto type).
			STORM_CTOR Var(Auto<Block> block, Auto<SStr> name, Auto<Expr> init);

			// Declared variable.
			Auto<LocalVar> variable;

			// Initialize to.
			Auto<Expr> initExpr;

			// Result type.
			virtual Value result();

			// Generate code.
			virtual void code(const GenState &state, GenResult &to);

		private:
			// Initialize.
			void init(Auto<Block> block, const Value &type, Auto<SStr> name);

			// Set return value.
			void initTo(Auto<Expr> expr);
		};

	}
}

#include "BSBlock.h"
