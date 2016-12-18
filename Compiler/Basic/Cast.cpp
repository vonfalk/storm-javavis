#include "stdafx.h"
#include "Cast.h"
#include "Actuals.h"
#include "Named.h"
#include "Compiler/Type.h"
#include "Compiler/Exception.h"
#include "Compiler/Lib/MaybeTemplate.h"

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
			if (to.isClass())
				if (to.ref && !from.ref)
					return false;

			return true;
		}

		// Find a cast ctor. (borrowed ptr).
		Function *castCtor(Value from, Value to) {
			if (!to.type)
				return null;

			Array<Value> *params = new (to.type) Array<Value>(2, from);
			params->at(0) = thisPtr(to.type);
			Named *n = to.type->find(Type::CTOR, params);
			if (Function *ctor = as<Function>(n))
				if (ctor->flags & namedAutoCast)
					return ctor;

			return null;
		}

		Bool castable(Expr *from, Value to) {
			return castable(from, to, namedDefault);
		}

		void expectCastable(Expr *from, Value to) {
			if (!castable(from, to))
				throw TypeError(from->pos, to, from->result());
		}

		Bool castable(Expr *from, Value to, NamedFlags mode) {
			return castPenalty(from, to, mode) >= 0;
		}

		Int castPenalty(Expr *from, Value to) {
			return castPenalty(from, to, namedDefault);
		}

		Int castPenalty(Expr *from, Value to, NamedFlags mode) {
			ExprResult r = from->result();
			if (!r.any())
				return 0;
			Value f = r.type();

			if (mode & namedMatchNoInheritance)
				return (canStore(f, to) && f.type == to.type) ? 0 : -1;

			// No work needed?
			if (canStore(f, to)) {
				if (f.type != null && to.type != null)
					return f.type->distanceFrom(to.type);
				else
					return 0;
			}

			// Special cast supported directly by the Expr?
			Int penalty = from->castPenalty(to);
			if (penalty >= 0)
				return 100 * penalty; // Quite large penalty, so that we prefer functions without casting.

			// If 'to' is a value, we can create a reference to it without problem.
			if (!to.isClass() && to.ref) {
				Int penalty = from->castPenalty(to.asRef(false));
				if (penalty >= 0)
					return 100 * penalty;
			}

			// Find a cast ctor!
			if (castCtor(f, to))
				return 1000; // Larger than casting literals.

			return -1;
		}

		Expr *castTo(Expr *from, Value to) {
			return castTo(from, to, namedDefault);
		}

		Expr *castTo(Expr *from, Value to, NamedFlags mode) {
			// Safeguard.
			if (!castable(from, to, mode))
				return null;

			ExprResult r = from->result();
			if (!r.any())
				return from;
			Value f = r.type();

			// No work?
			if (canStore(f, to))
				return from;

			// Supported cast?
			if (from->castPenalty(to) >= 0)
				return from;

			if (!to.isClass() && to.ref)
				if (from->castPenalty(to.asRef(false)) >= 0)
					return from;

			// Cast ctor?
			if (Function *ctor = castCtor(f, to)) {
				Actuals *params = new (from) Actuals(from);
				return new (from) CtorCall(from->pos, ctor, params);
			}

			return null;
		}

		Expr *expectCastTo(Expr *from, Value to) {
			if (Expr *r = castTo(from, to))
				return r;
			throw TypeError(from->pos, to, from->result());
		}

		ExprResult common(Expr *a, Expr *b) {
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
			if (castable(a, bv))
				return bv;
			if (castable(b, av))
				return av;

			// Nothing possible... Return void.
			return ExprResult();
		}


	}
}
