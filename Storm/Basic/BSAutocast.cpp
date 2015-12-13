#include "stdafx.h"
#include "BSAutocast.h"
#include "BSActual.h"
#include "BSNamed.h"
#include "Type.h"
#include "Function.h"
#include "Exception.h"
#include "Lib/Maybe.h"

namespace storm {
	using namespace bs;

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

		Auto<Named> n = to.type->findCpp(Type::CTOR, valList(2, Value::thisPtr(to.type), from));
		Auto<Function> ctor = n.as<Function>();
		if (ctor)
			if (ctor->flags & namedAutoCast)
				return ctor.borrow();

		return null;
	}

	Bool bs::castable(Par<Expr> from, Value to) {
		return castable(from, to, namedDefault);
	}

	void bs::expectCastable(Par<Expr> from, Value to) {
		if (!castable(from, to))
			throw TypeError(from->pos, to, from->result());
	}

	Bool bs::castable(Par<Expr> from, Value to, NamedFlags mode) {
		return castPenalty(from, to, mode) >= 0;
	}

	Int bs::castPenalty(Par<Expr> from, Value to) {
		return castPenalty(from, to, namedDefault);
	}

	Int bs::castPenalty(Par<Expr> from, Value to, NamedFlags mode) {
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

	Expr *bs::castTo(Par<Expr> from, Value to) {
		return castTo(from, to, namedDefault);
	}

	Expr *bs::castTo(Par<Expr> from, Value to, NamedFlags mode) {
		// Safeguard.
		if (!castable(from, to, mode))
			return null;

		ExprResult r = from->result();
		if (!r.any())
			return from.ret();
		Value f = r.type();

		// No work?
		if (canStore(f, to))
			return from.ret();

		// Supported cast?
		if (from->castPenalty(to) >= 0)
			return from.ret();

		if (!to.isClass() && to.ref)
			if (from->castPenalty(to.asRef(false)) >= 0)
				return from.ret();

		// Cast ctor?
		if (Function *ctor = castCtor(f, to)) {
			Auto<Actual> params = CREATE(Actual, from, from);
			return CREATE(CtorCall, from, ctor, params);
		}

		return null;
	}

	Expr *bs::expectCastTo(Par<Expr> from, Value to) {
		if (Expr *r = castTo(from, to))
			return r;
		throw TypeError(from->pos, to, from->result());
	}

	ExprResult bs::common(Par<Expr> a, Par<Expr> b) {
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
