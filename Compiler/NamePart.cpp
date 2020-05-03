#include "stdafx.h"
#include "NamePart.h"
#include "Core/SrcPos.h"
#include "Scope.h"
#include "Name.h"
#include "Type.h"
#include "Core/Str.h"
#include "Core/StrBuf.h"
#include "Core/CloneEnv.h"
#include "NameSet.h"
#include "Exception.h"
#include <limits>

namespace storm {

	NamePart::NamePart(Str *name) : name(name) {}

	NamePart::NamePart(const wchar *name) : name(new (engine()) Str(name)) {}

	void NamePart::deepCopy(CloneEnv *env) {
		// Nothing mutable in here.
	}

	SimplePart *NamePart::find(const Scope &scope) {
		return new (this) SimplePart(name);
	}

	void NamePart::toS(StrBuf *to) const {
		*to << name;
	}


	/**
	 * SimplePart.
	 */

	SimplePart::SimplePart(Str *name) : NamePart(name), params(new (engine()) Array<Value>()) {}

	SimplePart::SimplePart(syntax::SStr *name) : NamePart(name->v), params(new (engine()) Array<Value>()) {}

	SimplePart::SimplePart(const wchar *name) : NamePart(name), params(new (engine()) Array<Value>()) {}

	SimplePart::SimplePart(Str *name, Array<Value> *params) : NamePart(name), params(params) {}

	SimplePart::SimplePart(Str *name, Value param) : NamePart(name), params(new (engine()) Array<Value>(1, param)) {}

	void SimplePart::deepCopy(CloneEnv *env) {
		NamePart::deepCopy(env);
		cloned(params, env);
	}

	SimplePart *SimplePart::find(const Scope &scope) {
		return this;
	}

	Named *SimplePart::choose(NameOverloads *from, Scope source) const {
		// Note: We do this in two steps. First, we find the best match, and keep track of whether
		// or not there are multiple instances of that or not. If there are multiple instances of
		// the best match, we need to produce an error. To do that, we loop through the candidates a
		// second time (since the error path is generally not critical).
		Named *bestCandidate = null;
		Bool multipleBest = false;
		Int best = std::numeric_limits<Int>::max();

		for (Nat i = 0; i < from->count(); i++) {
			Named *candidate = from->at(i);
			// Do not consider 'candidate' if we're not allowed to access it! Note: no 'visibility' means 'public'.
			if (!candidate->visibleFrom(source))
				continue;

			Int badness = matches(candidate, source);
			if (badness < 0 || badness > best)
				continue;

			if (badness == best) {
				// Multiple best matches so far. We can't keep track of them without allocating memory.
				multipleBest = true;
			} else {
				multipleBest = false;
				best = badness;
				bestCandidate = candidate;
			}
		}

		if (!multipleBest) {
			// We have an answer!
			return bestCandidate;
		}

		// Error case, we need to find all candidates.
		StrBuf *msg = new (this) StrBuf();
		*msg << S("Multiple possible matches for ") << this << S(", all with badness ") << best << S("\n");

		for (Nat i = 0; i < from->count(); i++) {
			Named *candidate = from->at(i);
			// Do not consider 'candidate' if we're not allowed to access it! Note: no 'visibility' means 'public'.
			if (!candidate->visibleFrom(source))
				continue;

			Int badness = matches(candidate, source);
			if (badness == best)
				*msg << S("  Could be: ") << candidate->identifier() << S("\n");
		}

		throw new (this) LookupError(msg->toS());
	}

	Int SimplePart::matches(Named *candidate, Scope source) const {
		Array<Value> *c = candidate->params;
		if (c->count() != params->count())
			return -1;

		int distance = 0;

		for (nat i = 0; i < c->count(); i++) {
			const Value &match = c->at(i);
			const Value &ours = params->at(i);

			if (match == Value() && ours != Value())
				return -1;
			if (!match.matches(ours, candidate->flags))
				return -1;
			if (ours.type && match.type)
				distance += ours.type->distanceFrom(match.type);
		}

		return distance;
	}

	void SimplePart::toS(StrBuf *to) const {
		*to << name;
		if (params != null && params->count() > 0) {
			*to << L"(" << params->at(0);
			for (nat i = 1; i < params->count(); i++)
				*to << L", " << params->at(i);
			*to << L")";
		}
	}

	Bool SimplePart::operator ==(const SimplePart &o) const {
		if (!sameType(this, &o))
			return false;

		if (*name != *o.name)
			return false;
		if (params->count() != o.params->count())
			return false;

		for (Nat i = 0; i < params->count(); i++)
			if (params->at(i) != o.params->at(i))
				return false;

		return true;
	}

	Nat SimplePart::hash() const {
		Nat r = 5381;
		r = ((r << 5) + r) + name->hash();
		// Note: we only care about the number of parameters so far. This is probably a decent
		// trade-off between a good-enough hash with few collisions, and the cost of actually
		// traversing all parameters recursively (with the possibility of infinite recursion). Note
		// that we can not hash the pointers, even if we can compare them in the '==' operator since
		// objects move.
		r = ((r << 5) + r) + params->count();
		return r;
	}


	/**
	 * RecPart.
	 */

	RecPart::RecPart(Str *name) : NamePart(name), params(new (engine()) Array<Name *>()) {}

	RecPart::RecPart(syntax::SStr *name) : NamePart(name->v), params(new (engine()) Array<Name *>()) {}

	RecPart::RecPart(Str *name, Array<Name *> *params) : NamePart(name), params(params) {}

	void RecPart::deepCopy(CloneEnv *env) {
		NamePart::deepCopy(env);
		cloned(params, env);
	}

	SimplePart *RecPart::find(const Scope &scope) {
		Array<Value> *v = new (this) Array<Value>();
		v->reserve(params->count());

		try {
			for (nat i = 0; i < params->count(); i++) {
				Name *n = params->at(i);
				if (SrcName *s = as<SrcName>(n)) {
					v->push(scope.value(s, s->pos));
				} else {
					v->push(scope.value(n, SrcPos()));
				}
			}
		} catch (const CodeError *) {
			// It seems like a compilation error, propagate it further now. Otherwise, we get really
			// strange error messages in some cases (e.g. type X not found due to a compilation
			// error inside that type).
			throw;
		} catch (const Exception *) {
			// Return null as per specification.
			return null;
		}

		return new (this) SimplePart(name, v);
	}

	void RecPart::toS(StrBuf *to) const {
		*to << name;
		if (params->count() > 0) {
			*to << L"(" << params->at(0);
			for (nat i = 1; i < params->count(); i++)
				*to << L", " << params->at(i);
			*to << L")";
		}
	}

}
