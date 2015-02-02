#include "stdafx.h"
#include "BSParams.h"

namespace storm {

	bs::Params::Params() : thisType(null) {}

	void bs::Params::add(Par<TypeName> type) {
		params.push_back(type);
	}

	void bs::Params::add(Par<SStr> name) {
		names.push_back(name);
	}

	vector<String> bs::Params::cNames() const {
		vector<String> r;
		r.reserve(names.size() + (thisType ? 1 : 0));
		if (thisType)
			r.push_back(L"this");

		for (nat i = 0; i < names.size(); i++)
			r.push_back(names[i]->v->v);

		return r;
	}

	vector<Value> bs::Params::cTypes(const Scope &scope) const {
		vector<Value> r;
		r.reserve(params.size() + (thisType ? 1 : 0));
		if (thisType)
			r.push_back(Value::thisPtr(thisType));

		for (nat i = 0; i < params.size(); i++)
			r.push_back(params[i]->value(scope));

		return r;
	}

	void bs::Params::addThis(Type *t) {
		thisType = t;
	}

	void bs::Params::output(wostream &to) const {
		to << L"(";
		join(to, params, L", ");
		to << L")";
	}
}
