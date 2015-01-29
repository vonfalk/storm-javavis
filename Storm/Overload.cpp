#include "stdafx.h"
#include "Overload.h"
#include "Type.h"

namespace storm {

	Overload::Overload(NameLookup *parent, const String &name) : Named(name), p(parent) {}

	Overload::~Overload() {}

	NameLookup *Overload::parent() const {
		return p;
	}

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
		n->owner = this;
		items.push_back(capture(n));
	}

	NameOverload *Overload::find(const vector<Value> &params) {
		for (nat i = 0; i < items.size(); i++) {
			if (suitable(items[i], params))
				return items[i].borrow();
		}
		return null;
	}

	bool Overload::suitable(Par<NameOverload> overload, const vector<Value> &params) {
		// No default-parameters yet!
		if (overload->params.size() != params.size())
			return false;

		nat start = 0;

		if (params.size() >= 1 && params[0] == Value()) {
			Type *owner = as<Type>(p);
			if (!owner)
				return false;

			if (!overload->params[0].canStore(Value(owner)))
				return false;

			start = 1;
		}

		for (nat i = start; i < params.size(); i++) {
			if (!overload->params[i].canStore(params[i]))
				return false;
		}

		return true;
	}

	NameOverload::NameOverload(const String &name, const vector<Value> &params)
		: Named(name), owner(owner), params(params) {}

	String NameOverload::identifier() const {
		std::wostringstream ss;
		ss << parent()->identifier();
		ss << L"(";
		join(ss, params, L", ");
		ss << L")";
		return ss.str();
	}

	Name NameOverload::path() const {
		TODO(L"Make Name able to store parameters!");
		return parent()->path();
	}

	Overload *NameOverload::parent() const {
		assert(owner);
		return owner;
	}

}
