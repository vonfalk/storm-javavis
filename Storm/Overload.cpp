#include "stdafx.h"
#include "Overload.h"

namespace storm {

	NameOverload::NameOverload(const String &name, const vector<Value> &params) : Named(name), params(params) {}


	Overload::Overload(const String &name) : Named(name) {}

	Overload::~Overload() {}

	void Overload::output(wostream &to) const {
		if (items.size() == 0) {
			to << name << L" (nothing)" << endl;
		} else {
			for (nat i = 0; i < items.size(); i++) {
				to << *items[i] << endl;
			}
		}
	}

	void Overload::add(NameOverload *n) {
		assert(n->name == name);
		assert(find(n->params) == null);
		items.push_back(n);
	}

	NameOverload *Overload::find(const vector<Value> &params) {
		for (nat i = 0; i < items.size(); i++) {
			if (suitable(items[i], params))
				return items[i].borrow();
		}
		return null;
	}

	bool Overload::suitable(const Auto<NameOverload> &overload, const vector<Value> &params) {
		// No default-parameters yet!
		if (overload->params.size() != params.size())
			return false;

		for (nat i = 0; i < params.size(); i++) {
			if (!overload->params[i].canStore(params[i]))
				return false;
		}

		return true;
	}

}
