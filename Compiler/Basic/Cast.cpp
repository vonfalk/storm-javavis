#include "stdafx.h"
#include "Cast.h"
#include "Actuals.h"
#include "Named.h"
#include "Compiler/Type.h"
#include "Compiler/Exception.h"
#include "Compiler/Lib/Maybe.h"

namespace storm {
	namespace bs {

		// TODO: Move some of the logic from the function call wrappers in BSNamed/BSActual to the autocast
		// functionality instead (such as casting to/from references). This will make things much cleaner!

		// Our rules. We do not care about Maybe<T> here, it is done later.
		static bool canStore(const Value &from, const Value &to) {
			if (to.type == null)
				return true;
			if (from.type == null)
				return false;

			// Respect inheritance.
			if (!from.type->isA(to.type))
				return false;

			// Only values can be casted to a reference automatically.
			if (to.isObject())
				if (to.ref && !from.ref)
					return false;

			return true;
		}

		// Find a cast ctor.
		static Function *castCtor(Value from, Value to, Scope scope) {
			if (!to.type)
				return null;

			Array<Value> *params = new (to.type) Array<Value>(2, from);
			params->at(0) = thisPtr(to.type);
			Named *n = to.type->find(Type::CTOR, params, scope);
			if (Function *ctor = as<Function>(n))
				if (ctor->fnFlags() & fnAutoCast)
					return ctor;

			return null;
		}

		void expectCastable(Expr *from, Value to, Scope scope) {
			if (!castable(from, to, scope))
				throw new (from) TypeError(from->pos, to, from->result());
		}

		Bool castable(Expr *from, Value to, Scope scope) {
			return castable(from, to, namedDefault, scope);
		}

		Bool castable(Expr *from, Value to, NamedFlags mode, Scope scope) {
			return castPenalty(from, to, mode, scope) >= 0;
		}

		Int castPenalty(Expr *from, Value to, Scope scope) {
			return castPenalty(from, to, namedDefault, scope);
		}

		Int castPenalty(Expr *from, ExprResult fromResult, Value to, NamedFlags mode, Scope scope) {
			Int penalty = plainCastPenalty(fromResult, to, mode);
			if (penalty >= 0)
				return penalty;

			// Special cast supported directly by the Expr?
			penalty = from->castPenalty(to);
			if (penalty >= 0)
				return 100 * penalty; // Quite large penalty, so that we prefer functions without casting.

			// If 'to' is a value, we can create a reference to it without problem.
			if (!to.isObject() && to.ref) {
				Int penalty = from->castPenalty(to.asRef(false));
				if (penalty >= 0)
					return 100 * penalty;
			}

			// Find a cast ctor!
			if (castCtor(fromResult.type(), to, scope))
				return 1000; // Larger than casting literals.

			return -1;
		}

		Int castPenalty(Expr *from, Value to, NamedFlags mode, Scope scope) {
			return castPenalty(from, from->result(), to, mode, scope);
		}

		Expr *castTo(Expr *from, Value to, Scope scope) {
			return castTo(from, to, namedDefault, scope);
		}

		Int plainCastPenalty(ExprResult from, Value to, NamedFlags mode) {
			// We want to allow 'casting' <nothing> into anything. This is good, since we want to be
			// able to place non-returning functions basically anywhere (especially in if-statements).
			if (from.nothing())
				return 0;
			Value f = from.type();

			if (mode & namedMatchNoInheritance)
				return (canStore(f, to) && f.type == to.type) ? 0 : -1;

			// No work needed?
			if (canStore(f, to)) {
				if (f.type != null && to.type != null)
					return f.type->distanceFrom(to.type);
				else
					return 0;
			}

			return -1;
		}


		Expr *castTo(Expr *from, Value to, NamedFlags mode, Scope scope) {
			ExprResult r = from->result();
			if (r.nothing())
				return from;

			// Safeguard.
			if (castPenalty(from, r, to, mode, scope) < 0)
				return null;

			Value f = r.type();

			// No work?
			if (canStore(f, to))
				return from;

			// Supported cast?
			if (from->castPenalty(to) >= 0)
				return from;

			if (!to.isObject() && to.ref)
				if (from->castPenalty(to.asRef(false)) >= 0)
					return from;

			// Cast ctor?
			if (Function *ctor = castCtor(f, to, scope)) {
				Actuals *params = new (from) Actuals(from);
				return new (from) CtorCall(from->pos, scope, ctor, params);
			}

			return null;
		}

		Expr *expectCastTo(Expr *from, Value to, Scope scope) {
			if (Expr *r = castTo(from, to, scope))
				return r;
			throw new (from) TypeError(from->pos, to, from->result());
		}

		ExprResult common(Expr *a, Expr *b, Scope scope) {
			ExprResult ar = a->result();
			ExprResult br = b->result();

			// Do we need to wrap/unwrap maybe types?
			if (isMaybe(ar.type()) || isMaybe(br.type())) {
				Value r = common(unwrapMaybe(ar.type()), unwrapMaybe(br.type()));
				if (r != Value())
					return wrapMaybe(r);
			}

			// Simple inheritance?
			ExprResult r = common(ar, br);
			if (r != ExprResult())
				return r;

			// Now, ar and br should have some type.
			Value av = ar.type();
			Value bv = br.type();

			// Simple casting?
			if (castable(a, bv, scope))
				return bv;
			if (castable(b, av, scope))
				return av;

			// Nothing possible... Return void.
			return ExprResult();
		}

		Bool implicitlyCallableCtor(Function *ctor) {
			if (ctor->params->count() != 2)
				return false;

			Value first = ctor->params->at(0);
			Value second = ctor->params->at(1);

			// This is the copy-ctor (at least, close enough).
			if (first.type == second.type)
				return true;

			// If it was explicitly declared as a cast-ctor, then it is also fine.
			if (ctor->fnFlags() & fnAutoCast)
				return true;

			// Else, we don't allow it.
			return false;
		}

	}
}
