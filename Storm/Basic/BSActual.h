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
			STORM_CTOR Actual(Par<Expr> expr);

			// Parameters.
			vector<Auto<Expr> > expressions;

			// Compute all types.
			vector<Value> values();

			// Generate the code to get one parameter. Returns where it is stored.
			// 'type' may differ slightly from 'expressions[id]->result()'.
			code::Value code(nat id, Par<CodeGen> s, Value type);

			// Empty?
			inline Bool STORM_FN empty() { return expressions.empty(); }

			// Add a parameter.
			void STORM_FN add(Par<Expr> expr);

			// Add a parameter to the beginning.
			void STORM_FN addFirst(Par<Expr> expr);

		protected:
			virtual void output(wostream &to) const;
		};

		/**
		 * Extension to the name part class that takes care of the automatic casts supported by
		 * Basic Storm. It delegates these checks to the BSAutocast-header.
		 *
		 * Note that Actual needs to be updated (to some degree) to make some casts work.
		 */
		class BSNamePart : public FoundParams {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR BSNamePart(Par<Str> name, Par<Actual> params);

			// C++ version.
			BSNamePart(const String &name, Par<Actual> params);

			// Insert a type as the first parameter (used for this pointers).
			void STORM_FN insert(Value first);
			void STORM_FN insert(Value first, Nat at);

			// Alter an expression.
			void STORM_FN alter(Nat id, Value to);

			// Matches?
			Int STORM_FN matches(Par<Named> candidate);

		private:
			// Original expressions. (may contain null).
			vector<Auto<Expr>> exprs;
		};

		// Helper to create a Name with one BSNamePart inside of it.
		Name *STORM_FN bsName(Par<Str> name, Par<Actual> params);
		Name *bsName(const String &name, Par<Actual> params);
	}
}
