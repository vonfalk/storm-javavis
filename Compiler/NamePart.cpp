#include "stdafx.h"
#include "NamePart.h"
#include "SrcPos.h"
#include "Scope.h"
#include "Name.h"
#include "Type.h"
#include "Engine.h"
#include "Core/Str.h"
#include "Core/StrBuf.h"
#include "Core/CloneEnv.h"
#include "NameSet.h"
#include "Exception.h"
#include <limits>

namespace storm {

	extern void gcCheck(void *, const wchar *);

	NamePart::NamePart(Str *name) : name(name) {
		if (wcscmp(name->c_str(), L"@ 100") == 0) {
			// DebugBreak();
			gcCheck(name, L"Str");
			int top;
			PLN(L"Allocated " << (void *)this << L" with stack top " << &top << L" and str " << (void *)name);
		}
	}

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

	SimplePart::SimplePart(Str *name, Array<Value> *params) : NamePart(name), params(params) {}

	SimplePart::SimplePart(Str *name, Value param) : NamePart(name), params(new (engine()) Array<Value>(1, param)) {}

	void SimplePart::deepCopy(CloneEnv *env) {
		NamePart::deepCopy(env);
		cloned(params, env);
	}

	SimplePart *SimplePart::find(const Scope &scope) {
		return this;
	}

	Named *SimplePart::choose(NameOverloads *from) const {
		Array<Named *> *candidates = new (this) Array<Named *>();
		int best = std::numeric_limits<int>::max();

		for (nat i = 0; i < from->count(); i++) {
			Named *candidate = from->at(i);
			int badness = matches(candidate);
			if (badness >= 0 && badness <= best) {
				if (badness != best)
					candidates->clear();
				best = badness;
				candidates->push(candidate);
			}
		}

		if (candidates->count() == 0) {
			return null;
		} else if (candidates->count() == 1) {
			return candidates->at(0);
		} else {
			StrBuf *msg = new (this) StrBuf();
			*msg << L"Multiple possible matches for " << this << L", all with badness " << best << L"\n";
			for (nat i = 0; i < candidates->count(); i++)
				*msg << L" Could be: " << candidates->at(i)->identifier() << L"\n";
			throw TypeError(SrcPos(), msg->c_str());
		}
	}

	Int SimplePart::matches(Named *candidate) const {
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

	Bool SimplePart::equals(Object *other) const {
		if (type() != other->type())
			return false;
		SimplePart *o = (SimplePart *)other;
		if (!name->equals(o->name))
			return false;
		if (params->count() != o->params->count())
			return false;

		for (nat i = 0; i < params->count(); i++)
			if (params->at(i) != o->params->at(i))
				return false;

		return true;
	}

	Nat SimplePart::hash() const {
		Nat r = 5381;
		r = ((r << 5) + r) + name->hash();
		// Note: we only care about the number of parameters so far.
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
		} catch (const Exception &) {
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
