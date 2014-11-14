#include "stdafx.h"
#include "Name.h"

namespace storm {

	Name::Name() {}

	Name::Name(const String &path) {
		parts = path.split(L".");
	}

	Name::Name(const vector<String> &parts) : parts(parts) {}

	Name &Name::operator +=(const Name &o) {
		for (nat i = 0; i < o.parts.size(); i++)
			parts.push_back(o.parts[i]);
		return *this;
	}

	Name Name::operator +(const Name &o) const {
		Name t(*this);
		t += o;
		return t;
	}

	Name Name::parent() const {
		Name p(*this);
		if (!root())
			p.parts.pop_back();
		return p;
	}

	String Name::last() const {
		return parts.back();
	}

	Name Name::from(nat f) const {
		Name r;
		for (nat i = f; f < parts.size(); f++) {
			r.parts.push_back(parts[i]);
		}
		return r;
	}

	void Name::output(std::wostream &to) const {
		join(to, parts, L".");
	}
}
