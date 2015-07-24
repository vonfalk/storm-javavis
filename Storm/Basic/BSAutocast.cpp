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

		Function *ctor = as<Function>(to.type->findCpp(Type::CTOR, valList(2, Value::thisPtr(to.type), from)));
		if (ctor)
			if (ctor->flags & namedAutoCast)
				return ctor;

		return null;
	}

	Bool bs::castable(Par<Expr> from, Value to) {
		return castable(from, to, namedDefault);
	}

	Bool bs::castable(Par<Expr> from, Value to, NamedFlags mode) {
		return castPenalty(from, to, mode) >= 0;
	}

	Int bs::castPenalty(Par<Expr> from, Value to) {
		return castPenalty(from, to, namedDefault);
	}

	Int bs::castPenalty(Par<Expr> from, Value to, NamedFlags mode) {
		Value f = from->result();

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
		if (from->castable(to))
			return 100; // Quite large penalty, so that we prefer functions without casting.

		// If 'to' is a value, we can create a reference to it without problem.
		if (!to.isClass() && to.ref) {
			if (from->castable(to.asRef(false)))
				return 100;
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

		Value f = from->result();

		// No work?
		if (canStore(f, to))
			return from.ret();

		// Supported cast?
		if (from->castable(to))
			return from.ret();

		if (!to.isClass() && to.ref)
			if (from->castable(to.asRef(false)))
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

	Value bs::common(Par<Expr> a, Par<Expr> b) {
		Value at = a->result();
		Value bt = b->result();

		if (isMaybe(at) || isMaybe(bt)) {
			Value r = common(unwrapMaybe(at), unwrapMaybe(bt));
			if (r != Value())
				return wrapMaybe(r);
		}

		// Simple inheritance?
		Value r = common(at, bt);
		if (r != Value())
			return r;

		// Simple casting?
		if (castable(a, bt))
			return bt;
		if (castable(b, at))
			return at;

		// Nothing possible...
		return Value();
	}

}
