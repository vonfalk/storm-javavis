#include "stdafx.h"
#include "NamePart.h"
#include "SrcPos.h"
#include "Scope.h"
#include "Name.h"
#include "Core/StrBuf.h"
#include "Core/CloneEnv.h"
#include "NameSet.h"
#include "Exception.h"
#include <limits>

namespace storm {

	NamePart::NamePart(Str *name) : name(name) {}

	NamePart::NamePart(NamePart *o) : name(o->name) {}

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

	SimplePart::SimplePart(Str *name, Array<Value> *params) : NamePart(name), params(params) {}

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
			if (c->at(i) != params->at(i)) {
				TODO(L"Implement matching properly!");
				distance = -1;
			}
		}

		return distance;
	}

	void SimplePart::toS(StrBuf *to) const {
		*to << name;
		if (params->count() > 0) {
			*to << L"(" << params->at(0);
			for (nat i = 1; i < params->count(); i++)
				*to << L", " << params->at(i);
			*to << L")";
		}
	}


	/**
	 * RecPart.
	 */

	RecPart::RecPart(Str *name) : NamePart(name), params(new (engine()) Array<Name *>()) {}

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
				v->push(scope.value(params->at(i), SrcPos()));
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