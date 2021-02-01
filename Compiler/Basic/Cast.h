#pragma once
#include "Expr.h"
#include "Compiler/Scope.h"
#include "Compiler/NamePart.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		/**
		 * Functions for automatic casting in Basic Storm. The casting rules are inspired from C++,
		 * but are generally more restrictive. Basic Storm only uses constructors marked with the
		 * flag namedAutoCast. Aside from that, a few expressions can be casted automatically.
		 */

		// Is the value 'from' castable to 'to'?
		Bool castable(Expr *from, Value to, NamedFlags mode, Scope scope);
		Bool STORM_FN castable(Expr *from, Value to, Scope scope);

		// Same as 'castable', but throws an exception on failure.
		void STORM_FN expectCastable(Expr *from, Value to, Scope scope);

		// What is the penalty of casting 'from' to 'to'? Returns -1 if not possible. Note: We will
		// always allow casting '<nothing>' into anything. This is convenient when implementing
		// if-statements for example.
		Int castPenalty(Expr *from, Value to, NamedFlags mode, Scope scope);
		Int STORM_FN castPenalty(Expr *from, Value to, Scope scope);

		// What is the penalty of casting 'from' to 'to', disregarding any automatic type conversions.
		Int plainCastPenalty(ExprResult from, Value to, NamedFlags mode);

		// Return an expression that returns the type 'to'. If nothing special needs to be done,
		// simply returns 'from'. Note that the returned expr may differ in the 'ref' member!
		Expr *castTo(Expr *from, Value to, NamedFlags mode, Scope scope);
		MAYBE(Expr *) STORM_FN castTo(Expr *from, Value to, Scope scope);

		// Same as 'castTo', but will throw a type error if it is not possible to do the cast.
		Expr *STORM_FN expectCastTo(Expr *from, Value to, Scope scope);

		// Get the lowest common type that can store both expressions.
		ExprResult STORM_FN common(Expr *a, Expr *b, Scope scope);

		// Check if 'ctor' is implicitly callable.
		Bool STORM_FN implicitlyCallableCtor(Function *ctor);


		/**
		 * Class used to find suitable constructors. This works like the plain SimplePart
		 * implementation, but is aware of "maybe" instances.
		 */
		class MaybeAwarePart : public SimplePart {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR MaybeAwarePart(Str *name, Array<Value> *params);

			// Custom weight function.
			virtual Int STORM_FN matches(Named *candidate, Scope source) const;
		};

	}
}
