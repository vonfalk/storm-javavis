#pragma once
#include "BSExpr.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		/**
		 * Functions for automatic casting in Basic Storm. The casting rules are inspired from C++,
		 * but are generally more restrictive. Basic Storm only uses constructors marked with the
		 * flag namedAutoCast. Aside from that, a few expressions can be casted automatically.
		 */

		// Is the value 'from' castable to 'to'?
		Bool castable(Par<Expr> from, Value to, NamedFlags mode);
		Bool STORM_FN castable(Par<Expr> from, Value to);

		// What is the penalty of casting 'from' to 'to'? Returns -1 if not possible.
		Int castPenalty(Par<Expr> from, Value to, NamedFlags mode);
		Int STORM_FN castPenalty(Par<Expr> from, Value to);

		// Return an expression that returns the type 'to'. If nothing special needs to be done,
		// simply returns 'from'. Note that the returned expr may differ in the 'ref' member!
		Expr *castTo(Par<Expr> from, Value to, NamedFlags mode);
		MAYBE(Expr) *STORM_FN castTo(Par<Expr> from, Value to);

		// Same as 'castTo', but will throw a type error if it is not possible to do the cast.
		Expr *STORM_FN expectCastTo(Par<Expr> from, Value to);

		// Get the lowest common type that can store both expressions.
		Value STORM_FN common(Par<Expr> a, Par<Expr> b);

	}
}
