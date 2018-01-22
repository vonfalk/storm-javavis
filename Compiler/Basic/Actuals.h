#pragma once
#include "Compiler/NamedThread.h"
#include "Compiler/Syntax/SStr.h"
#include "Compiler/Name.h"
#include "Compiler/Scope.h"
#include "Expr.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		/**
		 * Actual parameters to a function.
		 */
		class Actuals : public ObjectOn<Compiler> {
			STORM_CLASS;
		public:
			STORM_CTOR Actuals();
			STORM_CTOR Actuals(Expr *expr);

			// Parameters.
			Array<Expr *> *expressions;

			// Compute all types.
			Array<Value> *values();

			// Generate the code to get one parameter. Returns where it is stored.
			// 'type' may differ slightly from 'expressions->at(id)->result()'.
			code::Operand code(nat id, CodeGen *s, Value type, Scope scope);

			// Empty?
			inline Bool STORM_FN empty() { return expressions->empty(); }

			// Add a parameter.
			void STORM_FN add(Expr *expr);

			// Add a parameter to the beginning.
			void STORM_FN addFirst(Expr *expr);

		protected:
			virtual void STORM_FN toS(StrBuf *to) const;
		};

		/**
		 * Extension to the name part class that takes care of the automatic casts supported by
		 * Basic Storm. It delegates these checks to the BSAutocast-header.
		 *
		 * Note that Actuals needs to be updated (to some degree) to make some casts work.
		 */
		class BSNamePart : public SimplePart {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR BSNamePart(syntax::SStr *name, Actuals *params);
			STORM_CTOR BSNamePart(Str *name, SrcPos pos, Actuals *params);
			BSNamePart(const wchar *name, SrcPos pos, Actuals *params);

			// Insert a type as the first parameter (used for this pointers).
			void STORM_FN insert(Value first);
			void STORM_FN insert(Value first, Nat at);

			// Alter an expression.
			void STORM_FN alter(Nat id, Value to);

			// Matches?
			virtual Int STORM_FN matches(Named *candidate, Scope source) const;

		private:
			// Original expressions. (may contain null).
			Array<Expr *> *exprs;

			// Position of this part.
			SrcPos pos;
		};

		// Helper to create a Name with one BSNamePart inside of it.
		Name *STORM_FN bsName(syntax::SStr *name, Actuals *params) ON(Compiler);
		Name *STORM_FN bsName(Str *name, SrcPos pos, Actuals *params) ON(Compiler);

	}
}
