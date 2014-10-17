#include "stdafx.h"
#include "Overload.h"

namespace storm {

	NameOverload::NameOverload(const String &name, const vector<Value> &params) : Named(name), params(params) {}


	Overload::Overload(const String &name) : Named(name) {}

	Overload::~Overload() {
		for (ItemMap::iterator i = items.begin(); i != items.end(); i++) {
			delete i->v;
		}
	}

	void Overload::output(wostream &to) const {
		if (items.size() == 0) {
			to << name << L" (nothing)" << endl;
		} else {
			for (ItemMap::const_iterator i = items.begin(); i != items.end(); i++) {
				NameOverload *no = i->v;
				to << *no;
			}
		}
	}

	void Overload::add(NameOverload *n) {
		Item toInsert(n);
		assert(n->name == name);
		assert(items.count(toInsert) == 0);
		items.insert(toInsert);
	}

	/**
	 * Value.
	 */

	Overload::Item::Item(const vector<Value> &k) : k(k), v(null) {}

	Overload::Item::Item(NameOverload *overload) : k(overload->params), v(overload) {}

	bool Overload::Item::operator <(const Item &o) const {
		const vector<Value> &a = k;
		const vector<Value> &b = o.k;

		if (a.size() == b.size())
			return a.size() < b.size();

		for (nat i = 0;i < a.size(); i++) {
			if (a[i] != b[i])
				return a[i] < b[i];
		}

		return false;
	}

}
