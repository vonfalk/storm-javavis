#pragma once
#include "BSExpr.h"
#include "SyntaxObject.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		/**
		 * Actual parameters to a function.
		 */
		class Actual : public SObject {
			STORM_CLASS;
		public:
			STORM_CTOR Actual();

			// Parameters.
			vector<Auto<Expr> > expressions;

			// Compute all types.
			vector<Value> values();

			// Generate the code to get one parameter. Returns where it is stored.
			// 'type' may differ slightly from 'expressions[id]->result()'.
			code::Value code(nat id, const GenState &s, const Value &type);

			// Add a parameter.
			void STORM_FN add(Par<Expr> expr);

			// Add a parameter to the beginning.
			void STORM_FN addFirst(Par<Expr> expr);

		protected:
			virtual void output(wostream &to) const;
		};

	}
}
